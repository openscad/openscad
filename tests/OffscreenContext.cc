#include "OffscreenContext.h"
#include "printutils.h"
#include "lodepng.h"

// see http://www.gamedev.net/topic/552607-conflict-between-glew-and-sdl/
#define NO_SDL_GLEXT
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

//#include <GL/gl.h>
//#include <GL/glu.h>      // for gluCheckExtension
#include <SDL.h>

// Simple error reporting macros to help keep the sample code clean
#define REPORT_ERROR_AND_EXIT(desc) { std::cout << desc << "\n"; return false; }
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

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

void write_png(const char *filename, GLubyte *pixels, int width, int height)
{
	size_t pixel_size = 4;
	size_t dataout_size = -1;
	GLubyte *dataout = (GLubyte*)malloc(width*height*pixel_size); // freed below
	GLubyte *pixels_flipped = (GLubyte*)malloc(width*height*pixel_size); // freed below
	for (int y=0;y<height;y++) {
		for (int x=0;x<width;x++) {
			int offs1 = y*width*pixel_size + x*pixel_size;
			int offs2 = (height-1-y)*width*pixel_size + x*pixel_size;
			pixels_flipped[offs1  ] = pixels[offs2  ];
			pixels_flipped[offs1+1] = pixels[offs2+1];
			pixels_flipped[offs1+2] = pixels[offs2+2];
			pixels_flipped[offs1+3] = pixels[offs2+3];
		}
	}
	//encoder.settings.zlibsettings.windowSize = 2048;
	//LodePNG_Text_add(&encoder.infoPng.text, "Comment", "Created with LodePNG");

	LodePNG_encode(&dataout, &dataout_size, pixels_flipped, width, height, LCT_RGBA, 8);
	//LodePNG_saveFile(dataout, dataout_size, "blah2.png");
	FILE *f = fopen( filename, "w" );
	if (f) {
		fwrite( dataout, 1, dataout_size, f);
		fclose(f);
	}
	free(pixels_flipped);
	free(dataout);
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
  //  Test if framebuffer objects are supported
  const GLubyte* strExt = glGetString(GL_EXTENSIONS);
  GLboolean fboSupported = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", strExt);
  if (!fboSupported)
    REPORT_ERROR_AND_EXIT("Your system does not support framebuffer extension - unable to render scene");

  printf("%i\n", (int)glGenFramebuffersEXT);
  GLuint fbo;
  //ctx->fbo = 0;
  glGenFramebuffersEXT(1, &fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  REPORTGLERROR("binding framebuffer");


  GLuint renderBuffer = 0;
  GLuint depthBuffer = 0;
  // Depth buffer to use for depth testing - optional if you're not using depth testing
  glGenRenderbuffersEXT(1, &depthBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,  w, h);
  REPORTGLERROR("creating depth render buffer");

  // Render buffer to use for imaging
  glGenRenderbuffersEXT(1, &renderBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, w, h);
  REPORTGLERROR("creating color render buffer");

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                               GL_RENDERBUFFER_EXT, renderBuffer);
  REPORTGLERROR("specifying color render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != 
      GL_FRAMEBUFFER_COMPLETE_EXT)
    REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer after specifying color render buffer.");

  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
                               GL_RENDERBUFFER_EXT, depthBuffer);
  REPORTGLERROR("specifying depth render buffer");

  if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != 
      GL_FRAMEBUFFER_COMPLETE_EXT)
    REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer after specifying depth render buffer.");
*/
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
  // "un"bind my FBO
//  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  /*
   * Cleanup
   */
  return true;
}

bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  /*
   * Extract the resulting rendering as an image
   */

	SDL_GL_SwapBuffers(); // show image
	int samplesPerPixel = 4; // R, G, B and A

  GLubyte pixels[ ctx->width * ctx->height * samplesPerPixel ];
  glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  //char * filename2="blah2.tga";
  //PRINTF("writing %s\n",filename2);
  //write_targa(filename2,pixels,ctx->width, ctx->height);
  char * filename2="blah2.png";
  PRINTF("writing %s . . .",filename);
  //write_targa(filename2,pixels,ctx->width, ctx->height);
  write_png(filename,pixels,ctx->width, ctx->height);
  PRINTF("written\n");

  return true;
}

void bind_offscreen_context(OffscreenContext *ctx)
{
//  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->fbo);
}
