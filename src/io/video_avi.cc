/*
   Motion-JPEG in an AVI container.

   The point of this backend is a lightly-compressed intermediate that transcoders and
   NLEs read without argument -- not a deliverable file. AVI is a RIFF tree: a 'hdrl'
   header list, a 'movi' list of one '00dc' chunk per JPEG frame, and an 'idx1' index
   whose entries must resolve to those chunks or players disagree about the file.

   Three sizes and two frame counts are only known once the last frame is in, so the
   header is written with placeholders and patched by seeking back in close(). The
   alternative -- buffering every JPEG in RAM -- would scale with the length of the
   animation, and animations are exactly the case where that gets expensive.
 */

#include "io/VideoEncoder.h"

#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "stb/stb_image_write.h"

namespace {

constexpr int JPEG_QUALITY = 90;

/*
   The header layout is fixed, so the fields that have to be patched in close() sit at
   known absolute offsets:

     0   RIFF | 4   size*        | 8   'AVI '
     12  LIST | 16  size         | 20  'hdrl'
     24  avih | 28  size         | 32  data ... 48 dwTotalFrames*
     88  LIST | 92  size         | 96  'strl'
     100 strh | 104 size         | 108 data ... 140 dwLength*
     164 strf | 168 size         | 172 data (40 bytes)
     212 LIST | 216 movi size*   | 220 'movi'          <- index offsets are from here
 */
constexpr uint32_t HDRL_TOTAL = 8 + 4 + (8 + 56) + (8 + 4 + (8 + 56) + (8 + 40));
constexpr size_t OFFSET_RIFF_SIZE = 4;
constexpr size_t OFFSET_AVIH_TOTAL_FRAMES = 48;
constexpr size_t OFFSET_STRH_LENGTH = 140;
constexpr size_t OFFSET_MOVI_FOURCC = 220;

void appendLE32(std::string& out, uint32_t value)
{
  out.push_back(static_cast<char>(value & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
  out.push_back(static_cast<char>((value >> 16) & 0xff));
  out.push_back(static_cast<char>((value >> 24) & 0xff));
}

void appendLE16(std::string& out, uint16_t value)
{
  out.push_back(static_cast<char>(value & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
}

class AviEncoder : public VideoEncoder
{
public:
  bool open(const std::string& path, unsigned width, unsigned height, unsigned fps) override
  {
    if (width == 0 || height == 0 || fps == 0) return false;
    out_.open(path, std::ios::binary | std::ios::trunc);
    if (!out_) return false;

    width_ = width;
    height_ = height;
    fps_ = fps;
    frames_ = 0;
    index_.clear();

    std::string header;
    header.append("RIFF");
    appendLE32(header, 0);  // patched in close()
    header.append("AVI ");

    header.append("LIST");
    appendLE32(header, HDRL_TOTAL - 8);
    header.append("hdrl");

    // ---- main AVI header ----
    header.append("avih");
    appendLE32(header, 56);
    appendLE32(header, 1000000 / fps);  // dwMicroSecPerFrame
    appendLE32(header, 0);              // dwMaxBytesPerSec
    appendLE32(header, 0);              // dwPaddingGranularity
    appendLE32(header, 0x10);           // dwFlags: AVIF_HASINDEX
    appendLE32(header, 0);              // dwTotalFrames, patched in close()
    appendLE32(header, 0);              // dwInitialFrames
    appendLE32(header, 1);              // dwStreams
    appendLE32(header, 0);              // dwSuggestedBufferSize
    appendLE32(header, width);          // dwWidth
    appendLE32(header, height);         // dwHeight
    header.append(16, '\0');            // dwReserved[4]

    // ---- video stream ----
    header.append("LIST");
    appendLE32(header, 4 + (8 + 56) + (8 + 40));
    header.append("strl");

    header.append("strh");
    appendLE32(header, 56);
    header.append("vids");                              // fccType
    header.append("MJPG");                              // fccHandler
    appendLE32(header, 0);                              // dwFlags
    appendLE16(header, 0);                              // wPriority
    appendLE16(header, 0);                              // wLanguage
    appendLE32(header, 0);                              // dwInitialFrames
    appendLE32(header, 1);                              // dwScale
    appendLE32(header, fps);                            // dwRate: fps = dwRate / dwScale
    appendLE32(header, 0);                              // dwStart
    appendLE32(header, 0);                              // dwLength, patched in close()
    appendLE32(header, 0);                              // dwSuggestedBufferSize
    appendLE32(header, 0);                              // dwQuality
    appendLE32(header, 0);                              // dwSampleSize
    appendLE16(header, 0);                              // rcFrame.left
    appendLE16(header, 0);                              // rcFrame.top
    appendLE16(header, static_cast<uint16_t>(width));   // rcFrame.right
    appendLE16(header, static_cast<uint16_t>(height));  // rcFrame.bottom

    header.append("strf");
    appendLE32(header, 40);
    appendLE32(header, 40);                  // biSize
    appendLE32(header, width);               // biWidth
    appendLE32(header, height);              // biHeight
    appendLE16(header, 1);                   // biPlanes
    appendLE16(header, 24);                  // biBitCount
    header.append("MJPG");                   // biCompression
    appendLE32(header, width * height * 3);  // biSizeImage
    appendLE32(header, 0);                   // biXPelsPerMeter
    appendLE32(header, 0);                   // biYPelsPerMeter
    appendLE32(header, 0);                   // biClrUsed
    appendLE32(header, 0);                   // biClrImportant

    header.append("LIST");
    appendLE32(header, 0);  // movi size, patched in close()
    header.append("movi");

    // Index offsets are relative to the 'movi' fourcc, which is the last thing above.
    moviFourccOffset_ = header.size() - 4;
    if (moviFourccOffset_ != OFFSET_MOVI_FOURCC) return false;  // layout drifted
    out_.write(header.data(), static_cast<std::streamsize>(header.size()));
    return out_.good();
  }

  bool addFrame(const uint8_t *rgba, std::size_t stride) override
  {
    if (!out_.is_open() || rgba == nullptr) return false;

    // stb wants tightly-packed rows; the caller's buffer may be padded.
    const size_t rowBytes = static_cast<size_t>(width_) * 4;
    std::vector<uint8_t> packed(rowBytes * height_);
    for (unsigned y = 0; y < height_; ++y) {
      std::memcpy(packed.data() + y * rowBytes, rgba + y * stride, rowBytes);
    }

    std::string jpeg;
    const auto sink = [](void *context, void *data, int size) {
      static_cast<std::string *>(context)->append(static_cast<const char *>(data), size);
    };
    if (stbi_write_jpg_to_func(sink, &jpeg, static_cast<int>(width_), static_cast<int>(height_), 4,
                               packed.data(), JPEG_QUALITY) == 0) {
      return false;
    }

    const auto chunkOffset =
      static_cast<uint32_t>(static_cast<size_t>(out_.tellp()) - moviFourccOffset_);

    std::string chunk;
    chunk.append("00dc");
    appendLE32(chunk, static_cast<uint32_t>(jpeg.size()));
    out_.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    out_.write(jpeg.data(), static_cast<std::streamsize>(jpeg.size()));
    // RIFF chunks are word-aligned.
    if (jpeg.size() & 1) out_.put('\0');

    index_.push_back({chunkOffset, static_cast<uint32_t>(jpeg.size())});
    ++frames_;
    return out_.good();
  }

  bool close() override
  {
    if (!out_.is_open()) return false;
    if (frames_ == 0) {
      out_.close();
      return false;
    }

    const auto moviEnd = static_cast<size_t>(out_.tellp());

    // ---- idx1 ----
    std::string idx;
    idx.append("idx1");
    appendLE32(idx, static_cast<uint32_t>(index_.size() * 16));
    for (const auto& entry : index_) {
      idx.append("00dc");
      appendLE32(idx, 0x10);  // AVIIF_KEYFRAME: every MJPEG frame is a keyframe
      appendLE32(idx, entry.offset);
      appendLE32(idx, entry.length);
    }
    out_.write(idx.data(), static_cast<std::streamsize>(idx.size()));

    const auto fileSize = static_cast<size_t>(out_.tellp());

    // ---- patch the sizes and counts that were unknown at open() ----
    patchLE32(OFFSET_RIFF_SIZE, static_cast<uint32_t>(fileSize - 8));
    patchLE32(OFFSET_AVIH_TOTAL_FRAMES, frames_);
    patchLE32(OFFSET_STRH_LENGTH, frames_);
    patchLE32(moviFourccOffset_ - 4,
              static_cast<uint32_t>(moviEnd - moviFourccOffset_));  // movi list size

    out_.close();
    return out_.good();
  }

private:
  struct IndexEntry {
    uint32_t offset;  //!< relative to the 'movi' fourcc
    uint32_t length;
  };

  void patchLE32(size_t offset, uint32_t value)
  {
    std::string bytes;
    appendLE32(bytes, value);
    out_.seekp(static_cast<std::streamoff>(offset));
    out_.write(bytes.data(), 4);
  }

  std::ofstream out_;
  unsigned width_ = 0;
  unsigned height_ = 0;
  unsigned fps_ = 0;
  uint32_t frames_ = 0;
  size_t moviFourccOffset_ = 0;
  std::vector<IndexEntry> index_;
};

}  // namespace

std::unique_ptr<VideoEncoder> createAviEncoder()
{
  return std::make_unique<AviEncoder>();
}
