#pragma once

#include <vector>
#include <string>
#include <cstdint>

class OpenGLContext {
protected:
  uint32_t width_;
  uint32_t height_;

public:
  OpenGLContext(uint32_t width, uint32_t height) : width_(width), height_(height) {}
  virtual ~OpenGLContext() = default;
  uint32_t width() const { return this->width_; }
  uint32_t height() const { return this->height_; }
  virtual bool makeCurrent() const = 0;
  virtual std::string getInfo() const = 0;
  std::vector<uint8_t> getFramebuffer() const;
};
