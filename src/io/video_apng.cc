/*
   Animated PNG encoder.

   APNG is an ordinary PNG plus three chunk types: acTL (animation control), fcTL
   (per-frame control) and fdAT (frame data after the first). The first frame is
   carried in a plain IDAT so a non-APNG decoder still shows a valid still image.
   Built on the already-vendored lodepng, so this costs no new dependency.
 */

#include "io/VideoEncoder.h"

#include <memory>

namespace {

class ApngEncoder : public VideoEncoder
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

std::unique_ptr<VideoEncoder> createApngEncoder()
{
  return std::make_unique<ApngEncoder>();
}
