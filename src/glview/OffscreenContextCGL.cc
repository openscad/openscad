#include "glview/OffscreenContextCGL.h"

#include <sstream>
#include <memory>
#include <cstddef>
#include <string>
#include <iostream>

#include "glview/system-gl.h"
#include <OpenGL/OpenGL.h>

class OffscreenContextCGL : public OffscreenContext {

public:
  OffscreenContextCGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextCGL() {
    CGLDestroyContext(cglContext);
  }

  // FIXME: What info are we really interested in here?
  std::string getInfo() const override {
    std::ostringstream out;
    out << "GL context creator: CGL (new)\n"
	<< "PNG generator: Core Foundation\n";
    return out.str();
  }

  bool makeCurrent() const override {
    return CGLSetCurrentContext(this->cglContext) == kCGLNoError;
  }

  CGLContextObj cglContext = nullptr;
};

std::shared_ptr<OffscreenContext> CreateOffscreenContextCGL(size_t width, size_t height,
							    size_t majorGLVersion, size_t minorGLVersion)
{
  auto ctx = std::make_shared<OffscreenContextCGL>(width, height);

  CGLOpenGLProfile glVersion = kCGLOGLPVersion_Legacy;
  if (majorGLVersion >= 4) glVersion = kCGLOGLPVersion_GL4_Core;
  else if (majorGLVersion >= 3) glVersion = kCGLOGLPVersion_GL3_Core;

  CGLPixelFormatAttribute attributes[13] = {
    kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)glVersion,
    kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
    kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
    kCGLPFADoubleBuffer,
    kCGLPFASampleBuffers, (CGLPixelFormatAttribute)1,
    kCGLPFASamples,  (CGLPixelFormatAttribute)4,
    (CGLPixelFormatAttribute) 0
  };
  CGLPixelFormatObj pixelFormat = NULL;
  GLint numPixelFormats = 0;
  const auto status = CGLChoosePixelFormat(attributes, &pixelFormat, &numPixelFormats);
  if (status != kCGLNoError) {
    std::cerr << "CGLChoosePixelFormat() failed: " << CGLErrorString(status) << std::endl;
    return nullptr;
  }
  CGLCreateContext(pixelFormat, NULL, &ctx->cglContext);
  CGLDestroyPixelFormat(pixelFormat);

  return ctx;
}
