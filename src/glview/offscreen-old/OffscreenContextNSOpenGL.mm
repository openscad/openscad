#include "OffscreenContextNSOpenGL.h"

#include <iostream>
#include <sstream>

#include "imageutils.h"
#include "system-gl.h"
#import <AppKit/AppKit.h>   // for NSOpenGL...

class OffscreenContextNSOpenGL : public OffscreenContext
{
public:
  OffscreenContextNSOpenGL(uint32_t width, uint32_t height) : OffscreenContext(width, height) {}
  ~OffscreenContextNSOpenGL() {
    [this->openGLContext clearDrawable];
    [this->openGLContext release];
    [this->pool release];
  }

  std::string getInfo() const override {
    std::ostringstream out;
    out << "GL context creator: NSOpenGL (old)\n"
	<< "PNG generator: Core Foundation\n";
    return out.str();
  }

  bool makeCurrent() const override {
    [this->openGLContext makeCurrentContext];
    return true;
  }

  NSOpenGLContext *openGLContext;
  NSAutoreleasePool *pool;
};

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextNSOpenGL(
  uint32_t width, uint32_t height, uint32_t majorGLVersion, 
  uint32_t minorGLVersion)   
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

  return ctx;
}

}  // namespace offscreen_old
