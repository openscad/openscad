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

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "fbo.h"

#include <GL/gl.h>
#include <GL/glx.h>

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
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
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
  Window xWin = XCreateSimpleWindow( dpy, DefaultRootWindow(dpy), 0,0,10,10, 0,0,0 );
  XSync( dpy, false ); 
  if ( XCreateWindow_failed ) { 
    XFree( visinfo );
    XFree( fbconfigs );
    return false;    
  }
  XSetErrorHandler( original_xlib_handler );

  // Most programs would call XMapWindow here. But we don't, to keep the window hidden

  GLXContext context = glXCreateNewContext( dpy, fbconfigs[0], GLX_RGBA_TYPE, NULL, True );
  if ( context == NULL ) {
    cerr << "glXGetVisualFromFBConfig failed\n";
    XDestroyWindow( dpy, xWin );
    XFree( visinfo );
    XFree( fbconfigs );
    return false;
  }  

  if (!glXMakeContextCurrent( dpy, xWin, xWin, context )) {
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

void glewCheck() {
#ifdef DEBUG
  cerr << "GLEW version " << glewGetString(GLEW_VERSION) << "\n";
  cerr << (const char *)glGetString(GL_RENDERER) << "(" << (const char *)glGetString(GL_VENDOR) << ")\n"
       << "OpenGL version " << (const char *)glGetString(GL_VERSION) << "\n";
  cerr  << "Extensions: " << (const char *)glGetString(GL_EXTENSIONS) << "\n";

  if (GLEW_ARB_framebuffer_object) {
    cerr << "ARB_FBO supported\n";
  }
  if (GLEW_EXT_framebuffer_object) {
    cerr << "EXT_FBO supported\n";
  }
  if (GLEW_EXT_packed_depth_stencil) {
    cerr << "EXT_packed_depth_stencil\n";
  }
#endif
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
  glewInit(); 
  if (GLEW_OK != err) {
    fprintf(stderr, "Unable to init GLEW: %s\n", glewGetErrorString(err));
    exit(1);
  }
  glewCheck();

  ctx->fbo = fbo_new();
  if (!fbo_init(ctx->fbo, w, h)) {
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
  if (!ctx || !filename) return false;
  int samplesPerPixel = 4; // R, G, B and A
  GLubyte pixels[ctx->width * ctx->height * samplesPerPixel];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Flip it vertically - images read from OpenGL buffers are upside-down
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cerr << "Unable to allocate flipped buffer for corrected image.";
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
