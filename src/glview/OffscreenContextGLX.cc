#include "glview/OffscreenContextGLX.h"

#include <memory>

#define GLAD_GLX_IMPLEMENTATION
#include <glad/glx.h>

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

#include "utils/scope_guard.hpp"
#include "utils/printutils.h"

namespace {

int xlibLastError = 0;
int xlibErrorHandler(Display *dpy, XErrorEvent *event) {
  xlibLastError = event->error_code;
  return 0;
}

}  // namespace

class OffscreenContextGLX : public OffscreenContext {
public:
  GLXContext glxContext = nullptr;
  Display *display = nullptr;
  Window xWindow = 0;
  OffscreenContextGLX(int width, int height) : OffscreenContext(width, height) {}
  ~OffscreenContextGLX() {
    if (this->display) {
      if (this->glxContext) glXDestroyContext(this->display, this->glxContext);
      if (this->xWindow) XDestroyWindow(this->display, this->xWindow);
      XCloseDisplay(this->display);
    }
  }

  // FIXME: What info are we really interested in here?
  std::string getInfo() const override {
    std::ostringstream result;

    int major, minor;
    glXQueryVersion(this->display, &major, &minor);

    result << "GL context creator: GLX (new)\n"
	         << "GLX version: " << major << "." << minor << "\n"
	         << "PNG generator: lodepng\n";

    return result.str();
  }

  bool makeCurrent() const override {
    return glXMakeContextCurrent(this->display, this->xWindow, this->xWindow, this->glxContext);
  }

  // Create an OpenGL context, and a dummy X11 window to draw into, without showing (mapping) it.
  // This purposely does not use glxCreateWindow, to avoid crashes,
  // "failed to create drawable" errors, and Mesa "WARNING: Application calling
  // GLX 1.3 function when GLX 1.3 is not supported! This is an application bug!"

  //  This function will alter ctx.openGLContext and ctx.xwindow if successful
  bool createGLXContext(size_t majorGLVersion, size_t minorGLVersion, bool compatibilityProfile) {
    const int attributes[] = {
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PIXMAP_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_RED_SIZE, 8,
      GLX_GREEN_SIZE, 8,
      GLX_BLUE_SIZE, 8,
      GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24, // depth-stencil for OpenCSG
      GLX_STENCIL_SIZE, 8,
      None
    };

    int numConfigs = 0;
    GLXFBConfig *fbconfigs = nullptr;
    XVisualInfo *visinfo = nullptr;
    auto guard = sg::make_scope_guard([&fbconfigs, &visinfo]() {
      if (fbconfigs) XFree(fbconfigs);
      if (visinfo) XFree(visinfo);
    });
    fbconfigs = glXChooseFBConfig(this->display, DefaultScreen(this->display), attributes, &numConfigs);
    if (fbconfigs == nullptr) {
      LOG("glXChooseFBConfig() failed");
      return false;
    }
    visinfo = glXGetVisualFromFBConfig(this->display, fbconfigs[0]);
    if (visinfo == nullptr) {
      LOG("glXGetVisualFromFBConfig failed");
      return false;
    }

    // We can't depend on XCreateWindow() returning 0 on failure, so we use a custom Xlib error handler
    XErrorHandler originalErrorHandler = XSetErrorHandler(xlibErrorHandler);
    auto errorGuard = sg::make_scope_guard([originalErrorHandler]() {
      XSetErrorHandler(originalErrorHandler);
    });

    const auto root = DefaultRootWindow(this->display);
    XSetWindowAttributes windowAttributes = {
      .event_mask = StructureNotifyMask | ExposureMask | KeyPressMask,
      .colormap = XCreateColormap(this->display, root, visinfo->visual, AllocNone),
    };
    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    this->xWindow =
      XCreateWindow(this->display, root, 0, 0, this->width(), this->height(), 0,
                    visinfo->depth, InputOutput, visinfo->visual, mask, &windowAttributes);
    XSync(this->display, false);
    if (xlibLastError != Success) {
      char description[1024];
      XGetErrorText(this->display, xlibLastError, description, 1023);
      LOG("XCreateWindow() failed: %1$s", description);
      return false;
    }

    GLint context_attributes[] = {
      GLX_CONTEXT_MAJOR_VERSION_ARB, static_cast<GLint>(majorGLVersion),
      GLX_CONTEXT_MINOR_VERSION_ARB, static_cast<GLint>(minorGLVersion),
      GLX_CONTEXT_PROFILE_MASK_ARB, compatibilityProfile ? GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
      None
    };

    if (glXCreateContextAttribsARB) {
      this->glxContext = glXCreateContextAttribsARB(this->display, fbconfigs[0], nullptr, 1, context_attributes);
      if (!this->glxContext) {
        LOG("Unable to create GLX context using glXCreateContextAttribsARB()");
      }
    }
    if (!this->glxContext) {
      this->glxContext = glXCreateNewContext(this->display, fbconfigs[0], GLX_RGBA_TYPE, nullptr, 1);
      if (!this->glxContext) {
        LOG("Unable to create GLX context using glXCreateNewContext()");
        return false;
      }
    }
    return true;
  }
};


/*
   create a dummy X window without showing it. (without 'mapping' it)
   and save information to the ctx.

   This purposely does not use glxCreateWindow, to avoid crashes,
   "failed to create drawable" errors, and Mesa "WARNING: Application calling
   GLX 1.3 function when GLX 1.3 is not supported! This is an application bug!"

   This function will alter ctx.openGLContext and ctx.xwindow if successful
 */
std::shared_ptr<OffscreenContext> CreateOffscreenContextGLX(size_t width, size_t height,
							    size_t majorGLVersion, size_t minorGLVersion, bool gles, bool compatibilityProfile)
{
  auto ctx = std::make_shared<OffscreenContextGLX>(width, height);

  ctx->display = XOpenDisplay(nullptr);
  if (ctx->display == nullptr) {
    LOG("Unable to open a connection to the X server.");
    char *dpyenv = getenv("DISPLAY");
    LOG("  DISPLAY=%1$s", (dpyenv?dpyenv:""));
    return nullptr;
  }

  int glxVersion = gladLoaderLoadGLX(ctx->display, DefaultScreen(ctx->display));
  if (!glxVersion) {
      LOG("GLAD: Unable to load GLX");
      return nullptr;
  }
  int glxMajor = GLAD_VERSION_MAJOR(glxVersion);
  int glxMinor = GLAD_VERSION_MINOR(glxVersion);
  PRINTDB("GLAD: Loaded GLX %d.%d", glxMajor % glxMinor);

  // We require GLX >= 1.3.
  // However, glxQueryVersion sometimes returns an earlier version than is actually available, so
  // we also accept GLX < 1.3 as long as glXGetVisualFromFBConfig() exists.
  // FIXME: Figure out if this is still relevant with GLAD, as we may want to check functions anyway?
  if (glxMajor == 1 && glxMinor <= 2 && glXGetVisualFromFBConfig == nullptr) {
    LOG("Error: GLX version 1.3 functions missing. Your GLX version: %1$d.%2$d", glxMajor, glxMinor);
    return nullptr;
  }

  if (!ctx->createGLXContext(majorGLVersion, minorGLVersion, compatibilityProfile)) {
    return nullptr;
  }

	return ctx;
}
