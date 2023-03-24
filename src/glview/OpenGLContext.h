#pragma once

#include <ostream>
#include <vector>

class OpenGLContext {
 protected:
  int width_;
  int height_;

 public:
  OpenGLContext(int width, int height) : width_(width), height_(height) {}
  virtual ~OpenGLContext() = default;
  int width() const { return this->width_; }
  int height() const { return this->height_; }
  virtual bool isOffscreen() const = 0;
  virtual bool makeCurrent() {return false;}
	virtual std::string getInfo() const = 0;
  std::vector<uint8_t> getFramebuffer() const;
};
