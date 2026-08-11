/*
   Animated GIF encoder.

   Wraps the vendored gif.h (Charlie Tangora, public domain), which does the part that
   is genuinely fiddly: a median-cut palette per frame, optional Floyd-Steinberg
   dithering, and LZW. It writes through a FILE*, which is why VideoEncoder::open
   takes a path rather than a std::ostream.
 */

#include "io/VideoEncoder.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "gif-h/gif.h"

namespace {

/*
   Dithering is on. A shaded 3D render is mostly smooth gradients, and GIF's 256
   colours band badly across them; the cost is a second pass over each frame, which is
   nothing next to rendering it.
 */
constexpr bool DITHER = true;

class GifEncoder : public VideoEncoder
{
public:
  ~GifEncoder() override
  {
    if (open_) GifEnd(&writer_);
  }

  bool open(const std::string& path, unsigned width, unsigned height, unsigned fps) override
  {
    if (width == 0 || height == 0 || fps == 0) return false;

    // GIF frame delays are in centiseconds, so anything above 100fps rounds to zero.
    delay_ = 100 / fps;
    if (delay_ == 0) return false;

    if (!GifBegin(&writer_, path.c_str(), width, height, delay_, 8, DITHER)) return false;

    width_ = width;
    height_ = height;
    open_ = true;
    return true;
  }

  bool addFrame(const uint8_t *rgba, std::size_t stride) override
  {
    if (!open_ || rgba == nullptr) return false;

    // gif.h wants tightly-packed rows; the caller's buffer may be padded.
    const size_t rowBytes = static_cast<size_t>(width_) * 4;
    std::vector<uint8_t> packed(rowBytes * height_);
    for (unsigned y = 0; y < height_; ++y) {
      std::memcpy(packed.data() + y * rowBytes, rgba + y * stride, rowBytes);
    }

    return GifWriteFrame(&writer_, packed.data(), width_, height_, delay_, 8, DITHER);
  }

  bool close() override
  {
    if (!open_) return false;
    open_ = false;
    return GifEnd(&writer_);
  }

private:
  GifWriter writer_{};
  unsigned width_ = 0;
  unsigned height_ = 0;
  uint32_t delay_ = 0;
  bool open_ = false;
};

}  // namespace

std::unique_ptr<VideoEncoder> createGifEncoder()
{
  return std::make_unique<GifEncoder>();
}
