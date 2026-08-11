/*
   Tests for the animation container encoders (APNG, GIF, MJPEG/AVI).

   These run headless: the encoders take raw RGBA and write to a std::ostream, so no
   GL context, no window and no event loop are involved. They assert on container
   *structure* parsed back out of the produced bytes rather than comparing against
   golden files -- a golden would break on any zlib or JPEG library version bump and
   would teach us to re-bless it rather than to read it.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "io/VideoEncoder.h"

namespace {

constexpr unsigned WIDTH = 4;
constexpr unsigned HEIGHT = 4;
constexpr unsigned FPS = 10;
constexpr unsigned FRAMES = 3;

//! One solid-colour RGBA frame, tightly packed.
std::vector<uint8_t> solidFrame(uint8_t r, uint8_t g, uint8_t b)
{
  std::vector<uint8_t> px(static_cast<size_t>(WIDTH) * HEIGHT * 4);
  for (size_t i = 0; i < px.size(); i += 4) {
    px[i + 0] = r;
    px[i + 1] = g;
    px[i + 2] = b;
    px[i + 3] = 255;
  }
  return px;
}

/*!
   Encodes three distinct solid frames through the encoder registered for `suffix`
   and returns the raw container bytes.
 */
std::string encodeThreeFrames(const std::string& suffix)
{
  auto encoder = VideoEncoder::create(suffix);
  REQUIRE(encoder != nullptr);

  std::ostringstream out(std::ios::binary);
  REQUIRE(encoder->open(out, WIDTH, HEIGHT, FPS));

  const std::vector<std::vector<uint8_t>> frames = {
    solidFrame(255, 0, 0),
    solidFrame(0, 255, 0),
    solidFrame(0, 0, 255),
  };
  for (const auto& frame : frames) {
    REQUIRE(encoder->addFrame(frame.data(), static_cast<size_t>(WIDTH) * 4));
  }

  REQUIRE(encoder->close());
  return out.str();
}

uint32_t readBE32(const std::string& d, size_t off)
{
  return (static_cast<uint8_t>(d[off + 0]) << 24) | (static_cast<uint8_t>(d[off + 1]) << 16) |
         (static_cast<uint8_t>(d[off + 2]) << 8) | static_cast<uint8_t>(d[off + 3]);
}

uint32_t readLE32(const std::string& d, size_t off)
{
  return static_cast<uint8_t>(d[off + 0]) | (static_cast<uint8_t>(d[off + 1]) << 8) |
         (static_cast<uint8_t>(d[off + 2]) << 16) | (static_cast<uint8_t>(d[off + 3]) << 24);
}

uint16_t readBE16(const std::string& d, size_t off)
{
  return static_cast<uint16_t>((static_cast<uint8_t>(d[off]) << 8) |
                               static_cast<uint8_t>(d[off + 1]));
}

uint16_t readLE16(const std::string& d, size_t off)
{
  return static_cast<uint16_t>(static_cast<uint8_t>(d[off]) |
                               (static_cast<uint8_t>(d[off + 1]) << 8));
}

std::string fourcc(const std::string& d, size_t off) { return d.substr(off, 4); }

/*
   ---- PNG / APNG ----------------------------------------------------------------
 */

//! Self-contained CRC32 so the test does not depend on how lodepng was configured.
uint32_t crc32(const uint8_t *data, size_t len)
{
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xedb88320u & (~(crc & 1) + 1));
    }
  }
  return ~crc;
}

struct PngChunk {
  std::string type;
  size_t dataOffset;
  uint32_t length;
};

//! Walks the chunk list, verifying each chunk's declared CRC as it goes.
std::vector<PngChunk> parsePngChunks(const std::string& d)
{
  static const std::string signature("\x89PNG\r\n\x1a\n", 8);
  REQUIRE(d.size() > signature.size());
  REQUIRE(d.compare(0, signature.size(), signature) == 0);

  std::vector<PngChunk> chunks;
  size_t pos = signature.size();
  while (pos + 12 <= d.size()) {
    const uint32_t length = readBE32(d, pos);
    REQUIRE(pos + 12 + length <= d.size());

    const PngChunk chunk{d.substr(pos + 4, 4), pos + 8, length};

    // The CRC covers the type field and the data, but not the length field.
    const auto *crcStart = reinterpret_cast<const uint8_t *>(d.data()) + pos + 4;
    INFO("chunk " << chunk.type << " at offset " << pos);
    REQUIRE(crc32(crcStart, 4 + length) == readBE32(d, pos + 8 + length));

    chunks.push_back(chunk);
    pos += 12 + length;
    if (chunk.type == "IEND") break;
  }
  REQUIRE(pos == d.size());
  return chunks;
}

/*
   ---- RIFF / AVI ----------------------------------------------------------------
 */

struct RiffChunk {
  std::string id;        //!< "LIST", "avih", "00dc", ...
  std::string listType;  //!< for LIST/RIFF: "hdrl", "movi", ...; empty otherwise
  size_t headerOffset;   //!< offset of the id field
  size_t dataOffset;     //!< offset of the payload (past listType for lists)
  uint32_t size;         //!< declared payload size, not counting the 8-byte header
};

//! Immediate children of a RIFF range, without recursing into nested lists.
std::vector<RiffChunk> riffChildren(const std::string& d, size_t begin, size_t end)
{
  std::vector<RiffChunk> chunks;
  size_t pos = begin;
  while (pos + 8 <= end) {
    RiffChunk chunk;
    chunk.headerOffset = pos;
    chunk.id = fourcc(d, pos);
    chunk.size = readLE32(d, pos + 4);
    REQUIRE(pos + 8 + chunk.size <= d.size());

    if (chunk.id == "LIST" || chunk.id == "RIFF") {
      chunk.listType = fourcc(d, pos + 8);
      chunk.dataOffset = pos + 12;
    } else {
      chunk.dataOffset = pos + 8;
    }
    chunks.push_back(chunk);

    // RIFF chunks are word-aligned; an odd size is followed by a pad byte.
    pos += 8 + chunk.size + (chunk.size & 1);
  }
  return chunks;
}

const RiffChunk *findChunk(const std::vector<RiffChunk>& chunks, const std::string& id,
                           const std::string& listType = "")
{
  for (const auto& chunk : chunks) {
    if (chunk.id == id && (listType.empty() || chunk.listType == listType)) return &chunk;
  }
  return nullptr;
}

/*
   ---- GIF -----------------------------------------------------------------------
 */

struct GifParse {
  unsigned width = 0;
  unsigned height = 0;
  bool hasNetscapeLoop = false;
  unsigned imageDescriptors = 0;
  std::vector<unsigned> delaysCentiseconds;
  bool sawTrailer = false;
};

//! Skips a GIF sub-block chain, returning the offset just past the terminating 0x00.
size_t skipSubBlocks(const std::string& d, size_t pos)
{
  while (pos < d.size()) {
    const auto blockSize = static_cast<uint8_t>(d[pos]);
    pos += 1 + blockSize;
    if (blockSize == 0) break;
  }
  return pos;
}

GifParse parseGif(const std::string& d)
{
  GifParse parsed;
  REQUIRE(d.size() > 13);
  REQUIRE(d.compare(0, 6, "GIF89a") == 0);

  parsed.width = readLE16(d, 6);
  parsed.height = readLE16(d, 8);

  const auto packed = static_cast<uint8_t>(d[10]);
  size_t pos = 13;
  if (packed & 0x80) pos += 3u << ((packed & 0x07) + 1);  // global colour table

  while (pos < d.size()) {
    const auto introducer = static_cast<uint8_t>(d[pos]);
    if (introducer == 0x3b) {  // trailer
      parsed.sawTrailer = true;
      pos += 1;
      break;
    }
    if (introducer == 0x21) {  // extension
      const auto label = static_cast<uint8_t>(d[pos + 1]);
      if (label == 0xf9) {  // graphic control extension
        parsed.delaysCentiseconds.push_back(readLE16(d, pos + 4));
      } else if (label == 0xff && d.compare(pos + 3, 11, "NETSCAPE2.0") == 0) {
        parsed.hasNetscapeLoop = true;
      }
      pos = skipSubBlocks(d, pos + 2);
      continue;
    }
    if (introducer == 0x2c) {  // image descriptor
      ++parsed.imageDescriptors;
      const auto localPacked = static_cast<uint8_t>(d[pos + 9]);
      pos += 10;
      if (localPacked & 0x80) pos += 3u << ((localPacked & 0x07) + 1);
      pos += 1;  // LZW minimum code size
      pos = skipSubBlocks(d, pos);
      continue;
    }
    FAIL("unexpected GIF block introducer 0x" << std::hex << +introducer);
  }
  REQUIRE(pos == d.size());
  return parsed;
}

}  // namespace

TEST_CASE("VideoEncoder factory maps suffixes to encoders", "[video]")
{
  CHECK(VideoEncoder::create("gif") != nullptr);
  CHECK(VideoEncoder::create("apng") != nullptr);
  CHECK(VideoEncoder::create("avi") != nullptr);

  // Unknown suffixes must return null rather than crash; the caller reports an error.
  CHECK(VideoEncoder::create("") == nullptr);
  CHECK(VideoEncoder::create("webm") == nullptr);
  CHECK(VideoEncoder::create("GIF") == nullptr);  // create() takes a lowercased suffix

  // Plain "png" stays the existing numbered still-frame dump, not an animation.
  CHECK(VideoEncoder::create("png") == nullptr);

  // mp4/mov are handed to the platform encoder, which needs Qt and is not built here.
  CHECK(VideoEncoder::create("mp4") == nullptr);
}

TEST_CASE("APNG output is a structurally valid animated PNG", "[video]")
{
  const std::string data = encodeThreeFrames("apng");
  const std::vector<PngChunk> chunks = parsePngChunks(data);  // also verifies every CRC

  REQUIRE(chunks.size() >= 5);
  CHECK(chunks.front().type == "IHDR");
  CHECK(chunks.back().type == "IEND");

  CHECK(readBE32(data, chunks.front().dataOffset + 0) == WIDTH);
  CHECK(readBE32(data, chunks.front().dataOffset + 4) == HEIGHT);

  unsigned actl = 0, fctl = 0, fdat = 0, idat = 0;
  std::vector<uint32_t> sequenceNumbers;
  const PngChunk *animationControl = nullptr;

  for (const auto& chunk : chunks) {
    if (chunk.type == "acTL") {
      ++actl;
      animationControl = &chunk;
    } else if (chunk.type == "fcTL") {
      ++fctl;
      sequenceNumbers.push_back(readBE32(data, chunk.dataOffset));
      // Delay is a rational: FPS 10 must come out as some n/d equal to 1/10.
      const uint16_t delayNum = readBE16(data, chunk.dataOffset + 20);
      const uint16_t delayDen = readBE16(data, chunk.dataOffset + 22);
      REQUIRE(delayDen != 0);
      CHECK(static_cast<double>(delayNum) / delayDen == 1.0 / FPS);
    } else if (chunk.type == "fdAT") {
      ++fdat;
      sequenceNumbers.push_back(readBE32(data, chunk.dataOffset));
    } else if (chunk.type == "IDAT") {
      ++idat;
    }
  }

  // Exactly one animation control, declaring the real frame count.
  REQUIRE(actl == 1);
  CHECK(readBE32(data, animationControl->dataOffset) == FRAMES);  // num_frames

  // One fcTL per frame; the first frame rides in IDAT, so the rest are fdAT.
  CHECK(fctl == FRAMES);
  CHECK(fdat == FRAMES - 1);
  CHECK(idat >= 1);

  // acTL must precede the first fcTL/IDAT, and the first fcTL must precede IDAT.
  const auto typeIndex = [&chunks](const std::string& type) {
    for (size_t i = 0; i < chunks.size(); ++i) {
      if (chunks[i].type == type) return i;
    }
    FAIL("missing chunk " << type);
    return size_t{0};
  };
  CHECK(typeIndex("acTL") < typeIndex("fcTL"));
  CHECK(typeIndex("fcTL") < typeIndex("IDAT"));

  // Sequence numbers are shared across fcTL and fdAT: gapless, starting at zero.
  REQUIRE(sequenceNumbers.size() == FRAMES + (FRAMES - 1));
  for (size_t i = 0; i < sequenceNumbers.size(); ++i) {
    CHECK(sequenceNumbers[i] == i);
  }
}

TEST_CASE("GIF output is a structurally valid animated GIF", "[video]")
{
  const std::string data = encodeThreeFrames("gif");
  const GifParse parsed = parseGif(data);

  CHECK(parsed.width == WIDTH);
  CHECK(parsed.height == HEIGHT);
  CHECK(parsed.imageDescriptors == FRAMES);
  CHECK(parsed.sawTrailer);

  // Without the NETSCAPE2.0 application extension the animation plays once and stops.
  CHECK(parsed.hasNetscapeLoop);

  REQUIRE(parsed.delaysCentiseconds.size() == FRAMES);
  for (const unsigned delay : parsed.delaysCentiseconds) {
    CHECK(delay == 100 / FPS);  // GIF delays are in centiseconds
  }
}

TEST_CASE("AVI output is a structurally valid MJPEG AVI", "[video]")
{
  const std::string data = encodeThreeFrames("avi");

  const std::vector<RiffChunk> top = riffChildren(data, 0, data.size());
  REQUIRE(top.size() == 1);
  REQUIRE(top[0].id == "RIFF");
  CHECK(top[0].listType == "AVI ");

  // A wrong top-level size is the classic truncation bug: it covers everything but
  // the 8-byte RIFF header itself.
  CHECK(top[0].size == data.size() - 8);

  const std::vector<RiffChunk> body = riffChildren(data, top[0].dataOffset, data.size());
  const RiffChunk *hdrl = findChunk(body, "LIST", "hdrl");
  const RiffChunk *movi = findChunk(body, "LIST", "movi");
  const RiffChunk *idx1 = findChunk(body, "idx1");
  REQUIRE(hdrl != nullptr);
  REQUIRE(movi != nullptr);
  REQUIRE(idx1 != nullptr);

  // ---- main AVI header ----
  const std::vector<RiffChunk> hdrlChildren =
    riffChildren(data, hdrl->dataOffset, hdrl->dataOffset + hdrl->size - 4);
  const RiffChunk *avih = findChunk(hdrlChildren, "avih");
  REQUIRE(avih != nullptr);
  CHECK(readLE32(data, avih->dataOffset + 0) == 1000000 / FPS);  // dwMicroSecPerFrame
  CHECK(readLE32(data, avih->dataOffset + 16) == FRAMES);        // dwTotalFrames
  CHECK(readLE32(data, avih->dataOffset + 32) == WIDTH);         // dwWidth
  CHECK(readLE32(data, avih->dataOffset + 36) == HEIGHT);        // dwHeight

  // ---- video stream header declares MJPEG ----
  const RiffChunk *strl = findChunk(hdrlChildren, "LIST", "strl");
  REQUIRE(strl != nullptr);
  const std::vector<RiffChunk> strlChildren =
    riffChildren(data, strl->dataOffset, strl->dataOffset + strl->size - 4);
  const RiffChunk *strh = findChunk(strlChildren, "strh");
  REQUIRE(strh != nullptr);
  CHECK(fourcc(data, strh->dataOffset + 0) == "vids");
  CHECK(fourcc(data, strh->dataOffset + 4) == "MJPG");
  REQUIRE(findChunk(strlChildren, "strf") != nullptr);

  // ---- one JPEG per frame ----
  const std::vector<RiffChunk> moviChildren =
    riffChildren(data, movi->dataOffset, movi->dataOffset + movi->size - 4);
  std::vector<const RiffChunk *> frames;
  for (const auto& chunk : moviChildren) {
    if (chunk.id == "00dc") frames.push_back(&chunk);
  }
  REQUIRE(frames.size() == FRAMES);
  for (const auto *frame : frames) {
    REQUIRE(frame->size >= 4);
    // JPEG start-of-image and end-of-image markers.
    CHECK(static_cast<uint8_t>(data[frame->dataOffset]) == 0xff);
    CHECK(static_cast<uint8_t>(data[frame->dataOffset + 1]) == 0xd8);
    CHECK(static_cast<uint8_t>(data[frame->dataOffset + frame->size - 2]) == 0xff);
    CHECK(static_cast<uint8_t>(data[frame->dataOffset + frame->size - 1]) == 0xd9);
  }

  /*
     ---- the index actually resolves ----
     An AVI with a bogus idx1 still opens in some players and fails in others, which
     makes it the worst kind of bug to ship. Offsets are relative to the position of
     the 'movi' fourcc (i.e. the LIST header + 8), per OpenDML.
   */
  const size_t moviBase = movi->headerOffset + 8;
  REQUIRE(idx1->size == FRAMES * 16);
  for (unsigned i = 0; i < FRAMES; ++i) {
    const size_t entry = idx1->dataOffset + i * 16;
    const uint32_t chunkOffset = readLE32(data, entry + 8);
    const uint32_t chunkLength = readLE32(data, entry + 12);

    INFO("idx1 entry " << i);
    CHECK(fourcc(data, entry + 0) == "00dc");
    REQUIRE(moviBase + chunkOffset + 8 <= data.size());
    CHECK(fourcc(data, moviBase + chunkOffset) == "00dc");
    CHECK(readLE32(data, moviBase + chunkOffset + 4) == chunkLength);
    CHECK(chunkLength == frames[i]->size);
  }
}
