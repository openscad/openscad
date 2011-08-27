#include "OffscreenContext.h"

#include <GL/gl.h>
#include <GL/glu.h>      // for gluCheckExtension
#include <SDL.h>

// Simple error reporting macros to help keep the sample code clean
#define REPORT_ERROR_AND_EXIT(desc) { std::cout << desc << "\n"; return false; }
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

struct OffscreenContext
{
  int width;
  int height;
  GLuint fbo;
  GLuint colorbo;
  GLuint depthbo;
};


OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  ctx->width = w;
  ctx->height = h;

  // dummy window 
  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetVideoMode(10,10,32,SDL_OPENGL);

  /*
   * Test if framebuffer objects are supported
   */
  const GLubyte* strExt = glGetString(GL_EXTENSIONS);
  GLboolean fboSupported = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", strExt);
  if (!fboSupported)
    REPORT_ERROR_AND_EXIT("Your system does not support framebuffer extension - unable to render scene");
  /*
   * Create an FBO
   */
  GLuint renderBuffer = 0;
  GLuint depthBuffer = 0;
  // Depth buffer to use for depth testing - optional if you're not using depth testing
  glGenRenderbuffersEXT(1, &depthBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,  w, h);
  REPORTGLERROR("creating depth render buffer");

  // Render buffer to use for imaging
  glGenRenderbuffersEXT(1, &renderBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, w, h);
  REPORTGLERROR("creating color render buffer");
  ctx->fbo = 0;
  glGenFramebuffersEXT(1, &ctx->fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->fbo);
  REPORTGLERROR("binding framebuffer");

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                               GL_RENDERBUFFER_EXT, renderBuffer);
  REPORTGLERROR("specifying color render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != 
      GL_FRAMEBUFFER_COMPLETE_EXT)
    REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer after specifying color render buffer.");

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
                               GL_RENDERBUFFER_EXT, depthBuffer);
  REPORTGLERROR("specifying depth render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != 
      GL_FRAMEBUFFER_COMPLETE_EXT)
    REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer after specifying depth render buffer.");

  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  // "un"bind my FBO
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  /*
   * Cleanup
   */
  return true;
}

void write_targa(const char *filename, GLubyte *pixels, int width, int height)
{
	FILE *f = fopen( filename, "w" );
	int y;
	if (f) {
		GLubyte header[] = {
			00,00,02, 00,00,00, 00,00,00, 00,00,00,
			0xff & width, 0xff & width >> 8,
			0xff & height, 0xff & height >> 8,
			32, 0x20 }; // next-to-last = bit depth
		fwrite( header, sizeof(header), 1, f);
		for (y=height-1; y>=0; y--)
			fwrite( pixels + y*width*4, 4, width, f);
		fclose(f);
	}
}

bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  /*
   * Extract the resulting rendering as an image
   */

  int samplesPerPixel = 4; // R, G, B and A

  GLubyte pixels[ ctx->width * ctx->height * samplesPerPixel ];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
  printf("writing %s\n",filename);
  write_targa(filename,pixels,ctx->width, ctx->height);

  return true;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->fbo);
}
