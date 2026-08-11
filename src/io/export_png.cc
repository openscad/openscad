#include <cassert>
#include <cstdio>
#include <memory>
#include <ostream>

#include "core/Tree.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "glview/Camera.h"
#include "glview/CsgInfo.h"
#include "glview/OffscreenView.h"
#include "glview/RenderSettings.h"
#include "glview/Renderer.h"
#include "io/export.h"
#include "io/VideoEncoder.h"
#include "io/imageutils.h"
#include "utils/printutils.h"

#ifndef NULLGL
#include "glview/PolySetRenderer.h"
#if not defined(USE_POLYSET_FOR_CGAL)
#include "glview/cgal/CGALRenderer.h"
#endif

#ifdef ENABLE_OPENCSG
#include <opencsg.h>

#include "glview/preview/OpenCSGRenderer.h"
#endif  // ENABLE_OPENCSG

#include "glview/preview/ThrownTogetherRenderer.h"

namespace {

void setupCamera(Camera& cam, const BoundingBox& bbox)
{
  if (cam.viewall) cam.viewAll(bbox);
}

}  // namespace

namespace {

//! Renders `root_geom` offscreen and returns the painted view, or null on failure.
std::unique_ptr<OffscreenView> prepare_geom_view(const std::shared_ptr<const Geometry>& root_geom,
                                                 const ViewOptions& options, Camera& camera)
{
  assert(root_geom != nullptr);
  PRINTD("prepare_geom_view");
  std::unique_ptr<OffscreenView> glview;
  try {
    glview = std::make_unique<OffscreenView>(camera.pixel_width, camera.pixel_height);
  } catch (const OffscreenViewException& ex) {
    fprintf(stderr, "Can't create OffscreenView: %s.\n", ex.what());
    return nullptr;
  }
  std::shared_ptr<Renderer> geomRenderer;
#if defined(USE_POLYSET_FOR_CGAL)
  geomRenderer = std::make_shared<PolySetRenderer>(root_geom);
#else
  // Choose PolySetRenderer for PolySet and Polygon2d, and for Manifold since we
  // know that all geometries are convertible to PolySet.
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend ||
      std::dynamic_pointer_cast<const PolySet>(root_geom) ||
      std::dynamic_pointer_cast<const Polygon2d>(root_geom)) {
    geomRenderer = std::make_shared<PolySetRenderer>(root_geom);
  } else {
    geomRenderer = std::make_shared<CGALRenderer>(root_geom);
  }
#endif
  const BoundingBox bbox = geomRenderer->getBoundingBox();
  setupCamera(camera, bbox);

  glview->setCamera(camera);
  glview->setRenderer(geomRenderer);
  glview->setColorScheme(RenderSettings::inst()->colorscheme);
  glview->setShowCrosshairs(options["crosshairs"]);
  glview->setShowAxes(options["axes"]);
  glview->setShowScaleProportional(options["scales"]);
  glview->setShowEdges(options["edges"]);
  glview->paintGL();
  return glview;
}

}  // namespace

bool export_png(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                Camera& camera, std::ostream& output)
{
  const auto glview = prepare_geom_view(root_geom, options, camera);
  if (!glview) return false;
  glview->save(output);
  return true;
}

std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera)
{
  PRINTD("prepare_preview_common");
  CsgInfo csgInfo = CsgInfo();
  csgInfo.compile_products(tree);

  std::unique_ptr<OffscreenView> glview;
  try {
    glview = std::make_unique<OffscreenView>(camera.pixel_width, camera.pixel_height);
  } catch (const OffscreenViewException& ex) {
    LOG("Can't create OffscreenView: %1$s.", ex.what());
    return nullptr;
  }

  std::shared_ptr<Renderer> renderer;
  if (options.previewer == Previewer::OPENCSG) {
#ifdef ENABLE_OPENCSG
    PRINTD("Initializing OpenCSGRenderer");
    renderer = std::make_shared<OpenCSGRenderer>(csgInfo.root_products, csgInfo.highlights_products,
                                                 csgInfo.background_products);
#else
    fprintf(stderr, "This openscad was built without OpenCSG support\n");
    return 0;
#endif
  } else {
    PRINTD("Initializing ThrownTogetherRenderer");
    renderer = std::make_shared<ThrownTogetherRenderer>(
      csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
  }

  glview->setRenderer(renderer);

#ifdef ENABLE_OPENCSG
  const BoundingBox bbox = glview->getRenderer()->getBoundingBox();
  setupCamera(camera, bbox);

  glview->setCamera(camera);
  OpenCSG::setContext(0);
  OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
  glview->setColorScheme(RenderSettings::inst()->colorscheme);
  glview->setShowAxes(options["axes"]);
  glview->setShowScaleProportional(options["scales"]);
  glview->setShowEdges(options["edges"]);
  glview->paintGL();
  return glview;
}

bool export_png(const OffscreenView& glview, std::ostream& output)
{
  PRINTD("export_png_preview_common");
  glview.save(output);
  return true;
}

bool export_video_frame(const OffscreenView& glview, VideoEncoder& encoder)
{
  if (!glview.ctx) return false;
  const auto pixels = glview.ctx->getFramebuffer();
  const size_t width = glview.ctx->width();
  const size_t height = glview.ctx->height();
  const size_t samplesPerPixel = 4;  // R, G, B and A
  if (pixels.size() < samplesPerPixel * width * height) return false;

  // Images read from OpenGL buffers are upside-down.
  std::vector<uint8_t> flipped(samplesPerPixel * width * height);
  flip_image(pixels.data(), flipped.data(), samplesPerPixel, width, height);

  return encoder.addFrame(flipped.data(), samplesPerPixel * width);
}

bool export_video_frame(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                        Camera& camera, VideoEncoder& encoder)
{
  const auto glview = prepare_geom_view(root_geom, options, camera);
  if (!glview) return false;
  return export_video_frame(*glview, encoder);
}

#else  // NULLGL

bool export_png(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                Camera& camera, std::ostream& output)
{
  return false;
}
std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera)
{
  return nullptr;
}
bool export_png(const OffscreenView& glview, std::ostream& output)
{
  return false;
}
bool export_video_frame(const OffscreenView& glview, VideoEncoder& encoder)
{
  return false;
}
bool export_video_frame(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                        Camera& camera, VideoEncoder& encoder)
{
  return false;
}

#endif  // NULLGL
