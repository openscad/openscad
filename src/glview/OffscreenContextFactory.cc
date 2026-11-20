#include "glview/OffscreenContextFactory.h"

#include <memory>
#include <string>

#include "utils/printutils.h"

#ifdef __APPLE__
#include "glview/offscreen-old/OffscreenContextNSOpenGL.h"
#include "glview/OffscreenContextCGL.h"
#endif
#ifdef _WIN32
#include "glview/offscreen-old/OffscreenContextWGL.h"
#include "glview/OffscreenContextWGL.h"
#endif

#ifdef ENABLE_OSMESA
#include "glview/OffscreenContextOSMesa.h"
#endif
#ifdef ENABLE_EGL
#include "glview/offscreen-old/OffscreenContextEGL.h"
#include "glview/OffscreenContextEGL.h"
#endif
#ifdef ENABLE_GLX
#include "glview/offscreen-old/OffscreenContextGLX.h"
#include "glview/OffscreenContextGLX.h"
#endif
#ifdef NULLGL
#include "glview/OffscreenContextNULL.h"
#endif

namespace OffscreenContextFactory {

const char *defaultProvider()
{
#ifdef NULLGL
  return "nullgl";
#elif defined(DEFAULT_OFFSCREEN_CONTEXT)
  return DEFAULT_OFFSCREEN_CONTEXT;
#else
#if defined(__APPLE__)
  return "cgl";
#elif defined(ENABLE_EGL)
  return "egl";
#elif defined(ENABLE_GLX)
  return "glx";
#elif defined(ENABLE_OSMESA)
  return "osmesa";
#elif defined(_WIN32)
  return "wgl-old";
#else
  return "nullgl";
#endif

#endif  // NULLGL
}

std::shared_ptr<OpenGLContext> create(const std::string& provider,
                                      const OffscreenContextFactory::ContextAttributes& attrib)
{
  PRINTDB("Creating OpenGL context with the %1s provider:", provider);
  PRINTDB("  Size: %d x %d", attrib.width % attrib.height);
  PRINTDB("  Version: %s %d.%d %s",
          (attrib.gles ? "OpenGL ES" : "OpenGL") % attrib.majorGLVersion % attrib.minorGLVersion %
            (attrib.compatibilityProfile ? "(compatibility profile requested)" : ""));
  // FIXME: We should log an error if the chosen provider doesn't support all our attribs.
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
    return offscreen_old::CreateOffscreenContextNSOpenGL(attrib.width, attrib.height,
                                                         attrib.majorGLVersion, attrib.minorGLVersion);
  } else if (provider == "cgl") {
    return CreateOffscreenContextCGL(attrib.width, attrib.height, attrib.majorGLVersion,
                                     attrib.minorGLVersion);
  }
#endif
#ifdef ENABLE_OSMESA
  if (provider == "osmesa") {
    return CreateOffscreenContextOSMesa(attrib.width, attrib.height, attrib.majorGLVersion,
                                        attrib.minorGLVersion, attrib.compatibilityProfile);
  }
#endif
#if ENABLE_EGL
  if (provider == "egl-old") {
    return offscreen_old::CreateOffscreenContextEGL(attrib.width, attrib.height, attrib.majorGLVersion,
                                                    attrib.minorGLVersion, attrib.gles,
                                                    attrib.compatibilityProfile);
  } else if (provider == "egl") {
    return CreateOffscreenContextEGL(attrib.width, attrib.height, attrib.majorGLVersion,
                                     attrib.minorGLVersion, attrib.gles, attrib.compatibilityProfile);
  } else
#endif
#ifdef ENABLE_GLX
    if (provider == "glx-old") {
    return offscreen_old::CreateOffscreenContextGLX(attrib.width, attrib.height, attrib.majorGLVersion,
                                                    attrib.minorGLVersion, attrib.gles,
                                                    attrib.compatibilityProfile);
  } else if (provider == "glx") {
    return CreateOffscreenContextGLX(attrib.width, attrib.height, attrib.majorGLVersion,
                                     attrib.minorGLVersion, attrib.gles, attrib.compatibilityProfile);
  }
#endif
#ifdef _WIN32
  if (provider == "wgl-old") {
    if (attrib.gles) {
      LOG("GLES is not supported on Windows");
    }
    return offscreen_old::CreateOffscreenContextWGL(attrib.width, attrib.height, attrib.majorGLVersion,
                                                    attrib.minorGLVersion, attrib.compatibilityProfile);
  } else if (provider == "wgl") {
    return CreateOffscreenContextWGL(attrib.width, attrib.height, attrib.majorGLVersion,
                                     attrib.minorGLVersion, attrib.compatibilityProfile);
  } else
#endif

#endif  // NULLGL
  LOG("GL context provider '%1$s' not found", provider);
  return nullptr;
}

}  // namespace OffscreenContextFactory
