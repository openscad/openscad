#include "OpenGLContext.h"

#include "system-gl.h"

std::vector<uint8_t> OpenGLContext::getFramebuffer() const
{
  const uint8_t samplesPerPixel = 4; // R, G, B and A
  const size_t rowBytes = samplesPerPixel * this->width_;
  std::vector<uint8_t> buffer(rowBytes * this->height_);
  GL_CHECK(glReadPixels(0, 0, this->width_, this->height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data()));
  return buffer;
}
