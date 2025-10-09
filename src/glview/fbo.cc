#include "glview/fbo.h"

#include <memory>

#include "glview/system-gl.h"
#include "utils/printutils.h"

namespace {

bool checkFBOStatus()
{
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  const char *statusString = nullptr;
  switch (status) {
  case GL_FRAMEBUFFER_COMPLETE:  return true; break;
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
  if (hasGLExtension(ARB_framebuffer_object)) {
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
  GL_CHECKD(glGenFramebuffers(1, &this->fbo_id_));
  this->bind();

  // Generate depth and render buffers
  GL_CHECKD(glGenRenderbuffers(1, &this->depthbuf_id_));
  GL_CHECKD(glGenRenderbuffers(1, &this->renderbuf_id_));

  // Create buffers with correct size
  if (!this->resize(width, height)) return;

  // Attach render and depth buffers
  GL_CHECKD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                      this->renderbuf_id_));

  if (!checkFBOStatus()) {
    LOG(message_group::Error, "Problem with OpenGL framebuffer after specifying color render buffer.");
    return;
  }

  // to prevent Mesa's software renderer from crashing, do this in two stages.
  // ie. instead of using GL_DEPTH_STENCIL_ATTACHMENT, do DEPTH then STENCIL.
  GL_CHECKD(
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->depthbuf_id_));
  GL_CHECKD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      this->depthbuf_id_));

  if (!checkFBOStatus()) {
    LOG(message_group::Error, "Problem with OpenGL framebuffer after specifying depth render buffer.");
    return;
  }

  this->complete_ = true;
}

bool FBO::resize(size_t width, size_t height)
{
  if (this->use_ext_) {
    GL_CHECKD(glBindRenderbufferEXT(GL_RENDERBUFFER, this->renderbuf_id_));
  } else {
    GL_CHECKD(glBindRenderbuffer(GL_RENDERBUFFER, this->renderbuf_id_));
  }
  GL_CHECKD(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height));
  if (this->use_ext_) {
    GL_CHECKD(glBindRenderbufferEXT(GL_RENDERBUFFER, this->depthbuf_id_));
  } else {
    GL_CHECKD(glBindRenderbuffer(GL_RENDERBUFFER, this->depthbuf_id_));
  }
  GL_CHECKD(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));

  width_ = width;
  height_ = height;

  return true;
}

// Bind this VBO. Returs the old FBO id.
GLuint FBO::bind()
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&this->old_fbo_id_));
  if (this->use_ext_) {
    GL_CHECKD(glBindFramebufferEXT(GL_FRAMEBUFFER, this->fbo_id_));
  } else {
    GL_CHECKD(glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id_));
  }
  return this->old_fbo_id_;
}

// Unbind this VBO, and bind the previous FBO id.
void FBO::unbind()
{
  if (this->use_ext_) {
    GL_CHECKD(glBindFramebufferEXT(GL_FRAMEBUFFER, this->old_fbo_id_));
  } else {
    GL_CHECKD(glBindFramebuffer(GL_FRAMEBUFFER, this->old_fbo_id_));
  }
  this->old_fbo_id_ = 0;
}

void FBO::destroy()
{
  this->unbind();
  if (this->depthbuf_id_ != 0) {
    GL_CHECKD(glDeleteRenderbuffers(1, &this->depthbuf_id_));
    this->depthbuf_id_ = 0;
  }
  if (this->renderbuf_id_ != 0) {
    GL_CHECKD(glDeleteRenderbuffers(1, &this->renderbuf_id_));
    this->renderbuf_id_ = 0;
  }
  if (this->fbo_id_ != 0) {
    GL_CHECKD(glDeleteFramebuffers(1, &this->fbo_id_));
    this->fbo_id_ = 0;
  }
}
