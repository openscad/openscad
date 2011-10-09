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

bool use_ext()
{
  // do we need to use the EXT or ARB version?
  if (!glewIsSupported("GL_ARB_framebuffer_object") &&
        glewIsSupported("GL_EXT_framebuffer_object")) {
    return true;
  } else {
    return false;
  }
}

bool check_fbo_status()
{
	/* This code is based on user V-man code from
	http://www.opengl.org/wiki/GL_EXT_framebuffer_multisample
	See also: http://www.songho.ca/opengl/gl_fbo.html */
	GLenum status;
	bool result = false;
	if (use_ext())
		status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	else
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (glGetError() != GL_NO_ERROR)
		fprintf(stderr, "OpenGL Error %i\n",glGetError());

	if (status == GL_FRAMEBUFFER_COMPLETE)
		result = true;
	else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
		fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT\n");
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT)
		fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT\n");
	else
		fprintf(stderr, "Unknown Code: glCheckFramebufferStatusEXT returned %i\n",status);
	return result;
}

bool fbo_ext_init(fbo_t *fbo, size_t width, size_t height)
{
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


  if (!check_fbo_status()) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying color render buffer.\n");
    return false;
  }

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
                               GL_RENDERBUFFER_EXT, fbo->depthbuf_id);
  REPORTGLERROR("specifying depth render buffer");

  if (!check_fbo_status()) {
    fprintf(stderr, "Problem with OpenGL framebuffer after specifying depth render buffer.\n");
    return false;
  }

  return true;
}

bool fbo_arb_init(fbo_t *fbo, size_t width, size_t height)
{
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


bool fbo_init(fbo_t *fbo, size_t width, size_t height)
{
  /*
  Some OpenGL drivers include the framebuffer functions but not with
  core or ARB names, only with the EXT name. This has been worked-around
  by deciding at runtime, using GLEW, which version needs to be used. See also:
  
  http://www.opengl.org/wiki/Framebuffer_Object
  http://stackoverflow.com/questions/6912988/glgenframebuffers-or-glgenframebuffersex
  http://www.devmaster.net/forums/showthread.php?t=10967
  */

  bool result = false;
  if (glewIsSupported("GL_ARB_framebuffer_object"))
    result = fbo_arb_init(fbo, width, height);
  else if (use_ext())
    result = fbo_ext_init(fbo, width, height);
  else
    fprintf(stderr, "Framebuffer Object extension not found by GLEW\n");
  return result;
}

bool fbo_resize(fbo_t *fbo, size_t width, size_t height)
{
  if (use_ext()) {
    glBindRenderbufferEXT(GL_RENDERBUFFER, fbo->depthbuf_id);
    glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    REPORTGLERROR("creating depth render buffer");

    glBindRenderbufferEXT(GL_RENDERBUFFER, fbo->renderbuf_id);
    glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_RGBA8, width, height);
    REPORTGLERROR("creating color render buffer");
  } else {
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->depthbuf_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    REPORTGLERROR("creating depth render buffer");

    glBindRenderbuffer(GL_RENDERBUFFER, fbo->renderbuf_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    REPORTGLERROR("creating color render buffer");
  }

  return true;
}

void fbo_delete(fbo_t *fbo)
{
  delete fbo;
}

GLuint fbo_bind(fbo_t *fbo)
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&fbo->old_fbo_id);
  if (use_ext())
    glBindFramebufferEXT(GL_FRAMEBUFFER, fbo->fbo_id);
  else
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);
  return fbo->old_fbo_id;
}

void fbo_unbind(fbo_t *fbo)
{
  if (use_ext())
    glBindFramebufferEXT(GL_FRAMEBUFFER, fbo->old_fbo_id);
  else
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->old_fbo_id);
}
