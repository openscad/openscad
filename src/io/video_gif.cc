/*
   Animated GIF encoder.

   Intended to sit on a vendored single-header GIF writer rather than an external
   giflib dependency. The exact library, upstream URL and licence must be verified
   before it is added to src/ext/ — see the feature page.
 */

#include "io/VideoEncoder.h"

#include <memory>

namespace {

class GifEncoder : public VideoEncoder
{
public:
  bool open(std::ostream& /*out*/, unsigned /*width*/, unsigned /*height*/,
            unsigned /*fps*/) override
  {
    return false;  // ponytail: stub — tests are written first, see video_encoder_test.cc
  }

  bool addFrame(const uint8_t * /*rgba*/, std::size_t /*stride*/) override { return false; }

  bool close() override { return false; }
};

}  // namespace

std::unique_ptr<VideoEncoder> createGifEncoder()
{
  return std::make_unique<GifEncoder>();
}
