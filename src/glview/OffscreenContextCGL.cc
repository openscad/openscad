#include "glview/OffscreenContextCGL.h"

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

#include "glview/OffscreenContext.h"
#include "glview/system-gl.h"
#include "utils/printutils.h"
#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>

class OffscreenContextCGL : public OffscreenContext
{
public:
  OffscreenContextCGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextCGL()
  {
    if (cglContext) {
      CGLDestroyContext(cglContext);
    }
  }

  std::string getInfo() const override
  {
    std::ostringstream out;
    out << "GL context creator: CGL (new)\n";
    return out.str();
  }

  bool makeCurrent() const override
  {
    const auto err = CGLSetCurrentContext(cglContext);
    if (err != kCGLNoError) {
      LOG("CGLSetCurrentContext() failed: %1$s (%2$d)", CGLErrorString(err), static_cast<int>(err));
      return false;
    }
    return true;
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
    kCGLPFAOpenGLProfile,       (CGLPixelFormatAttribute)glVersion,
    kCGLPFAColorSize,           (CGLPixelFormatAttribute)24,
    kCGLPFAAlphaSize,           (CGLPixelFormatAttribute)8,
    kCGLPFADoubleBuffer,        kCGLPFASampleBuffers,
    (CGLPixelFormatAttribute)1, kCGLPFASamples,
    (CGLPixelFormatAttribute)4, (CGLPixelFormatAttribute)0};
  CGLPixelFormatObj pixelFormat = NULL;
  GLint numPixelFormats = 0;
  const auto status = CGLChoosePixelFormat(attributes, &pixelFormat, &numPixelFormats);
  if (status != kCGLNoError) {
    LOG("CGLChoosePixelFormat() failed: %1$s (%2$d)", CGLErrorString(status), static_cast<int>(status));
    return nullptr;
  }
  CGLCreateContext(pixelFormat, NULL, &ctx->cglContext);
  CGLDestroyPixelFormat(pixelFormat);

  return ctx;
}
