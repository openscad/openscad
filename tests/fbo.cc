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
  glGenFramebuffers(1, &fbo->fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);
  REPORTGLERROR("binding framebuffer");

  // Generate depth and render buffers
  glGenRenderbuffers(1, &fbo->depthbuf_id);
  glGenRenderbuffers(1, &fbo->renderbuf_id);

  // Create buffers with correct size
  if (!fbo_resize(fbo, width, height)) return false;

  // Attach render and depth buffers
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_RENDERBUFFER, fbo->renderbuf_id);
  REPORTGLERROR("specifying color render buffer");

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying color render buffer.\n");
    return false;
  }

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                               GL_RENDERBUFFER, fbo->depthbuf_id);
  REPORTGLERROR("specifying depth render buffer");

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying depth render buffer.\n");
    return false;
  }

  return true;
}

bool fbo_resize(fbo_t *fbo, size_t width, size_t height)
{
  glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthbuf_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  REPORTGLERROR("creating depth render buffer");

  glBindRenderbuffer(GL_RENDERBUFFER, fbo->renderbuf_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
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
  glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);
  return fbo->old_fbo_id;
}

void fbo_unbind(fbo_t *fbo)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo->old_fbo_id);
}
