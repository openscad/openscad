#include "glview/fbo.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "glview/system-gl.h"
#include "utils/printutils.h"

namespace {

bool checkFBOStatus()
{
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  const char *statusString = nullptr;
  switch (status) {
  case GL_FRAMEBUFFER_COMPLETE:  return true;
  case GL_FRAMEBUFFER_UNDEFINED: statusString = "GL_FRAMEBUFFER_UNDEFINED"; break;
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
  case GL_FRAMEBUFFER_UNSUPPORTED: statusString = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
    statusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
    statusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
    break;
  default: break;
  }

  LOG(message_group::Error, "glCheckFramebufferStatus(): %1$s",
      statusString ? statusString : "Unknown status " + std::to_string(status));
  return false;
}

}  // namespace

std::unique_ptr<FBO> createFBO(int width, int height)
{
  if (hasGLVersion3() || hasGLESVersion2() || hasGLExtension(ARB_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ false);
  } else if (hasGLExtension(EXT_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ true);
  } else {
    LOG(message_group::Error, "Framebuffer Objects not supported");
    return nullptr;
  }
}

FBO::FBO(int width, int height, bool useEXT) : width_(width), height_(height), use_ext_(useEXT)
{
  // Generate and bind FBO
  GL_CHECK(glGenFramebuffers(1, &fbo_id_));
  bind();

  // Generate depth and render buffers
  GL_CHECK(glGenRenderbuffers(1, &depthbuf_id_));
  GL_CHECK(glGenRenderbuffers(1, &renderbuf_id_));

  // Create buffers with correct size
  if (!resize(width, height)) return;

  // Attach render and depth buffers
  GL_CHECK(
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuf_id_));

  if (!checkFBOStatus()) {
    LOG(message_group::Error, "Problem with OpenGL framebuffer after specifying color render buffer.");
    return;
  }

  // to prevent Mesa's software renderer from crashing, do this in two stages.
  // ie. instead of using GL_DEPTH_STENCIL_ATTACHMENT, do DEPTH then STENCIL.
  GL_CHECK(
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuf_id_));
  GL_CHECK(
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthbuf_id_));

  if (!checkFBOStatus()) {
    LOG(message_group::Error, "Problem with OpenGL framebuffer after specifying depth render buffer.");
    return;
  }

  complete_ = true;
}

bool FBO::resize(size_t width, size_t height)
{
  if (use_ext_) {
    GL_CHECK(glBindRenderbufferEXT(GL_RENDERBUFFER, renderbuf_id_));
  } else {
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, renderbuf_id_));
  }
  GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height));
  if (use_ext_) {
    GL_CHECK(glBindRenderbufferEXT(GL_RENDERBUFFER, depthbuf_id_));
  } else {
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, depthbuf_id_));
  }
  GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));

  width_ = width;
  height_ = height;

  return true;
}

GLuint FBO::bind()
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&old_fbo_id_));
  if (use_ext_) {
    GL_CHECK(glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_id_));
  } else {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_));
  }
  return old_fbo_id_;
}

void FBO::unbind()
{
  if (use_ext_) {
    GL_CHECK(glBindFramebufferEXT(GL_FRAMEBUFFER, old_fbo_id_));
  } else {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, old_fbo_id_));
  }
  old_fbo_id_ = 0;
}

void FBO::destroy()
{
  unbind();
  if (depthbuf_id_ != 0) {
    GL_CHECK(glDeleteRenderbuffers(1, &depthbuf_id_));
    depthbuf_id_ = 0;
  }
  if (renderbuf_id_ != 0) {
    GL_CHECK(glDeleteRenderbuffers(1, &renderbuf_id_));
    renderbuf_id_ = 0;
  }
  if (fbo_id_ != 0) {
    GL_CHECK(glDeleteFramebuffers(1, &fbo_id_));
    fbo_id_ = 0;
  }
}
