#include "OffscreenContext.h"

#import <OpenGL/OpenGL.h>
#import <OpenGL/glu.h>      // for gluCheckExtension
#import <AppKit/AppKit.h>   // for NSOpenGL...

// Simple error reporting macros to help keep the sample code clean
#define REPORT_ERROR_AND_EXIT(desc) { std::cout << desc << "\n"; return false; }
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

struct OffscreenContext
{
  NSOpenGLContext *openGLContext;
  NSAutoreleasePool *pool;
  int width;
  int height;
};


OffscreenContext *create_offscreen_context(int w, int h)
{
  OffscreenContext *ctx = new OffscreenContext;
  ctx->width = w;
  ctx->height = h;

  ctx->pool = [NSAutoreleasePool new];

  /*
   * Create an OpenGL context just so that OpenGL calls will work. I'm not 
   using it for actual rendering.
  */
                                   
  NSOpenGLPixelFormatAttribute    attributes[] = {
    NSOpenGLPFAPixelBuffer,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 24,
    (NSOpenGLPixelFormatAttribute) 0
  };
  NSOpenGLPixelFormat*            pixFormat = [[[NSOpenGLPixelFormat 
                                                 alloc] initWithAttributes:attributes] autorelease];
  // Create the OpenGL context to render with (with color and depth buffers)
  ctx->openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat 
                   shareContext:nil];
  NULL_ERROR_EXIT(ctx->openGLContext, "Unable to create NSOpenGLContext");

  [ctx->openGLContext makeCurrentContext];

  /*
   * Test if framebuffer objects are supported
   */
  const GLubyte* strExt = glGetString(GL_EXTENSIONS);
  GLboolean fboSupported = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", strExt);
  if (!fboSupported)
    REPORT_ERROR_AND_EXIT("Your system does not support framebuffer extension - unable to render scene");
  /*
   * Create an FBO
   */
  GLuint  renderBuffer = 0;
  GLuint  depthBuffer = 0;
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
  GLuint  fbo = 0;
  glGenFramebuffersEXT(1, &fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  REPORTGLERROR("binding framebuffer");

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

  return ctx;
}

bool teardown_offscreen_context(OffscreenContext *ctx)
{
  // "un"bind my FBO
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  /*
   * Cleanup
   */
  [ctx->openGLContext clearDrawable];
  [ctx->openGLContext release];

  [ctx->pool release];
  return true;
}

bool save_framebuffer(OffscreenContext *ctx, const char *filename)
{
  /*
   * Extract the resulting rendering as an image
   */

  int samplesPerPixel = 4; // R, G, B and A
  int rowBytes = samplesPerPixel * ctx->width;
  char* bufferData = (char*)malloc(rowBytes * ctx->height);
  if (!bufferData) {
    std::cerr << "Unable to allocate buffer for image extraction.";
    return 1;
  }
  glReadPixels(0, 0, ctx->width, ctx->height, GL_BGRA, GL_UNSIGNED_BYTE, 
               bufferData);
  REPORTGLERROR("reading pixels from framebuffer");
  
  // Flip it vertically - images read from OpenGL buffers are upside-down
  char* flippedBuffer = (char*)malloc(rowBytes * ctx->height);
  if (!flippedBuffer) {
    std::cout << "Unable to allocate flipped buffer for corrected image.";
    return 1;
  }
  for (int i = 0 ; i < ctx->height ; i++) {
    bcopy(bufferData + i * rowBytes, flippedBuffer + (ctx->height - i - 1) * 
          rowBytes, rowBytes);
  }

  /*
   * Output the image to a file
   */
  CGColorSpaceRef colorSpace = 
    CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  CGBitmapInfo bitmapInfo = kCGImageAlphaNoneSkipFirst | 
    kCGBitmapByteOrder32Little;  // XRGB Little Endian
  int bitsPerComponent = 8;
  CGContextRef contextRef = CGBitmapContextCreate(flippedBuffer,
                                                  ctx->width, ctx->height, bitsPerComponent, rowBytes, 
                                                  colorSpace, bitmapInfo);
  if (!contextRef) {
    std::cerr << "Unable to create CGContextRef.";
    return false;
  }

  CGImageRef imageRef = CGBitmapContextCreateImage(contextRef);
  if (!imageRef) {
    std::cerr <<  "Unable to create CGImageRef.";
    return false;
  }
  Boolean isDirectory = false;
  CFStringRef fname = CFStringCreateWithCString(kCFAllocatorDefault, filename, kCFStringEncodingUTF8);
  CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                   fname, kCFURLPOSIXPathStyle, isDirectory);
  if (!fileURL) {
    std::cerr << "Unable to create file URL ref.";
    return false;
  }
  CFIndex                 fileImageIndex = 1;
  CFMutableDictionaryRef  fileDict       = NULL;
  CFStringRef             fileUTType     = kUTTypeJPEG;
  // Create an image destination opaque reference for authoring an image file
  CGImageDestinationRef imageDest = CGImageDestinationCreateWithURL(fileURL, 
                                                                    fileUTType, 
                                                                    fileImageIndex, 
                                                                    fileDict);
  if (!imageDest) {
    std::cerr <<  "Unable to create CGImageDestinationRef.";
    return false;
  }
  CFIndex capacity = 1;
  CFMutableDictionaryRef imageProps = 
    CFDictionaryCreateMutable(kCFAllocatorDefault, 
                              capacity,
                              &kCFTypeDictionaryKeyCallBacks,
                              &kCFTypeDictionaryValueCallBacks);
  CGImageDestinationAddImage(imageDest, imageRef, imageProps);
  CGImageDestinationFinalize(imageDest);

  free(flippedBuffer);
  free(bufferData);
  CFRelease(imageDest);
  CFRelease(fileURL);
  CFRelease(fname);
  CFRelease(imageProps);
  CGColorSpaceRelease( colorSpace );
  CGImageRelease(imageRef);
  return true;
}
