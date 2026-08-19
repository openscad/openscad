#include "io/VideoEncoder.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lodepng/lodepng.h"

// Defined by the per-container implementations alongside this file.
std::unique_ptr<VideoEncoder> createGifEncoder();
std::unique_ptr<VideoEncoder> createApngEncoder();
std::unique_ptr<VideoEncoder> createAviEncoder();

bool VideoEncoder::addPngFrame(const uint8_t *png, std::size_t size)
{
  std::vector<unsigned char> rgba;
  unsigned width = 0, height = 0;
  if (lodepng::decode(rgba, width, height, png, size) != 0) return false;
  return addFrame(rgba.data(), static_cast<std::size_t>(width) * 4);
}

std::unique_ptr<VideoEncoder> VideoEncoder::create(const std::string& suffix)
{
  if (suffix == "gif") return createGifEncoder();
  if (suffix == "apng") return createApngEncoder();
  if (suffix == "avi") return createAviEncoder();
  return nullptr;
}

namespace {

/*!
   Offset of the suffix's dot in `path`, or npos when the final component has none.
   A dot in a directory name does not count.
 */
size_t suffixDot(const std::string& path)
{
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos) return std::string::npos;
  const auto separator = path.find_last_of("/\\");
  if (separator != std::string::npos && dot < separator) return std::string::npos;
  return dot;
}

}  // namespace

std::string outputSuffix(const std::string& path)
{
  const auto dot = suffixDot(path);
  if (dot == std::string::npos) return {};

  std::string suffix = path.substr(dot + 1);
  std::transform(suffix.begin(), suffix.end(), suffix.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return suffix;
}

std::string numberedFramePath(const std::string& path, unsigned frame)
{
  std::ostringstream number;
  number << std::setw(5) << std::setfill('0') << frame;

  const auto dot = suffixDot(path);
  if (dot == std::string::npos) return path + number.str();
  return path.substr(0, dot) + number.str() + path.substr(dot);
}
