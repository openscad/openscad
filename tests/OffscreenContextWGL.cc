/*

Create an OpenGL context without creating an OpenGL Window. for Windows.

*/

#include <windows.h>

#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"
#include "fbo.h"
#include <vector>
#include <GL/gl.h>

using namespace std;

struct OffscreenContext
{
  HWND window;
  HDC dev_context;
  HGLRC openGLContext;
  int width;
  int height;
  fbo_t *fbo;
};

void offscreen_context_init(OffscreenContext &ctx, int width, int height)
{
  ctx.window = (HWND)NULL;
  ctx.dev_context = (HDC)NULL;
  ctx.openGLContext = (HGLRC)NULL;
  ctx.width = width;
  ctx.height = height;
  ctx.fbo = NULL;
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) 
{
	return DefWindowProc( hwnd, message, wparam, lparam );
}

bool create_wgl_dummy_context(OffscreenContext &ctx)
{
	// create window

        HINSTANCE inst = GetModuleHandle(0);
	WNDCLASS wc;
        ZeroMemory( &wc, sizeof( wc ) );
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.lpszClassName = "OpenSCAD";
	RegisterClass( &wc );
	
	HWND window = CreateWindow( "OpenSCAD", "OpenSCAD", 
		WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
		0, 0, 256, 256,	NULL, NULL, inst, NULL );
	
	
	// create WGL context, make current

	PIXELFORMATDESCRIPTOR pixformat;
	int chosenformat;
	HDC dev_context = GetDC( window );
	ZeroMemory( &pixformat, sizeof( pixformat ) );
	pixformat.nSize = sizeof( pixformat );
	pixformat.nVersion = 1;
	pixformat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pixformat.iPixelType = PFD_TYPE_RGBA;
	pixformat.cColorBits = 24;
	pixformat.cDepthBits = 16;
	pixformat.iLayerType = PFD_MAIN_PLANE;
	chosenformat = ChoosePixelFormat( dev_context, &pixformat );
	SetPixelFormat( dev_context, chosenformat, &pixformat );
	HGLRC gl_render_context = wglCreateContext( dev_context );
	wglMakeCurrent( dev_context, gl_render_context );

	return true;
}


OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  offscreen_context_init( *ctx, w, h );

  // before an FBO can be setup, a WGL context must be created
  // this call alters ctx->window and ctx->openGLContext 
  //  and ctx->dev_context if successfull
  if (!create_wgl_dummy_context( *ctx )) {
    return NULL;
  }

  glewInit(); //must come after Context creation and before FBO calls.
  glewCheck();

  ctx->fbo = fbo_new();
  if (!fbo_init(ctx->fbo, w, h)) {
    return NULL;
  }

  ctx = NULL;
  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  if (ctx) {
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( ctx->openGLContext );
    ReleaseDC( ctx->window, ctx->dev_context );

    fbo_unbind(ctx->fbo);
    fbo_delete(ctx->fbo);
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
  vector<GLubyte> pixels(ctx->width * ctx->height * samplesPerPixel);
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

  // Flip it vertically - images read from OpenGL buffers are upside-down
  int rowBytes = samplesPerPixel * ctx->width;
  unsigned char *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cerr << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  flip_image(&pixels[0], flippedBuffer, samplesPerPixel, ctx->width, ctx->height);

  bool writeok = write_png(filename, flippedBuffer, ctx->width, ctx->height);

  free(flippedBuffer);

  return writeok;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
  if (ctx) fbo_bind(ctx->fbo);
}
