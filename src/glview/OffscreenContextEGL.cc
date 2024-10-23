#include "glview/OffscreenContextEGL.h"

#include <memory>
#include <fcntl.h>
#include <cstddef>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "utils/printutils.h"
#define GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#include <GL/gl.h>

namespace {

#define CASE_STR(value) case value: return #value;
const char* eglGetErrorString(EGLint error)
{
  switch(error) {
    CASE_STR(EGL_SUCCESS)
    CASE_STR(EGL_NOT_INITIALIZED)
    CASE_STR(EGL_BAD_ACCESS)
    CASE_STR(EGL_BAD_ALLOC)
    CASE_STR(EGL_BAD_ATTRIBUTE)
    CASE_STR(EGL_BAD_CONTEXT)
    CASE_STR(EGL_BAD_CONFIG)
    CASE_STR(EGL_BAD_CURRENT_SURFACE)
    CASE_STR(EGL_BAD_DISPLAY)
    CASE_STR(EGL_BAD_SURFACE)
    CASE_STR(EGL_BAD_MATCH)
    CASE_STR(EGL_BAD_PARAMETER)
    CASE_STR(EGL_BAD_NATIVE_PIXMAP)
    CASE_STR(EGL_BAD_NATIVE_WINDOW)
    CASE_STR(EGL_CONTEXT_LOST)
    default: return "Unknown";
  }
}
#undef CASE_STR

} // namespace

class OffscreenContextEGL : public OffscreenContext {

public:
  EGLDisplay eglDisplay;
  EGLSurface eglSurface;
  EGLContext eglContext;

  OffscreenContextEGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextEGL() {
    if (this->eglSurface) eglDestroySurface(this->eglDisplay, this->eglSurface);
    if (this->eglDisplay) eglTerminate(this->eglDisplay);
  }

  std::string getInfo() const override {
    std::ostringstream result;

    const char *eglVersion = eglQueryString(this->eglDisplay, EGL_VERSION);

    result << "GL context creator: EGL (new)\n"
	   << "EGL version: " << eglVersion << "\n"
	   << "PNG generator: lodepng\n";

    return result.str();
  }

  bool makeCurrent() const override {
    return eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface, this->eglContext);
  }

  void findPlatformDisplay() {
    std::set<std::string> clientExtensions;
    std::string ext = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    std::istringstream iss(ext);
    while (iss) {
      std::string extension;
      iss >> extension;
      clientExtensions.insert(extension);
    }

    if (clientExtensions.find("EGL_EXT_platform_device") == clientExtensions.end()) {
      return;
    }

    if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
      EGLDeviceEXT eglDevice;
      EGLint numDevices = 0;
      eglQueryDevicesEXT(1, &eglDevice, &numDevices);
      if (numDevices > 0) {
	// FIXME: Attribs
        this->eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevice, nullptr);
      }
    }
  }

  void createSurface(const EGLConfig& config,size_t width, size_t height) {
    const EGLint pbufferAttribs[] = {
      EGL_WIDTH, static_cast<EGLint>(width),
      EGL_HEIGHT, static_cast<EGLint>(height),
      EGL_NONE,
    };
    this->eglSurface = eglCreatePbufferSurface(this->eglDisplay, config, pbufferAttribs);
  }
};

// Typical variants:
// OpenGL core major.minor
// OpenGL compatibility major.minor
// OpenGL ES major.minor
std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(size_t width, size_t height,
							    size_t majorGLVersion, size_t minorGLVersion,
							    bool gles, bool compatibilityProfile)
{
  auto ctx = std::make_shared<OffscreenContextEGL>(width, height);

  int initialEglVersion = gladLoaderLoadEGL(nullptr);
  if (!initialEglVersion) {
    LOG("gladLoaderLoadEGL(NULL): Unable to load EGL");
    return nullptr;
  }
  PRINTDB("GLAD: Loaded EGL %d.%d on first load",
	  GLAD_VERSION_MAJOR(initialEglVersion) % GLAD_VERSION_MINOR(initialEglVersion));

  EGLint conformant;
  if (!gles) conformant = EGL_OPENGL_BIT;
  else if (majorGLVersion >= 3) conformant = EGL_OPENGL_ES3_BIT;
  else if (majorGLVersion >= 2) conformant = EGL_OPENGL_ES2_BIT;
  else conformant = EGL_OPENGL_ES_BIT;

  const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_STENCIL_SIZE, 8,
    EGL_CONFORMANT, conformant,
    EGL_CONFIG_CAVEAT, EGL_NONE,
    EGL_NONE
  };

  // FIXME: Should we try default display first?
  // If so, we also have to try initializing it
  ctx->findPlatformDisplay();
  if (ctx->eglDisplay == EGL_NO_DISPLAY) {
    ctx->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }

  if (ctx->eglDisplay == EGL_NO_DISPLAY) {
    LOG("No EGL display found");
    return nullptr;
  }

  EGLint major, minor;
  if (!eglInitialize(ctx->eglDisplay, &major, &minor)) {
    LOG("Unable to initialize EGL: %1$s", eglGetErrorString(eglGetError()));
    return nullptr;
  }

  PRINTDB("Initialized EGL version: %d.%d (%s)", major % minor % eglQueryString(ctx->eglDisplay, EGL_VENDOR));

  const auto eglVersion = gladLoaderLoadEGL(ctx->eglDisplay);
  if (!eglVersion) {
    LOG("gladLoaderLoadEGL(eglDisplay): Unable to reload EGL");
    return nullptr;
  }
  PRINTDB("GLAD: Loaded EGL %d.%d after reload", GLAD_VERSION_MAJOR(eglVersion) %GLAD_VERSION_MINOR(eglVersion));

  EGLint numConfigs;
  EGLConfig config;
  bool gotConfig = eglChooseConfig(ctx->eglDisplay, configAttribs, &config, 1, &numConfigs);
  if (!gotConfig || numConfigs == 0) {
    LOG("Failed to choose config (eglError: %1$x)", eglGetError());
    return nullptr;
  }
  if (!eglBindAPI(gles ? EGL_OPENGL_ES_API : EGL_OPENGL_API)) {
    LOG("eglBindAPI() failed!");
    return nullptr;
  }

  ctx->createSurface(config, width, height);
  if (ctx->eglSurface == EGL_NO_SURFACE) {
    LOG("Unable to create EGL surface (eglError: %1$x)", eglGetError());
    return nullptr;
  }

  std::vector<EGLint> ctxattr = {
    EGL_CONTEXT_MAJOR_VERSION, static_cast<EGLint>(majorGLVersion),
    EGL_CONTEXT_MINOR_VERSION, static_cast<EGLint>(minorGLVersion),
  };
  if (!gles) {
    ctxattr.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK);
    ctxattr.push_back(compatibilityProfile ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
  }
  ctxattr.push_back(EGL_NONE);
  ctx->eglContext = eglCreateContext(ctx->eglDisplay, config, EGL_NO_CONTEXT, ctxattr.data());
  if (ctx->eglContext == EGL_NO_CONTEXT) {
    LOG("Unable to create EGL context (eglError: %1$x)", eglGetError());
    return nullptr;
  }

  return ctx;
}
