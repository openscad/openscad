#include "OffscreenContextFactory.h"

#include "printutils.h"

#ifdef __APPLE__
#include "offscreen-old/OffscreenContextNSOpenGL.h"
#endif
#ifdef _WIN32
#include "offscreen-old/OffscreenContextWGL.h"
#endif
#ifdef ENABLE_EGL
#include "offscreen-old/OffscreenContextEGL.h"
#endif
#ifdef ENABLE_GLX
#include "offscreen-old/OffscreenContextGLX.h"
#endif

namespace OffscreenContextFactory {

const char *defaultProvider() {
#ifdef __APPLE__
  return "nsopengl";
#endif
#ifdef ENABLE_EGL
  return "egl";
#endif
#ifdef ENABLE_GLX
  return "glx";
#endif
#ifdef _WIN32
  return "wgl";
#endif
#ifdef ENABLE_GLFW
 return "glfw";
#endif
}

std::shared_ptr<OpenGLContext> create(const std::string& provider, const OffscreenContextFactory::ContextAttributes& attrib)
{
  // FIXME: We could log an error if the chosen provider doesn't support all our attribs.
#ifdef __APPLE__
  if (provider == "nsopengl") {
    if (attrib.gles) {
      LOG("GLES is not supported on macOS");
    }
    if (attrib.compatibilityProfile) {
      LOG("Compatibility context is not available on macOS");
    }
    return CreateOffscreenContextNSOpenGL(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion);
  }
#endif
#if ENABLE_EGL
  if (provider == "egl") {
    return CreateOffscreenContextEGL(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion,
                                     attrib.gles, attrib.compatibilityProfile, attrib.gpu);
  }
  else
#endif
#ifdef ENABLE_GLX
  if (provider == "glx") {
   return CreateOffscreenContextGLX(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion, 
                                    attrib.gles, attrib.compatibilityProfile);
  }
#endif
#ifdef _WIN32
  if (provider == "wgl") {
    if (attrib.gles) {
      LOG("GLES is not supported on Windows");
    }
    return CreateOffscreenContextWGL(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion,
                                     attrib.compatibilityProfile);
  }
  else
#endif
  LOG("GL context provider '%1$s' not found", provider);
  return nullptr;
}

}  // namespace OffscreenContextFactory

