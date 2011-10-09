/*

Create an OpenGL context without creating an OpenGL Window. for Linux.

based on

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

#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }

struct OffscreenContext
{
  GLXContext openGLContext;
  Display *xDisplay;
  GLXPixmap glx_pixmap;
  Pixmap x11_pixmap;
  int width;
  int height;
  fbo_t *fbo;
};

Bool glx_1_3_pixmap_dummy_context(OffscreenContext *ctx, Bool hybrid)
{
	XVisualInfo *vInfo;
	GLXFBConfig *fbConfigs;

	int numReturned;
	int result;
	int dummyAttributes_1_3[] = {
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		None
	};

	fbConfigs = glXChooseFBConfig( ctx->xDisplay,
	 	DefaultScreen(ctx->xDisplay),
		dummyAttributes_1_3, &numReturned );
	if ( fbConfigs == NULL ) {
		REPORTGLERROR("glXChooseFBConfig failed") ;
		return False;
	}

	vInfo = glXGetVisualFromFBConfig( ctx->xDisplay, fbConfigs[0] );
	if ( vInfo == NULL ) {
		REPORTGLERROR("glXGetVisualFromFBConfig failed") ;
		return False;
	}

	ctx->x11_pixmap = XCreatePixmap( ctx->xDisplay, DefaultRootWindow(ctx->xDisplay) , 10, 10, 8 );

	if (hybrid) {
		// GLX 1.2 - prevent Mesa warning
		ctx->glx_pixmap = glXCreateGLXPixmap( ctx->xDisplay, vInfo, ctx->x11_pixmap );
	} else {
		// GLX 1.3
		ctx->glx_pixmap = glXCreatePixmap( ctx->xDisplay, fbConfigs[0], ctx->x11_pixmap, NULL ); // GLX 1.3
	}

	ctx->openGLContext = glXCreateNewContext( ctx->xDisplay, fbConfigs[0], GLX_RGBA_TYPE, NULL, True );
	if ( ctx->openGLContext == NULL ) {
		REPORTGLERROR("glXCreateNewContext failed" );
		return False;
	}

	result = glXMakeContextCurrent( ctx->xDisplay, ctx->glx_pixmap, ctx->glx_pixmap, ctx->openGLContext );
	if ( result == False ) {
		REPORTGLERROR("glXMakeContextCurrent failed" );
		return False;
	}

	return True;
}

Bool make_glx_dummy_context(OffscreenContext *ctx)
{
	/*
	Before opening a framebuffer, an OpenGL context must be created.
	For GLX, you can do this by creating a 'Pixmap' drawable then
	creating the Context off of that. The Pixmap is then never used.
	*/
	int major;
	int minor;

	ctx->xDisplay = XOpenDisplay( NULL );
	if ( ctx->xDisplay == NULL ) {
		fprintf(stderr, "Unable to open a connection to the X server\n" );
		return False;
	}

	/*
	On some systems, the GLX library will report that it is version
	1.2, but some 1.3 functions will be implemented, and, furthermore,
	some 1.2 functions won't work right, while the 1.3 functions will,
	but glXCreatePixmp will still generate a MESA GLX 1.3 runtime warning.

	To workaround this, detect the situation and use 'hybrid' mix of
	1.2 and 1.3 as needed.
	*/
	glXQueryVersion(ctx->xDisplay, &major, &minor);

	if (major==1 && minor<=2) {
		if (glXCreatePixmap!=NULL) { // 1.3 function exists, even though its 1.2
			return glx_1_3_pixmap_dummy_context(ctx,True);
		} else {
			fprintf(stderr,"OpenGL error: GLX version 1.3 functions missing. Your GLX: %i.%i\n",major,minor);
			return False;
		}
	} else if (major>=1 && minor>=3) {
		return glx_1_3_pixmap_dummy_context(ctx,False);
	}
}

void glewCheck() {
#ifdef DEBUG
  cout << "GLEW version " << glewGetString(GLEW_VERSION) << "\n";
  cout << (const char *)glGetString(GL_RENDERER) << "(" << (const char *)glGetString(GL_VENDOR) << ")\n"
       << "OpenGL version " << (const char *)glGetString(GL_VERSION) << "\n";
  cout  << "Extensions: " << (const char *)glGetString(GL_EXTENSIONS) << "\n";

  if (GLEW_ARB_framebuffer_object) {
    cout << "ARB_FBO supported\n";
  }
  if (GLEW_EXT_framebuffer_object) {
    cout << "EXT_FBO supported\n";
  }
  if (GLEW_EXT_packed_depth_stencil) {
    cout << "EXT_packed_depth_stencil\n";
  }
#endif
}

OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  ctx->width = w;
  ctx->height = h;
  ctx->openGLContext = NULL;
  ctx->xDisplay = NULL;
  ctx->glx_pixmap = NULL;
  ctx->x11_pixmap = NULL;
  ctx->fbo = NULL;

  // fill ctx->xDisplay, ctx->openGLContext, x11_pixmap, glx_pixmap
  if (!make_glx_dummy_context(ctx)) {
    return NULL;
  }

  glewInit(); //must come after Context creation and before FBO calls.
  glewCheck();

  ctx->fbo = fbo_new();
  if (!fbo_init(ctx->fbo, w, h)) {
    return NULL;
  }

  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
	fbo_unbind(ctx->fbo);
	fbo_delete(ctx->fbo);
	glXDestroyPixmap(ctx->xDisplay, ctx->glx_pixmap );
	XFreePixmap(ctx->xDisplay, ctx->x11_pixmap );
	glXDestroyContext( ctx->xDisplay, ctx->openGLContext );
	return true;
}

/*!
  Capture framebuffer from OpenGL and write it to the given filename as PNG.
*/
bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  int samplesPerPixel = 4; // R, G, B and A
  GLubyte pixels[ctx->width * ctx->height * samplesPerPixel];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Flip it vertically - images read from OpenGL buffers are upside-down
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cout << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  flip_image(pixels, flippedBuffer, samplesPerPixel, ctx->width, ctx->height);

  bool writeok = write_png(filename, flippedBuffer, ctx->width, ctx->height);

  free(flippedBuffer);

  return writeok;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
	fbo_bind(ctx->fbo);
}
