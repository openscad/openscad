/*
   Motion-JPEG in an AVI container.

   The point of this backend is a lightly-compressed intermediate that transcoders and
   NLEs read without argument -- not a deliverable file. AVI is a RIFF tree: a 'hdrl'
   header list, a 'movi' list of one '00dc' chunk per JPEG frame, and an 'idx1' index
   whose entries must resolve to those chunks or players disagree about the file.

   Still open: where the JPEG encoder comes from. Nothing in the tree encodes JPEG
   (only Qt does, which this file deliberately cannot use). See the feature page.
 */

#include "io/VideoEncoder.h"

#include <memory>

namespace {

class AviEncoder : public VideoEncoder
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

std::unique_ptr<VideoEncoder> createAviEncoder()
{
  return std::make_unique<AviEncoder>();
}
