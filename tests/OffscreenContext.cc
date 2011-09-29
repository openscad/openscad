#include "OffscreenContext.h"
#include "printutils.h"
#include "imageutils.h"

// see http://www.gamedev.net/topic/552607-conflict-between-glew-and-sdl/
#define NO_SDL_GLEXT
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

//#include <GL/gl.h>
//#include <GL/glu.h>      // for gluCheckExtension
#include <SDL.h>

struct OffscreenContext
{
  int width;
  int height;
  GLuint fbo;
  GLuint colorbo;
  GLuint depthbo;
};

void write_targa(const char *filename, GLubyte *pixels, int width, int height)
{
	FILE *f = fopen( filename, "w" );
	int y;
	if (f) {
		GLubyte header[] = {
			00,00,02, 00,00,00, 00,00,00, 00,00,00,
			0xff & width, 0xff & width >> 8,
			0xff & height, 0xff & height >> 8,
			32, 0x20 }; // next-to-last = bit depth
		fwrite( header, sizeof(header), 1, f);
		for (y=height-1; y>=0; y--)
			fwrite( pixels + y*width*4, 4, width, f);
		fclose(f);
	}
}

OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  ctx->width = w;
  ctx->height = h;

  // dummy window 
  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetVideoMode(ctx->width,ctx->height,32,SDL_OPENGL);

  // must come after openGL context init (done by dummy window)
  // but must also come before various EXT calls
  //glewInit();

/*
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f( 1, 0, 0);
	glVertex3f( 0, 0, 0);
	glVertex3f( 1, 0, 0);
	glVertex3f( 0, 1, 0);
	glEnd();
        SDL_GL_SwapBuffers();
//  sleep(2);
*/

  int samplesPerPixel = 4; // R, G, B and A

/*  char * filename = "blah.tga";
  GLubyte pixels[ ctx->width * ctx->height * samplesPerPixel ];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
  printf("writing %s\n",filename);
  write_targa(filename,pixels,ctx->width, ctx->height);*/
  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  return true;
}

/*!
  Capture framebuffer from OpenGL and write it to the given filename as PNG.
*/
bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  SDL_GL_SwapBuffers(); // show image

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
}
