#include "fboutils.h"
#include <iostream>

GLuint fbo_create(GLsizei width, GLsizei height)
{
  // Test if framebuffer objects are supported
  // FIXME: Use GLEW
  const GLubyte* strExt = glGetString(GL_EXTENSIONS);
  GLboolean fboSupported = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", strExt);
  if (!fboSupported) {
		std::cerr << "Your system does not support framebuffer extension - unable to render scene\n";
		return 0;
	}

  // Create an FBO
  GLuint fbo = 0;
  GLuint renderBuffer = 0;
  GLuint depthBuffer = 0;
  // Depth buffer to use for depth testing - optional if you're not using depth testing
  glGenRenderbuffersEXT(1, &depthBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,  width, height);
  REPORTGLERROR("creating depth render buffer");

  // Render buffer to use for imaging
  glGenRenderbuffersEXT(1, &renderBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
  REPORTGLERROR("creating color render buffer");
  glGenFramebuffersEXT(1, &fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  REPORTGLERROR("binding framebuffer");

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                               GL_RENDERBUFFER_EXT, renderBuffer);
  REPORTGLERROR("specifying color render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=  GL_FRAMEBUFFER_COMPLETE_EXT) {
		std::cerr << "Problem with OpenGL framebuffer after specifying color render buffer.\n";
		return 0;
  }

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthBuffer);
  REPORTGLERROR("specifying depth render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
		std::cerr << "Problem with OpenGL framebuffer after specifying depth render buffer.\n";
		return 0;
  }
	return fbo;
}

void fbo_bind(GLuint fbo)
{
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
}

void fbo_unbind()
{
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
