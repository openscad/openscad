#include "OffscreenContextEGL.h"

#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>
#ifdef HAS_GBM
#include <gbm.h>
#endif
#define GLAD_EGL_IMPLEMENTATION
#include "glad/egl.h"
#include "GL/gl.h"

namespace {

#define CASE_STR( value ) case value: return #value; 
const char* eglGetErrorString( EGLint error )
{
    switch( error )
    {
    CASE_STR( EGL_SUCCESS             )
    CASE_STR( EGL_NOT_INITIALIZED     )
    CASE_STR( EGL_BAD_ACCESS          )
    CASE_STR( EGL_BAD_ALLOC           )
    CASE_STR( EGL_BAD_ATTRIBUTE       )
    CASE_STR( EGL_BAD_CONTEXT         )
    CASE_STR( EGL_BAD_CONFIG          )
    CASE_STR( EGL_BAD_CURRENT_SURFACE )
    CASE_STR( EGL_BAD_DISPLAY         )
    CASE_STR( EGL_BAD_SURFACE         )
    CASE_STR( EGL_BAD_MATCH           )
    CASE_STR( EGL_BAD_PARAMETER       )
    CASE_STR( EGL_BAD_NATIVE_PIXMAP   )
    CASE_STR( EGL_BAD_NATIVE_WINDOW   )
    CASE_STR( EGL_CONTEXT_LOST        )
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

// If eglDisplay is backed by a GBM device.
  struct gbm_device *gbmDevice = nullptr;

  OffscreenContextEGL(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextEGL() {
    // FIXME: Free resources
  }

  std::string getInfo() const override {
    std::ostringstream result;

    // FIXME: Version
    result << "GL context creator: EGL (new)\n"
	         << "PNG generator: lodepng\n";

    return result.str();
  }

  bool makeCurrent() const override {
    eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface, this->eglContext);
    return true;
  }

#ifdef HAS_GBM
  void getDisplayFromDrmNode(const std::string& drmNode) {
    this->eglDisplay = EGL_NO_DISPLAY;
    const int fd = open(drmNode.c_str(), O_RDWR);
    if (fd < 0) {
      std::cerr << "Unable to open DRM node " << drmNode << std::endl;
      return;
    }

    this->gbmDevice = gbm_create_device(fd);
    if (!this->gbmDevice) {
      std::cerr << "Unable to create GDM device" << std::endl;
      return;
    }

    // FIXME: Check EGL extension before passing the identifier to this function
    this->eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, this->gbmDevice, nullptr);
  }
#endif

  void findPlatformDisplay() {
    std::set<std::string> clientExtensions;
    std::string ext = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    std::cout << ext << std::endl;
    std::istringstream iss(ext);
    while (iss) {
      std::string extension;
      iss >> extension;
      clientExtensions.insert(extension);
    }

    if (clientExtensions.find("EGL_EXT_platform_device") == clientExtensions.end()) {
      return;
    }

    std::cout << "Trying Platform display..." << std::endl;
    if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
      EGLDeviceEXT eglDevice;
      EGLint numDevices = 0;
      eglQueryDevicesEXT(1, &eglDevice, &numDevices);
      if (numDevices > 0) {
      // FIXME: Attribs
        this->eglDisplay =  eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevice, nullptr);
      }
    }
  }

  void createSurface(const EGLConfig& config,size_t width, size_t height) {
    if (this->gbmDevice) {
#ifdef HAS_GBM
// FIXME: For some reason, we have to pass 0 as flags for the nvidia GBM backend
      const auto gbmSurface =
        gbm_surface_create(this->gbmDevice, width, height,
                           GBM_FORMAT_ARGB8888, 
                           0); // GBM_BO_USE_RENDERING
      if (!gbmSurface) {
        std::cerr << "Unable to create GBM surface" << std::endl;
        this->eglSurface = EGL_NO_SURFACE;
        return;
      }

      this->eglSurface =
        eglCreatePlatformWindowSurface(this->eglDisplay, config, gbmSurface, nullptr);
#endif
    } else {
      const EGLint pbufferAttribs[] = {
        EGL_WIDTH, static_cast<EGLint>(width),
        EGL_HEIGHT, static_cast<EGLint>(height),
        EGL_NONE,
      };
      this->eglSurface = eglCreatePbufferSurface(this->eglDisplay, config, pbufferAttribs);
    }
  }
};

// Typical variants:
// OpenGL core major.minor
// OpenGL compatibility major.minor
// OpenGL ES major.minor
std::shared_ptr<OffscreenContext> CreateOffscreenContextEGL(size_t width, size_t height,
							       size_t majorGLVersion, size_t minorGLVersion, bool gles, bool compatibilityProfile,
                 const std::string &drmNode)
{
  auto ctx = std::make_shared<OffscreenContextEGL>(width, height);

  int initialEglVersion = gladLoaderLoadEGL(NULL);
  if (!initialEglVersion) {
    std::cerr << "gladLoaderLoadEGL(NULL): Unable to load EGL" << std::endl;
    return nullptr;
  }
  std::cout << "Loaded EGL " << GLAD_VERSION_MAJOR(initialEglVersion) << "."
    << GLAD_VERSION_MINOR(initialEglVersion) << " on first load." << std::endl;
  
  EGLint conformant;
  if (!gles) conformant = EGL_OPENGL_BIT;
  else if (majorGLVersion >= 3) conformant = EGL_OPENGL_ES3_BIT;
  else if (majorGLVersion >= 2) conformant = EGL_OPENGL_ES2_BIT;
  else conformant = EGL_OPENGL_ES_BIT;

  const EGLint configAttribs[] = {
    // For some reason, we have to request a "window" surface when using GBM, although
    // we're rendering offscreen
    EGL_SURFACE_TYPE, drmNode.empty() ? EGL_PBUFFER_BIT : EGL_WINDOW_BIT,
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

  if (!drmNode.empty()) {
#ifdef HAS_GBM
    std::cout << "Using GBM..." << std::endl;
    ctx->getDisplayFromDrmNode(drmNode);
#endif
  } else {
    // FIXME: Should we try default display first?
    // If so, we also have to try initializing it
    ctx->findPlatformDisplay();
    if (ctx->eglDisplay == EGL_NO_DISPLAY) {
      std::cout << "Trying default EGL display..." << std::endl;
      ctx->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
  }

  if (ctx->eglDisplay == EGL_NO_DISPLAY) {
    std::cerr << "No EGL display found" << std::endl;
    return nullptr;
  }

  EGLint major, minor;
  if (!eglInitialize(ctx->eglDisplay, &major, &minor)) {
    std::cerr << "Unable to initialize EGL: " << eglGetErrorString(eglGetError()) << std::endl;
    return nullptr;
  }

  std::cout << "EGL Version: " << major << "." << minor << " (" << eglQueryString(ctx->eglDisplay, EGL_VENDOR) << ")" << std::endl;

  const auto eglVersion = gladLoaderLoadEGL(ctx->eglDisplay);
  if (!eglVersion) {
    std::cerr << "gladLoaderLoadEGL(eglDisplay): Unable to reload EGL" << std::endl;
    return nullptr;
  }
  std::cout << "Loaded EGL " << GLAD_VERSION_MAJOR(eglVersion) << "." << GLAD_VERSION_MINOR(eglVersion) << " after reload" << std::endl;

  if (eglGetDisplayDriverName) {
    const char *name = eglGetDisplayDriverName(ctx->eglDisplay);
    if (name) {
      std::cout << "Got EGL display with driver name: " << name << std::endl;
    }
  }

  EGLint numConfigs;
  EGLConfig config;
  bool gotConfig = eglChooseConfig(ctx->eglDisplay, configAttribs, &config, 1, &numConfigs);
  if (!gotConfig || numConfigs == 0) {
    std::cerr << "Failed to choose config (eglError: " << std::hex << eglGetError() << ")" << std::endl;
    return nullptr;
  }
  if (!eglBindAPI(gles ? EGL_OPENGL_ES_API : EGL_OPENGL_API)) {
    std::cerr << "eglBindAPI() failed!" << std::endl;
    return nullptr;
  }

  ctx->createSurface(config, width, height);    
  if (ctx->eglSurface == EGL_NO_SURFACE) {
    std::cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << std::endl;
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
    std::cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << std::endl;
    return nullptr;
  }

  return ctx;
}
