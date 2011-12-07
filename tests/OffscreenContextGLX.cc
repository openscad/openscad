/*

Create an OpenGL context without creating an OpenGL Window. for Linux.

See Also

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

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "system-gl.h"
#include "fbo.h"

#include <GL/gl.h>
#include <GL/glx.h>

#include <assert.h>
#include <sstream>

#include <sys/utsname.h> // for uname

using namespace std;

struct OffscreenContext
{
  GLXContext openGLContext;
  Display *xdisplay;
  Window xwindow;
  int width;
  int height;
  fbo_t *fbo;
};

void offscreen_context_init(OffscreenContext &ctx, int width, int height)
{
  ctx.width = width;
  ctx.height = height;
  ctx.openGLContext = NULL;
  ctx.xdisplay = NULL;
  ctx.xwindow = (Window)NULL;
  ctx.fbo = NULL;
}

string get_os_info()
{
  struct utsname u;
  stringstream out;

  if (uname(&u) < 0)
    out << "OS info: unknown, uname() error\n";
  else {
    out << "OS info: "
      << u.sysname << " "
      << u.release << " "
      << u.version << "\n";
    out << "Machine: " << u.machine;
  }
  return out.str();
}

string offscreen_context_getinfo(OffscreenContext *ctx)
{
  assert(ctx);

  if (!ctx->xdisplay)
    return string("No GL Context initialized. No information to report\n");

  int major, minor;
  glXQueryVersion(ctx->xdisplay, &major, &minor);

  stringstream out;
  out << "GL context creator: GLX\n"
      << "PNG generator: lodepng\n"
      << "GLX version: " << major << "." << minor << "\n"
      << get_os_info();

  return out.str();
}

static XErrorHandler original_xlib_handler = (XErrorHandler) NULL;
static bool XCreateWindow_failed = false;
static int XCreateWindow_error(Display *dpy, XErrorEvent *event)
{
  cerr << "XCreateWindow failed: XID: " << event->resourceid
        << " request: " << (int)event->request_code
        << " minor: " << (int)event->minor_code << "\n";
  char description[1024];
  XGetErrorText( dpy, event->error_code, description, 1023 );
  cerr << " error message: " << description << "\n";
  XCreateWindow_failed = true;
  return 0;
}

bool create_glx_dummy_window(OffscreenContext &ctx)
{
  /*
  create a dummy X window without showing it. (without 'mapping' it)
  and save information to the ctx.

  This purposely does not use glxCreateWindow, to avoid crashes,
  "failed to create drawable" errors, and Mesa "WARNING: Application calling 
    GLX 1.3 function when GLX 1.3 is not supported! This is an application bug!"

  This function will alter ctx.openGLContext and ctx.xwindow if successfull
  */

  int attributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT, //support all 3, for OpenCSG
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 24, // depth-stencil for OpenCSG
    GLX_STENCIL_SIZE, 8,
    GLX_DOUBLEBUFFER, True,
    None
  };

  Display *dpy = ctx.xdisplay;

  int num_returned = 0;
  GLXFBConfig *fbconfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy), attributes, &num_returned );
  if ( fbconfigs == NULL ) {
    cerr << "glXChooseFBConfig failed\n";
    return false;
  }

  XVisualInfo *visinfo = glXGetVisualFromFBConfig( dpy, fbconfigs[0] );
  if ( visinfo == NULL ) {
    cerr << "glXGetVisualFromFBConfig failed\n";
    XFree( fbconfigs );
    return false;
  }

  // can't depend on xWin==NULL at failure. use a custom Xlib error handler instead.
  original_xlib_handler = XSetErrorHandler( XCreateWindow_error );

  Window root = DefaultRootWindow( dpy );
  XSetWindowAttributes xwin_attr;
  int width = ctx.width;
  int height = ctx.height;
  xwin_attr.background_pixmap = None;
  xwin_attr.background_pixel = 0;
  xwin_attr.border_pixel = 0;
  xwin_attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
  xwin_attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
  unsigned long int mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  Window xWin = XCreateWindow( dpy, root, 0, 0, width, height,
                               0, visinfo->depth, InputOutput,
                               visinfo->visual, mask, &xwin_attr );

  // Window xWin = XCreateSimpleWindow( dpy, DefaultRootWindow(dpy), 0,0,42,42, 0,0,0 );

  XSync( dpy, false );
  if ( XCreateWindow_failed ) {
    XFree( visinfo );
    XFree( fbconfigs );
    return false;
  }
  XSetErrorHandler( original_xlib_handler );

  // Most programs would call XMapWindow here. But we don't, to keep the window hidden
  // XMapWindow( dpy, xWin );

  GLXContext context = glXCreateNewContext( dpy, fbconfigs[0], GLX_RGBA_TYPE, NULL, True );
  if ( context == NULL ) {
    cerr << "glXCreateNewContext failed\n";
    XDestroyWindow( dpy, xWin );
    XFree( visinfo );
    XFree( fbconfigs );
    return false;
  }

  //GLXWindow glxWin = glXCreateWindow( dpy, fbconfigs[0], xWin, NULL );

  if (!glXMakeContextCurrent( dpy, xWin, xWin, context )) {
  //if (!glXMakeContextCurrent( dpy, glxWin, glxWin, context )) {
    cerr << "glXMakeContextCurrent failed\n";
    glXDestroyContext( dpy, context );
    XDestroyWindow( dpy, xWin );
    XFree( visinfo );
    XFree( fbconfigs );
    return false;
  }

  ctx.openGLContext = context;
  ctx.xwindow = xWin;

  XFree( visinfo );
  XFree( fbconfigs );

  return true;
}


Bool create_glx_dummy_context(OffscreenContext &ctx)
{
  // This will alter ctx.openGLContext and ctx.xdisplay and ctx.xwindow if successfull
  int major;
  int minor;
  Bool result = False;

  ctx.xdisplay = XOpenDisplay( NULL );
  if ( ctx.xdisplay == NULL ) {
    cerr << "Unable to open a connection to the X server\n";
    return False;
  }

  // glxQueryVersion is not always reliable. Use it, but then
  // also check to see if GLX 1.3 functions exist

  glXQueryVersion(ctx.xdisplay, &major, &minor);

  if ( major==1 && minor<=2 && glXGetVisualFromFBConfig==NULL ) {
    cerr << "Error: GLX version 1.3 functions missing. "
        << "Your GLX version: " << major << "." << minor << endl;
  } else {
    result = create_glx_dummy_window(ctx);
  }

  if (!result) XCloseDisplay( ctx.xdisplay );
  return result;
}

OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  offscreen_context_init( *ctx, w, h );

  // before an FBO can be setup, a GLX context must be created
  // this call alters ctx->xDisplay and ctx->openGLContext 
  //  and ctx->xwindow if successfull
  if (!create_glx_dummy_context( *ctx )) {
    return NULL;
  }

  // glewInit must come after Context creation and before FBO calls.
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    cerr << "Unable to init GLEW: " << glewGetErrorString(err) << endl;
    return NULL;
  }

  ctx->fbo = fbo_new();
  if (!fbo_init(ctx->fbo, w, h)) {
    cerr << "GL Framebuffer Object init failed; dumping GLEW info" << endl;
    return NULL;
  }

  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  if (ctx) {
    fbo_unbind(ctx->fbo);
    fbo_delete(ctx->fbo);
    XDestroyWindow( ctx->xdisplay, ctx->xwindow );
    glXDestroyContext( ctx->xdisplay, ctx->openGLContext );
    XCloseDisplay( ctx->xdisplay );
    return true;
  }
  return false;
}

/*!
  Capture framebuffer from OpenGL and write it to the given filename as PNG.
*/
bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  glXSwapBuffers(ctx->xdisplay, ctx->xwindow);
  if (!ctx || !filename) return false;
  int samplesPerPixel = 4; // R, G, B and A
  GLubyte pixels[ctx->width * ctx->height * samplesPerPixel];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Flip it vertically - images read from OpenGL buffers are upside-down
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    cerr << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  flip_image(pixels, flippedBuffer, samplesPerPixel, ctx->width, ctx->height);

  bool writeok = write_png(filename, flippedBuffer, ctx->width, ctx->height);

  free(flippedBuffer);

  return writeok;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
  if (ctx) fbo_bind(ctx->fbo);
}
