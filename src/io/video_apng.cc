/*
   Animated PNG encoder.

   APNG is an ordinary PNG plus three chunk types: acTL (animation control), fcTL
   (per-frame control) and fdAT (frame data after the first). The first frame is
   carried in a plain IDAT so a non-APNG decoder still shows a valid still image.
   Frames are independently compressed, so each one is just a complete zlib stream --
   which means we can let the already-vendored lodepng encode each frame to a normal
   PNG in memory and lift its IDAT payload back out. That costs no new dependency and
   no hand-rolled deflate.
 */

#include "io/VideoEncoder.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "lodepng/lodepng.h"

namespace {

/*
   Self-contained CRC32. lodepng exposes lodepng_crc32, but it is compiled out under
   LODEPNG_NO_COMPILE_CRC, and a chunk with a bad CRC is silently rejected by decoders
   rather than diagnosed -- not worth the configuration coupling for ten lines.
 */
uint32_t crc32(const uint8_t *data, size_t len, uint32_t crc = 0xffffffffu)
{
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xedb88320u & (~(crc & 1) + 1));
    }
  }
  return crc;
}

void appendBE32(std::string& out, uint32_t value)
{
  out.push_back(static_cast<char>((value >> 24) & 0xff));
  out.push_back(static_cast<char>((value >> 16) & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
  out.push_back(static_cast<char>(value & 0xff));
}

void appendBE16(std::string& out, uint16_t value)
{
  out.push_back(static_cast<char>((value >> 8) & 0xff));
  out.push_back(static_cast<char>(value & 0xff));
}

//! Appends a complete PNG chunk: length, type, payload, CRC of type+payload.
void appendChunk(std::string& out, const char type[4], const std::string& payload)
{
  appendBE32(out, static_cast<uint32_t>(payload.size()));
  const size_t typeStart = out.size();
  out.append(type, 4);
  out.append(payload);
  const auto *crcStart = reinterpret_cast<const uint8_t *>(out.data()) + typeStart;
  appendBE32(out, ~crc32(crcStart, 4 + payload.size()));
}

/*!
   Encodes one tightly-packed RGBA frame to a standalone PNG via lodepng and returns
   the concatenated IDAT payload -- i.e. the frame's complete zlib stream. `ihdr`, if
   non-null, receives the 13-byte IHDR payload. Returns false on encoder failure.
 */
bool encodeFrameData(const std::vector<uint8_t>& rgba, unsigned width, unsigned height,
                     std::string *frameData, std::string *ihdr)
{
  /*
     auto_convert must stay off. It picks the smallest lossless representation per
     call, so a solid red frame becomes a 1-bit palette image and a solid green one
     gets a different palette -- but APNG has a single IHDR and PLTE for the whole
     file, so the frames would all render in frame 0's colours. Forcing RGBA8 keeps
     every frame's IHDR identical and means there is no PLTE to carry.
   */
  LodePNGState state;
  lodepng_state_init(&state);
  state.info_raw.colortype = LCT_RGBA;
  state.info_raw.bitdepth = 8;
  state.info_png.color.colortype = LCT_RGBA;
  state.info_png.color.bitdepth = 8;
  state.encoder.auto_convert = 0;

  unsigned char *png = nullptr;
  size_t pngSize = 0;
  const unsigned error = lodepng_encode(&png, &pngSize, rgba.data(), width, height, &state);
  lodepng_state_cleanup(&state);
  if (error != 0) {
    free(png);
    return false;
  }
  const std::unique_ptr<unsigned char, decltype(&free)> owned(png, &free);

  frameData->clear();
  // Walk the chunk list, skipping the 8-byte signature.
  size_t pos = 8;
  while (pos + 12 <= pngSize) {
    const uint32_t length = (png[pos] << 24) | (png[pos + 1] << 16) | (png[pos + 2] << 8) |
                            png[pos + 3];
    if (pos + 12 + length > pngSize) return false;
    const char *type = reinterpret_cast<const char *>(png + pos + 4);
    const char *data = reinterpret_cast<const char *>(png + pos + 8);

    if (std::memcmp(type, "IDAT", 4) == 0) {
      frameData->append(data, length);
    } else if (ihdr != nullptr && std::memcmp(type, "IHDR", 4) == 0) {
      ihdr->assign(data, length);
    }
    pos += 12 + length;
  }
  return !frameData->empty();
}

class ApngEncoder : public VideoEncoder
{
public:
  bool open(std::ostream& out, unsigned width, unsigned height, unsigned fps) override
  {
    if (width == 0 || height == 0 || fps == 0 || fps > 0xffff) return false;
    out_ = &out;
    width_ = width;
    height_ = height;
    fps_ = fps;
    frames_ = 0;
    sequence_ = 0;
    ihdr_.clear();
    body_.clear();
    return true;
  }

  bool addFrame(const uint8_t *rgba, std::size_t stride) override
  {
    if (out_ == nullptr || rgba == nullptr) return false;

    // lodepng wants tightly-packed rows; the caller's buffer may be padded.
    const size_t rowBytes = static_cast<size_t>(width_) * 4;
    std::vector<uint8_t> packed(rowBytes * height_);
    for (unsigned y = 0; y < height_; ++y) {
      std::memcpy(packed.data() + y * rowBytes, rgba + y * stride, rowBytes);
    }

    std::string frameData;
    if (!encodeFrameData(packed, width_, height_, &frameData,
                         frames_ == 0 ? &ihdr_ : nullptr)) {
      return false;
    }

    // fcTL: sequence, size, offset, delay as a rational, dispose/blend ops.
    std::string fctl;
    appendBE32(fctl, sequence_++);
    appendBE32(fctl, width_);
    appendBE32(fctl, height_);
    appendBE32(fctl, 0);  // x_offset
    appendBE32(fctl, 0);  // y_offset
    appendBE16(fctl, 1);  // delay_num
    appendBE16(fctl, static_cast<uint16_t>(fps_));  // delay_den
    fctl.push_back(0);  // APNG_DISPOSE_OP_NONE
    fctl.push_back(0);  // APNG_BLEND_OP_SOURCE
    appendChunk(body_, "fcTL", fctl);

    if (frames_ == 0) {
      // The first frame is a normal IDAT, so plain PNG decoders still see an image.
      appendChunk(body_, "IDAT", frameData);
    } else {
      std::string fdat;
      appendBE32(fdat, sequence_++);
      fdat.append(frameData);
      appendChunk(body_, "fdAT", fdat);
    }

    ++frames_;
    return true;
  }

  bool close() override
  {
    if (out_ == nullptr) return false;
    if (frames_ == 0 || ihdr_.size() != 13) {
      out_ = nullptr;
      return false;
    }

    /*
       acTL has to precede the first frame but carries the frame count, which is only
       known now. Rather than requiring a seekable stream, the frame chunks were
       buffered and the whole file is emitted here; the buffer costs about one output
       file's worth of memory, which we are about to write to disk anyway.
     */
    std::string file("\x89PNG\r\n\x1a\n", 8);
    appendChunk(file, "IHDR", ihdr_);

    std::string actl;
    appendBE32(actl, frames_);
    appendBE32(actl, 0);  // num_plays: 0 means loop forever
    appendChunk(file, "acTL", actl);

    file.append(body_);
    appendChunk(file, "IEND", std::string());

    out_->write(file.data(), static_cast<std::streamsize>(file.size()));
    const bool ok = out_->good();
    out_ = nullptr;
    return ok;
  }

private:
  std::ostream *out_ = nullptr;
  unsigned width_ = 0;
  unsigned height_ = 0;
  unsigned fps_ = 0;
  uint32_t frames_ = 0;
  uint32_t sequence_ = 0;
  std::string ihdr_;
  std::string body_;
};

}  // namespace

std::unique_ptr<VideoEncoder> createApngEncoder()
{
  return std::make_unique<ApngEncoder>();
}
