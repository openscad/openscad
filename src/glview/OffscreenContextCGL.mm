#include "OffscreenContext.h"
#include "imageutils.h"
#include "system-gl.h"

#include <iostream>
#include <sstream>

#import <AppKit/AppKit.h>   // for NSOpenGL...

struct OffscreenContext
{
  NSOpenGLContext *openGLContext;
  NSAutoreleasePool *pool;
  int width;
  int height;
};

#include "OffscreenContextAll.hpp"

std::string offscreen_context_getinfo(OffscreenContext *)
{
  std::ostringstream out;
  out << "GL context creator: Cocoa / CGL\n"
      << "PNG generator: Core Foundation\n";
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
    NSOpenGLPFANoRecovery,
    NSOpenGLPFADepthSize, 24,
    NSOpenGLPFAStencilSize, 8,
// Enable this to force software rendering
// NSOpenGLPFARendererID, kCGLRendererGenericID,
// Took out the acceleration requirement to be able to run the tests
// in a non-accelerated VM.
// NSOpenGLPFAAccelerated,
    (NSOpenGLPixelFormatAttribute) 0
  };
  NSOpenGLPixelFormat *pixFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];

  // Create and make current the OpenGL context to render with (with color and depth buffers)
  ctx->openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat shareContext:nil];
  if (!ctx->openGLContext) {
    std::cerr << "Unable to create NSOpenGLContext\n";
    [ctx->pool release];
    delete ctx;
    return nullptr;
  }

  [ctx->openGLContext makeCurrentContext];
  
  auto *returnCtx = create_offscreen_context_common(ctx);
  if (!returnCtx) {
    [ctx->openGLContext release];
    [ctx->pool release];
    delete ctx;
  }
  return returnCtx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  /*
   * Cleanup
   */
  [ctx->openGLContext clearDrawable];
  [ctx->openGLContext release];

  [ctx->pool release];
  return true;
}

/*!
  Capture framebuffer from OpenGL and write it to the given ostream
*/
bool save_framebuffer(const OffscreenContext *ctx, std::ostream &output)
{
  if (!ctx) return false;
  // Read pixels from OpenGL
  int samplesPerPixel = 4; // R, G, B and A
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *bufferData = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!bufferData) {
    std::cerr << "Unable to allocate buffer for image extraction.";
    return 1;
  }
  GL_CHECKD(glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE,
			 bufferData));

  // Flip it vertically - images read from OpenGL buffers are upside-down
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cout << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  flip_image(bufferData, flippedBuffer, samplesPerPixel, ctx->width, ctx->height);

  bool writeok = write_png(output, flippedBuffer, ctx->width, ctx->height);

  free(flippedBuffer);
  free(bufferData);

  return writeok;
}

