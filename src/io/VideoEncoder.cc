#include "io/VideoEncoder.h"

#include <memory>
#include <string>

// Defined by the per-container implementations alongside this file.
std::unique_ptr<VideoEncoder> createGifEncoder();
std::unique_ptr<VideoEncoder> createApngEncoder();
std::unique_ptr<VideoEncoder> createAviEncoder();

std::unique_ptr<VideoEncoder> VideoEncoder::create(const std::string& suffix)
{
  if (suffix == "gif") return createGifEncoder();
  if (suffix == "apng") return createApngEncoder();
  if (suffix == "avi") return createAviEncoder();
  return nullptr;
}
