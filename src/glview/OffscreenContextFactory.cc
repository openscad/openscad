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
#ifdef NULLGL
#include "OffscreenContextNULL.h"
#endif

namespace OffscreenContextFactory {

const char *defaultProvider() {
#ifdef NULLGL
  return "nullgl";
#else
#ifdef __APPLE__
  return "nsopengl-old";
#endif
#ifdef ENABLE_EGL
  return "egl-old";
#endif
#ifdef ENABLE_GLX
  return "glx-old";
#endif
#ifdef _WIN32
  return "wgl-old";
#endif
#endif  // NULLGL
}

std::shared_ptr<OpenGLContext> create(const std::string& provider, const OffscreenContextFactory::ContextAttributes& attrib)
{
  // FIXME: We could log an error if the chosen provider doesn't support all our attribs.
#ifdef NULLGL
  if (provider == "nullgl") {
    return CreateOffscreenContextNULL();
  }
#else
#ifdef __APPLE__
  if (provider == "nsopengl-old") {
    if (attrib.gles) {
      LOG("GLES is not supported on macOS");
    }
    if (attrib.compatibilityProfile) {
      LOG("Compatibility context is not available on macOS");
    }
    return offscreen_old::CreateOffscreenContextNSOpenGL(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion);
  }
#endif
#if ENABLE_EGL
  if (provider == "egl-old") {
    return offscreen_old::CreateOffscreenContextEGL(attrib.width, attrib.height,
                                                    attrib.majorGLVersion, attrib.minorGLVersion,
                                                    attrib.gles, attrib.compatibilityProfile, attrib.gpu);
  }
  else
#endif
#ifdef ENABLE_GLX
  if (provider == "glx-old") {
   return offscreen_old::CreateOffscreenContextGLX(attrib.width, attrib.height, attrib.majorGLVersion, attrib.minorGLVersion, 
                                    attrib.gles, attrib.compatibilityProfile);
  }
#endif
#ifdef _WIN32
  if (provider == "wgl-old") {
    if (attrib.gles) {
      LOG("GLES is not supported on Windows");
    }
    return offscreen_old::CreateOffscreenContextWGL(attrib.width, attrib.height,
						    attrib.majorGLVersion, attrib.minorGLVersion,
						    attrib.compatibilityProfile);
  }
  else
#endif
#endif  // NULLGL
  LOG("GL context provider '%1$s' not found", provider);
  return nullptr;
}

}  // namespace OffscreenContextFactory

