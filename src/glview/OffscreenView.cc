#include "OffscreenView.h"
#include "system-gl.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>

#include "imageutils.h"
#include "printutils.h"
#include "OffscreenContextFactory.h"
#include "glew-utils.h"

namespace {

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
  std::vector<uint8_t> flippedBuffer(samplesPerPixel * ctx->height() * ctx->width());
  flip_image(&pixels[0], flippedBuffer.data(), samplesPerPixel, ctx->width(), ctx->height());

  return write_png(output, flippedBuffer.data(), ctx->width(), ctx->height());
}

}  // namespace

OffscreenView::OffscreenView(uint32_t width, uint32_t height)
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
