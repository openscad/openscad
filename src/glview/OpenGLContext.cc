#include "glview/OpenGLContext.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "glview/system-gl.h"

std::vector<uint8_t> OpenGLContext::getFramebuffer() const
{
  const size_t samplesPerPixel = 4;  // R, G, B and A
  std::vector<uint8_t> buffer(samplesPerPixel * width_ * height_);
  GL_CHECK(glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data()));
  return buffer;
}
