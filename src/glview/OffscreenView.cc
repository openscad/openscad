#include "glview/OffscreenView.h"
#include "glview/system-gl.h"
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>

#include "io/imageutils.h"
#include "utils/printutils.h"
#include "glview/OffscreenContextFactory.h"
#include "glview/fbo.h"
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
#include "glview/glew-utils.h"
#endif

namespace {

/*!
 Capture framebuffer from OpenGL and write it to the given ostream.
 Called by save_framebuffer() from platform-specific code.
*/
bool save_framebuffer(const OpenGLContext *ctx, std::ostream& output)
{
  if (!ctx) return false;

  const auto pixels = ctx->getFramebuffer();

  const size_t samplesPerPixel = 4;  // R, G, B and A
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
  auto provider = OffscreenContextFactory::defaultProvider();
  // We cannot initialize GLX GLEW with an EGL context:
  // https://github.com/nigels-com/glew/issues/273
  // ..so if we're using GLEW, default to creating a GLX context.
  // FIXME: It's possible that GLEW was built using EGL, in which case this
  // logic isn't correct, but we don't have a good way of determining how GLEW was built.
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  provider = !strcmp(provider, "egl") ? "glx" : provider;
#endif
  this->ctx = OffscreenContextFactory::create(provider, attrib);
  if (!this->ctx) {
    // If the provider defaulted to EGL, fall back to GLX if EGL failed
    if (!strcmp(provider, "egl")) {
      this->ctx = OffscreenContextFactory::create("glx", attrib);
    }
    if (!this->ctx) {
      throw OffscreenViewException("Unable to obtain GL Context");
    }
  }
  if (!this->ctx->makeCurrent()) throw OffscreenViewException("Unable to make GL context current");

#ifndef NULLGL
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  if (!initializeGlew()) {
    throw OffscreenViewException("Unable to initialize Glew");
  }
#endif  // USE_GLEW
#ifdef USE_GLAD
  // We could ask for gladLoadGLES2UserPtr() here if we want to use GLES2+
  const auto version = gladLoaderLoadGL();
  if (version == 0) {
    throw OffscreenViewException("Unable to initialize GLAD");
  }
  PRINTDB("GLAD: Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(version) % GLAD_VERSION_MINOR(version));
#endif  // USE_GLAD

#endif  // NULLGL

  PRINTD(gl_dump());

  this->fbo = createFBO(width, height);
  if (!fbo) {
    throw OffscreenViewException("Unable to create FBO");
  }
  GLView::initializeGL();
  GLView::resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
  fbo.reset();
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

bool OffscreenView::saveDepth(std::ostream& output, DepthProfile profile) const
{
  if (!this->ctx) return false;

  const bool perspective = this->cam.projection == Camera::ProjectionType::PERSPECTIVE;
  const auto mm =
    linearize_depth(this->ctx->getDepthbuffer(), this->clipNear, this->clipFar, perspective);
  const auto image = encode_depthmap(mm, this->ctx->width(), this->ctx->height(), profile);

  // Same as the colour path: buffers read from OpenGL are upside-down.
  std::vector<uint8_t> flipped(image.pixels.size());
  flip_image(image.pixels.data(), flipped.data(), image.bytesPerPixel, this->ctx->width(),
             this->ctx->height());

  // The scale cannot travel inside the image, so report it - without it a
  // metric depthmap is just numbers.
  if (profile == DepthProfile::metric) {
    LOG("Depthmap: %1$.3f - %2$.3f mm from the camera, %3$g units per mm.", image.minDepth,
        image.maxDepth, DEPTHMAP_METRIC_SCALE);
    return write_png_gray16(output, flipped.data(), this->ctx->width(), this->ctx->height());
  }
  LOG("Depthmap: %1$.3f - %2$.3f mm from the camera, normalized across that range.", image.minDepth,
      image.maxDepth);
  return write_png(output, flipped.data(), this->ctx->width(), this->ctx->height());
}

bool OffscreenView::save(std::ostream& output) const
{
  return save_framebuffer(this->ctx.get(), output);
}

std::string OffscreenView::getRendererInfo() const
{
  std::ostringstream result;
  result << this->ctx->getInfo() << "\n" << gl_dump();
  return result.str();
}
