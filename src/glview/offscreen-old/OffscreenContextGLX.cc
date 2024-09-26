/*

   Create an OpenGL context without creating an OpenGL Window. for Linux.

   See also

   glxgears.c by Brian Paul from mesa-demos (mesa3d.org)
   http://cgit.freedesktop.org/mesa/demos/tree/src/xdemos?id=mesa-demos-8.0.1
   http://www.opengl.org/sdk/docs/man/xhtml/glXIntro.xml
   http://www.mesa3d.org/brianp/sig97/offscrn.htm
   http://glprogramming.com/blue/ch07.html
   OffscreenContext.mm (Mac OSX version)

 */

/*
 * Some portions of the code below are:
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "glview/offscreen-old/OffscreenContextGLX.h"


#include "glview/system-gl.h"
#include <iostream>
#include <cstdint>
#include <memory>
#include <GL/gl.h>
#include <GL/glx.h>

#include <cassert>
#include <sstream>
#include <string>
#include <sys/utsname.h> // for uname

#include "utils/printutils.h"

namespace {

class OffscreenContextGLX : public OffscreenContext {
public:
  OffscreenContextGLX(uint32_t width, uint32_t height) : OffscreenContext(width, height) {}
  ~OffscreenContextGLX() {
    if (this->xwindow) XDestroyWindow(this->xdisplay, this->xwindow);
    if (this->openGLContext) glXDestroyContext(this->xdisplay, this->openGLContext);
    if (this->xdisplay) XCloseDisplay(this->xdisplay);
  }

  std::string getInfo() const override {
    if (!this->xdisplay) {
      return {"No GL Context initialized. No information to report\n"};
    }

    std::ostringstream result;

    int major, minor;
    glXQueryVersion(this->xdisplay, &major, &minor);

    result << "GL context creator: GLX (old)\n"
    << "GLX version: " << major << "." << minor << "\n"
    << "PNG generator: lodepng\n";

    return result.str();
  }

  bool makeCurrent() const override {
    return glXMakeContextCurrent(this->xdisplay, this->xwindow, this->xwindow, this->openGLContext);
  }

  GLXContext openGLContext{nullptr};
  Display *xdisplay{nullptr};
  Window xwindow{0};
};

static XErrorHandler original_xlib_handler = nullptr;
static auto XCreateWindow_failed = false;
static int XCreateWindow_error(Display *dpy, XErrorEvent *event)
{
  std::cerr << "XCreateWindow failed: XID: " << event->resourceid
            << " request: " << static_cast<int>(event->request_code)
            << " minor: " << static_cast<int>(event->minor_code) << "\n";
  char description[1024];
  XGetErrorText(dpy, event->error_code, description, 1023);
  std::cerr << " error message: " << description << "\n";
  XCreateWindow_failed = true;
  return 0;
}

/*
   create a dummy X window without showing it. (without 'mapping' it)
   and save information to the ctx.

   This purposely does not use glxCreateWindow, to avoid crashes,
   "failed to create drawable" errors, and Mesa "WARNING: Application calling
   GLX 1.3 function when GLX 1.3 is not supported! This is an application bug!"

   This function will alter ctx.openGLContext and ctx.xwindow if successful
 */
bool create_glx_dummy_window(OffscreenContextGLX& ctx)
{
  int attributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT, //support all 3, for OpenCSG
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 24, // depth-stencil for OpenCSG
    GLX_STENCIL_SIZE, 8,
    GLX_DOUBLEBUFFER, true,
    None
  };

  auto dpy = ctx.xdisplay;

  int num_returned = 0;
  auto fbconfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attributes, &num_returned);
  if (fbconfigs == nullptr) {
    std::cerr << "glXChooseFBConfig failed\n";
    return false;
  }

  auto visinfo = glXGetVisualFromFBConfig(dpy, fbconfigs[0]);
  if (visinfo == nullptr) {
    std::cerr << "glXGetVisualFromFBConfig failed\n";
    XFree(fbconfigs);
    return false;
  }

  // can't depend on xWin==nullptr at failure. use a custom Xlib error handler instead.
  original_xlib_handler = XSetErrorHandler(XCreateWindow_error);

  auto root = DefaultRootWindow(dpy);
  XSetWindowAttributes xwin_attr;
  auto width = ctx.width();
  auto height = ctx.height();
  xwin_attr.background_pixmap = None;
  xwin_attr.background_pixel = 0;
  xwin_attr.border_pixel = 0;
  xwin_attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
  xwin_attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
  unsigned long int mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  auto xWin = XCreateWindow(dpy, root, 0, 0, width, height,
                            0, visinfo->depth, InputOutput,
                            visinfo->visual, mask, &xwin_attr);

  // Window xWin = XCreateSimpleWindow( dpy, DefaultRootWindow(dpy), 0,0,42,42, 0,0,0 );

  XSync(dpy, false);
  if (XCreateWindow_failed) {
    XFree(visinfo);
    XFree(fbconfigs);
    return false;
  }
  XSetErrorHandler(original_xlib_handler);

  // Most programs would call XMapWindow here. But we don't, to keep the window hidden
  // XMapWindow( dpy, xWin );

  auto context = glXCreateNewContext(dpy, fbconfigs[0], GLX_RGBA_TYPE, nullptr, true);
  if (context == nullptr) {
    std::cerr << "glXCreateNewContext failed\n";
    XDestroyWindow(dpy, xWin);
    XFree(visinfo);
    XFree(fbconfigs);
    return false;
  }

  //GLXWindow glxWin = glXCreateWindow( dpy, fbconfigs[0], xWin, nullptr );

  ctx.openGLContext = context;
  ctx.xwindow = xWin;

  XFree(visinfo);
  XFree(fbconfigs);

  return true;
}


#pragma GCC diagnostic ignored "-Waddress"
bool create_glx_dummy_context(OffscreenContextGLX& ctx)
{
  // This will alter ctx.openGLContext and ctx.xdisplay and ctx.xwindow if successful
  int major;
  int minor;
  auto result = false;

  ctx.xdisplay = XOpenDisplay(nullptr);
  if (ctx.xdisplay == nullptr) {
    std::cerr << "Unable to open a connection to the X server.\n";
    auto dpyenv = getenv("DISPLAY");
    std::cerr << "DISPLAY=" << (dpyenv?dpyenv:"") << "\n";
    return false;
  }

  // glxQueryVersion is not always reliable. Use it, but then
  // also check to see if GLX 1.3 functions exist

  glXQueryVersion(ctx.xdisplay, &major, &minor);
  if (major == 1 && minor <= 2 && glXGetVisualFromFBConfig == nullptr) {
    std::cerr << "Error: GLX version 1.3 functions missing. "
              << "Your GLX version: " << major << "." << minor << std::endl;
  } else {
    result = create_glx_dummy_window(ctx);
  }

  if (!result) XCloseDisplay(ctx.xdisplay);
  return result;
}

}  // namespace

namespace offscreen_old {

std::shared_ptr<OffscreenContext> CreateOffscreenContextGLX(
  uint32_t width, uint32_t height, uint32_t majorGLVersion, 
  uint32_t minorGLVersion, bool gles, bool compatibilityProfile)   
{
  auto ctx = std::make_shared<OffscreenContextGLX>(width, height);

  // before an FBO can be setup, a GLX context must be created
  // this call alters ctx->xDisplay and ctx->openGLContext
  // and ctx->xwindow if successful
  if (!create_glx_dummy_context(*ctx)) {
    return nullptr;
  }

  return ctx;
}

}  // namespace offscreen_old
