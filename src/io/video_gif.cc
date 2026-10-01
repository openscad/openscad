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
#include "utils/printutils.h"

namespace {

/*
   Dithering is on. A shaded 3D render is mostly smooth gradients, and GIF's 256
   colors band badly across them; the cost is a second pass over each frame, which is
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

    /*
       GIF frame delays are whole centiseconds, so most rates cannot be represented
       exactly and anything above 100fps would round to zero. Round to nearest rather
       than truncating: at 15fps truncation gives 6cs (16.7fps, 11% fast) where
       rounding gives 7cs (14.3fps, 4.8% slow), and at 60fps truncation gives 1cs --
       100fps, nearly double speed.
     */
    delay_ = (100 + fps / 2) / fps;
    if (delay_ == 0) return false;

    /*
       Say so when the requested rate is not representable, rather than silently
       producing an animation that plays at a different speed. 30fps is the common
       case: 100/30 is 3.33, and neither 3 (33.3fps) nor 4 (25fps) is 30. Rates that
       divide 100 -- 10, 20, 25, 50 -- are exact.
     */
    if (100 % fps != 0) {
      LOG(message_group::Warning,
          "GIF frame delays are whole centiseconds; %1$d fps is not representable, "
          "so this file plays at %2$.1f fps. Use 10, 20, 25 or 50 fps for exact timing.",
          fps, 100.0 / delay_);
    }

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
