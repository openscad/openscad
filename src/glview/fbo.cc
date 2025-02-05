#include "fbo.h"

#include "system-gl.h"

#include <iostream>
#include <memory>

namespace {

bool checkFBOStatus() {
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  const char *statusString = nullptr;
  switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
    return true;
    break;
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
  if (hasGLExtension(ARB_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ false);
  } else if (hasGLExtension(EXT_framebuffer_object)) {
    return std::make_unique<FBO>(width, height, /*useEXT*/ true);
  } else {
    // TODO: LOG
    std::cerr << "Framebuffer Objects not supported" << std::endl;
    return nullptr;
  }
}

FBO::FBO(int width, int height, bool useEXT) : useEXT(useEXT) {
  // Generate and bind FBO
  GL_CHECKD(glGenFramebuffers(1, &this->fbo_id));
  this->bind();

  // Generate depth and render buffers
  GL_CHECKD(glGenRenderbuffers(1, &this->depthbuf_id));
  GL_CHECKD(glGenRenderbuffers(1, &this->renderbuf_id));

  // Create buffers with correct size
  if (!this->resize(width, height)) return;

  // Attach render and depth buffers
  GL_CHECKD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				  GL_RENDERBUFFER, this->renderbuf_id));

  if (!checkFBOStatus()) {
    std::cerr << "Problem with OpenGL framebuffer after specifying color render buffer.\n";
    return;
  }

  //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
  // to prevent Mesa's software renderer from crashing, do this in two stages.
  // ie. instead of using GL_DEPTH_STENCIL_ATTACHMENT, do DEPTH then STENCIL.
  GL_CHECKD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				     GL_RENDERBUFFER, this->depthbuf_id));
  GL_CHECKD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
				     GL_RENDERBUFFER, this->depthbuf_id));

  if (!checkFBOStatus()) {
    std::cerr << "Problem with OpenGL framebuffer after specifying depth render buffer.\n";
    return;
  }

  this->complete = true;
}

bool FBO::resize(size_t width, size_t height)
{
  if (this->useEXT) {
    GL_CHECKD(glBindRenderbufferEXT(GL_RENDERBUFFER, this->renderbuf_id));
  } else {
    GL_CHECKD(glBindRenderbuffer(GL_RENDERBUFFER, this->renderbuf_id));
  }
  GL_CHECKD(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height));
  if (this->useEXT) {
    GL_CHECKD(glBindRenderbufferEXT(GL_RENDERBUFFER, this->depthbuf_id));
  } else {
    GL_CHECKD(glBindRenderbuffer(GL_RENDERBUFFER, this->depthbuf_id));
  }
  GL_CHECKD(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));

  return true;
}

GLuint FBO::bind()
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&this->old_fbo_id));
  if (this->useEXT) {
    GL_CHECKD(glBindFramebufferEXT(GL_FRAMEBUFFER, this->fbo_id));
  } else {
    GL_CHECKD(glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id));
  }
  return this->old_fbo_id;
}

void FBO::unbind()
{
  if (this->useEXT) {
    GL_CHECKD(glBindFramebufferEXT(GL_FRAMEBUFFER, this->old_fbo_id));
  } else {
    GL_CHECKD(glBindFramebuffer(GL_FRAMEBUFFER, this->old_fbo_id));
  }
  this->old_fbo_id = 0;
}

void FBO::destroy()
{
  this->unbind();
  if (this->depthbuf_id != 0) {
    GL_CHECKD(glDeleteRenderbuffers(1, &this->depthbuf_id));
    this->depthbuf_id = 0;
  }
  if (this->renderbuf_id != 0) {
    GL_CHECKD(glDeleteRenderbuffers(1, &this->renderbuf_id));
    this->renderbuf_id = 0;
  }
  if (this->fbo_id != 0) {
    GL_CHECKD(glDeleteFramebuffers(1, &this->fbo_id));
    this->fbo_id = 0;
  }
}
