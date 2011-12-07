#include "OffscreenContext.h"
#include "imageutils.h"
#include "fbo.h"
#include <iostream>

#import <AppKit/AppKit.h>   // for NSOpenGL...


#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }

struct OffscreenContext
{
  NSOpenGLContext *openGLContext;
  NSAutoreleasePool *pool;
  int width;
  int height;
  fbo_t *fbo;
};

string offscreen_context_getinfo(OffscreenContext *ctx)
{
  stringstream out;
  out << "GL context creator: Cocoa / CGL\n"
      << "PNG generator: Core Foundation\n"
      << "OS info: Mac OSX\n"
      << "Machine: Apple(TM) Mac(TM)\n";
  return out.str();
}

OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  ctx->width = w;
  ctx->height = h;

  ctx->pool = [NSAutoreleasePool new];

  // Create an OpenGL context just so that OpenGL calls will work. 
  // Will not be used for actual rendering.
                                   
  NSOpenGLPixelFormatAttribute attributes[] = {
    NSOpenGLPFAPixelBuffer,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 24,
    NSOpenGLPFAStencilSize, 8,
    (NSOpenGLPixelFormatAttribute) 0
  };
  NSOpenGLPixelFormat *pixFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];

  // Create and make current the OpenGL context to render with (with color and depth buffers)
  ctx->openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat shareContext:nil];
  if (!ctx->openGLContext) {
    std::cerr << "Unable to create NSOpenGLContext\n";
    return NULL;
  }

  [ctx->openGLContext makeCurrentContext];
  
  // glewInit must come after Context creation and before FBO calls.
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    std::cerr << "Unable to init GLEW: " << glewGetErrorString(err) << std::endl;
    return NULL;
  }
  glew_dump();

  ctx->fbo = fbo_new();
  if (!fbo_init(ctx->fbo, w, h)) {
    return NULL;
  }

  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  fbo_unbind(ctx->fbo);
  fbo_delete(ctx->fbo);

  /*
   * Cleanup
   */
  [ctx->openGLContext clearDrawable];
  [ctx->openGLContext release];

  [ctx->pool release];
  return true;
}

/*!
  Capture framebuffer from OpenGL and write it to the given filename as PNG.
*/
bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  if (!ctx || !filename) return false;
  // Read pixels from OpenGL
  int samplesPerPixel = 4; // R, G, B and A
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *bufferData = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!bufferData) {
    std::cerr << "Unable to allocate buffer for image extraction.";
    return 1;
  }
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, 
               bufferData);
  REPORTGLERROR("reading pixels from framebuffer");
  
  // Flip it vertically - images read from OpenGL buffers are upside-down
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cout << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  flip_image(bufferData, flippedBuffer, samplesPerPixel, ctx->width, ctx->height);

  bool writeok = write_png(filename, flippedBuffer, ctx->width, ctx->height);

  free(flippedBuffer);
  free(bufferData);

  return writeok;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
  fbo_bind(ctx->fbo);
}
