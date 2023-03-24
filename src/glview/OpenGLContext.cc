#include "OpenGLContext.h"

#include <iostream>

#include "system-gl.h"

std::vector<uint8_t> OpenGLContext::getFramebuffer() const
{
  int samplesPerPixel = 4; // R, G, B and A
  int rowBytes = samplesPerPixel * this->width_;
  std::vector<uint8_t> buffer(rowBytes * this->height_);
  GL_CHECK(glReadPixels(0, 0, this->width_, this->height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data()));
  return std::move(buffer);
}
