#include "OffscreenContextNSOpenGL.h"
#include "OffscreenContext.h"
#include "imageutils.h"
#include "system-gl.h"

#include <iostream>
#include <sstream>

#import <AppKit/AppKit.h>   // for NSOpenGL...

class OffscreenContextNSOpenGL : public OffscreenContext
{
public:
  OffscreenContextNSOpenGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextNSOpenGL() {
    [this->openGLContext clearDrawable];
    [this->openGLContext release];
    [this->pool release];
  }
  std::string getInfo() const override;
  
  NSOpenGLContext *openGLContext;
  NSAutoreleasePool *pool;
  int width;
  int height;
};

std::string OffscreenContextNSOpenGL::getInfo() const {
  std::ostringstream out;
  out << "GL context creator: NSOpenGL\n"
      << "PNG generator: Core Foundation\n";
  return out.str();
}

std::shared_ptr<OffscreenContext> CreateOffscreenContextNSOpenGL(
  unsigned int width, unsigned int height, unsigned int majorGLVersion, 
  unsigned int minorGLVersion)   
{
  auto ctx = std::make_shared<OffscreenContextNSOpenGL>(width, height);

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
    return nullptr;
  }

  [ctx->openGLContext makeCurrentContext];
  
  return ctx;
}

