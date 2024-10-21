#include "glview/OpenGLContext.h"

#include <cstdint>
#include <cstddef>
#include <vector>

#include "glview/system-gl.h"

std::vector<uint8_t> OpenGLContext::getFramebuffer() const
{
  const size_t samplesPerPixel = 4; // R, G, B and A
  std::vector<uint8_t> buffer(samplesPerPixel * this->width_ * this->height_);
  GL_CHECK(glReadPixels(0, 0, this->width_, this->height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data()));
  return buffer;
}
