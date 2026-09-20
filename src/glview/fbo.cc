#include "glview/fbo.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "glview/system-gl.h"

namespace {

bool checkFBOStatus() {
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  const char *statusString = nullptr;
  switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
      return true;
    case GL_FRAMEBUFFER_UNDEFINED:
      statusString = "GL_FRAMEBUFFER_UNDEFINED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      statusString = "GL_FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
      statusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
      break;
    default:
      break;
  }

  std::cerr << "glCheckFramebufferStatus(): ";
  if (statusString) std::cerr << statusString;
  else std::cerr << "Unknown status " << status;
  std::cerr << std::endl;
  return false;
}

}  // namespace

std::unique_ptr<FBO> createFBO(int width, int height) {
  if (hasGLVersion3() || hasGLESVersion2() || hasGLExtension(ARB_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ false);
  } else if (hasGLExtension(EXT_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ true);
  } else {
    std::cerr << "Framebuffer Objects not supported" << std::endl;
    return nullptr;
  }
}



FBO::FBO(int width, int height, bool useEXT) : width_(width), height_(height), use_ext_(useEXT) {
  // Generate and bind FBO
  GL_CHECK(glGenFramebuffers(1, &this->fbo_id_));
  this->bind();

  // Generate depth and render buffers
  GL_CHECK(glGenRenderbuffers(1, &this->depthbuf_id_));
  GL_CHECK(glGenRenderbuffers(1, &this->renderbuf_id_));

  // Create buffers with correct size
  if (!this->resize(width, height)) return;

  // Attach render and depth buffers
  GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_RENDERBUFFER, this->renderbuf_id_));

  if (!checkFBOStatus()) {
    std::cerr << "Problem with OpenGL framebuffer after specifying color render buffer.\n";
    return;
  }

  // to prevent Mesa's software renderer from crashing, do this in two stages.
  // ie. instead of using GL_DEPTH_STENCIL_ATTACHMENT, do DEPTH then STENCIL.
  GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                     GL_RENDERBUFFER, this->depthbuf_id_));
  GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                     GL_RENDERBUFFER, this->depthbuf_id_));

  if (!checkFBOStatus()) {
    std::cerr << "Problem with OpenGL framebuffer after specifying depth render buffer.\n";
    return;
  }

  this->complete_ = true;
}

bool FBO::resize(size_t width, size_t height) {
  if (this->use_ext_) {
    GL_CHECK(glBindRenderbufferEXT(GL_RENDERBUFFER, this->renderbuf_id_));
  } else {
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, this->renderbuf_id_));
  }
  GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height));
  if (this->use_ext_) {
    GL_CHECK(glBindRenderbufferEXT(GL_RENDERBUFFER, this->depthbuf_id_));
  } else {
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, this->depthbuf_id_));
  }
  GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));

  this->width_ = width;
  this->height_ = height;

  return true;
}

GLuint FBO::bind() {
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&this->old_fbo_id_));
  if (this->use_ext_) {
    GL_CHECK(glBindFramebufferEXT(GL_FRAMEBUFFER, this->fbo_id_));
  } else {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id_));
  }
  return this->old_fbo_id_;
}

void FBO::unbind() {
  if (this->use_ext_) {
    GL_CHECK(glBindFramebufferEXT(GL_FRAMEBUFFER, this->old_fbo_id_));
  } else {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, this->old_fbo_id_));
  }
  this->old_fbo_id_ = 0;
}

void FBO::destroy() {
  this->unbind();
  if (this->depthbuf_id_ != 0) {
    GL_CHECK(glDeleteRenderbuffers(1, &this->depthbuf_id_));
    this->depthbuf_id_ = 0;
  }
  if (this->renderbuf_id_ != 0) {
    GL_CHECK(glDeleteRenderbuffers(1, &this->renderbuf_id_));
    this->renderbuf_id_ = 0;
  }
  if (this->fbo_id_ != 0) {
    GL_CHECK(glDeleteFramebuffers(1, &this->fbo_id_));
    this->fbo_id_ = 0;
  }
}

