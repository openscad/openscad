#include "OffscreenView.h"
#include "system-gl.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <sstream>

#include "imageutils.h"
#include "printutils.h"
#include "OffscreenContextFactory.h"
#include "glew-utils.h"

OffscreenView::OffscreenView(unsigned int width, unsigned int height)
{
  OffscreenContextFactory::ContextAttributes attrib = {
    .width = width,
    .height = height,
    .majorGLVersion = 2,
    .minorGLVersion = 0,
  };
  this->ctx = OffscreenContextFactory::create(OffscreenContextFactory::defaultProvider(), attrib);
  if (!this->ctx) throw OffscreenViewException("Unable to obtain GL Context");

#ifndef NULLGL
  if (!initializeGlew()) throw OffscreenViewException("Unable to initialize Glew");
#ifdef USE_GLAD
  // FIXME: We could ask for gladLoaderLoadGLES2() here instead
  const auto version = gladLoaderLoadGL();
  if (version == 0) {
    // FIXME: Can we figure out why?
    throw OffscreenViewException("Unable to initialize GLAD");
  }
  // FIXME: Only if verbose
  LOG("GLAD: Loaded OpenGL %1$d.%2$d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#endif // USE_GLAD
#endif // NULLGL

  this->fbo = fbo_new();
  if (!fbo_init(this->fbo, width, height)) OffscreenViewException("Unable to create FBO");
  GLView::initializeGL();
  GLView::resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
  fbo_unbind(this->fbo);
  fbo_delete(this->fbo);
}

#ifdef ENABLE_OPENCSG
void OffscreenView::display_opencsg_warning()
{
  LOG("OpenSCAD recommended OpenGL version is 2.0.");
}
#endif

/*!
   Capture framebuffer from OpenGL and write it to the given ostream.
   Called by save_framebuffer() from platform-specific code.
 */
bool save_framebuffer(const OpenGLContext *ctx, std::ostream& output)
{
  if (!ctx) return false;

  const auto pixels = ctx->getFramebuffer();

  const size_t samplesPerPixel = 4; // R, G, B and A
  // Flip it vertically - images read from OpenGL buffers are upside-down
  const size_t rowBytes = samplesPerPixel * ctx->width();
  auto *flippedBuffer = (unsigned char *)malloc(rowBytes * ctx->height());
  if (!flippedBuffer) {
    std::cerr << "Unable to allocate flipped buffer for corrected image.";
    return true;
  }
  flip_image(&pixels[0], flippedBuffer, samplesPerPixel, ctx->width(), ctx->height());

  bool writeok = write_png(output, flippedBuffer, ctx->width(), ctx->height());

  free(flippedBuffer);

  return writeok;
}

bool OffscreenView::save(const char *filename) const
{
  std::ofstream fstream(filename, std::ios::out | std::ios::binary);
  if (!fstream.is_open()) {
    std::cerr << "Can't open file " << filename << " for writing";
    return false;
  } else {
    save_framebuffer(this->ctx.get(), fstream);
    fstream.close();
  }
  return true;
}

bool OffscreenView::save(std::ostream& output) const
{
  return save_framebuffer(this->ctx.get(), output);
}

std::string OffscreenView::getRendererInfo() const
{
  return STR(glew_dump(), this->ctx->getInfo());
}
