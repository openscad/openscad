#include "fbo.h"
#include <stdio.h>
#include <iostream>

fbo_t *fbo_new()
{
  fbo_t *fbo = new fbo_t;
  fbo->fbo_id = 0;
  fbo->old_fbo_id = 0;
  fbo->renderbuf_id = 0;
  fbo->depthbuf_id = 0;

  return fbo;
}

#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }

bool fbo_init(fbo_t *fbo, size_t width, size_t height)
{
  if (!glewIsSupported("GL_ARB_framebuffer_object")) {
    fprintf(stderr, "Framebuffer extension not found\n");
    return false;
  }

  // Generate and bind FBO
  glGenFramebuffersEXT(1, &fbo->fbo_id);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fbo_id);
  REPORTGLERROR("binding framebuffer");

  // Generate depth and render buffers
  glGenRenderbuffersEXT(1, &fbo->depthbuf_id);
  glGenRenderbuffersEXT(1, &fbo->renderbuf_id);

  // Create buffers with correct size
  if (!fbo_resize(fbo, width, height)) return false;

  // Attach render and depth buffers
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                               GL_RENDERBUFFER_EXT, fbo->renderbuf_id);
  REPORTGLERROR("specifying color render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying color render buffer.\n");
    return false;
  }

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
                               GL_RENDERBUFFER_EXT, fbo->depthbuf_id);
  REPORTGLERROR("specifying depth render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying depth render buffer.\n");
    return false;
  }

  return true;
}

bool fbo_resize(fbo_t *fbo, size_t width, size_t height)
{
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo->depthbuf_id);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
  REPORTGLERROR("creating depth render buffer");

  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo->renderbuf_id);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
  REPORTGLERROR("creating color render buffer");

  return true;
}

void fbo_delete(fbo_t *fbo)
{
  delete fbo;
}

GLuint fbo_bind(fbo_t *fbo)
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&fbo->old_fbo_id);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fbo_id);
  return fbo->old_fbo_id;
}

void fbo_unbind(fbo_t *fbo)
{
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->old_fbo_id);
}
