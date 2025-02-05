#pragma once

#include <cstddef>
#include <memory>

#include "system-gl.h"
#include "OpenGLContext.h"

class FBO
{
  bool useEXT;
  GLuint fbo_id = 0;
  GLuint old_fbo_id = 0;
  GLuint renderbuf_id = 0;
  GLuint depthbuf_id = 0;
  bool complete = false;

public:
  FBO(int width, int height, bool useEXT);
  ~FBO() { destroy(); };
  bool isComplete() { return this->complete; }
  bool resize(size_t width, size_t height);
  GLuint bind();
  void unbind();
  void destroy();
};

std::unique_ptr<FBO> createFBO(int width, int height);
