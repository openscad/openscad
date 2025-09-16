#pragma once

#include <cstddef>
#include <memory>

#include "glview/system-gl.h"

class FBO
{
public:
  FBO(int width, int height, bool useEXT);
  ~FBO() { destroy(); };

  int width() const { return this->width_; }
  int height() const { return this->height_; }
  bool isComplete() const { return this->complete_; }

  bool resize(size_t width, size_t height);
  GLuint bind();
  void unbind();

private:
  void destroy();

  int width_ = 0;
  int height_ = 0;
  bool use_ext_;
  GLuint fbo_id_ = 0;
  GLuint old_fbo_id_ = 0;
  GLuint renderbuf_id_ = 0;
  GLuint depthbuf_id_ = 0;
  bool complete_ = false;
};

std::unique_ptr<FBO> createFBO(int width, int height);
