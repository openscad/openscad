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

// Shared by the colour and depth exporters: same scene, same camera, only the
// readback differs.
std::unique_ptr<OffscreenView> prepare_geometry_view(const std::shared_ptr<const Geometry>& root_geom,
                                                     const ViewOptions& options, Camera& camera)
{
  assert(root_geom != nullptr);
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
  glview->setShowDepth(options["depth"]);
  glview->paintGL();
  return glview;
}

}  // namespace

bool export_png(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                Camera& camera, std::ostream& output)
{
  PRINTD("export_png geom");
  const auto glview = prepare_geometry_view(root_geom, options, camera);
  if (!glview) return false;
  glview->save(output);
  return true;
}

bool export_depthmap(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                     Camera& camera, DepthProfile profile, std::ostream& output)
{
  PRINTD("export_depthmap geom");
  const auto glview = prepare_geometry_view(root_geom, options, camera);
  if (!glview) return false;
  return glview->saveDepth(output, profile);
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
  glview->setShowDepth(options["depth"]);
  glview->paintGL();
  return glview;
}

bool export_png(const OffscreenView& glview, std::ostream& output)
{
  PRINTD("export_png_preview_common");
  glview.save(output);
  return true;
}

bool export_depthmap(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                     Camera& camera, const DepthmapOptions& depthOptions, std::ostream& output)
{
  PRINTD("export_depthmap geom options");
  const auto glview = prepare_geometry_view(root_geom, options, camera);
  if (!glview) return false;
  return glview->saveDepth(output, depthOptions);
}

bool export_depthmap(const OffscreenView& glview, const DepthmapOptions& depthOptions,
                     std::ostream& output)
{
  PRINTD("export_depthmap_preview_common options");
  return glview.saveDepth(output, depthOptions);
}

bool export_depthmap(const OffscreenView& glview, DepthProfile profile, std::ostream& output)
{
  PRINTD("export_depthmap_preview_common");
  return glview.saveDepth(output, profile);
}

bool export_pfm(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                Camera& camera, std::ostream& output)
{
  PRINTD("export_pfm geom");
  const auto glview = prepare_geometry_view(root_geom, options, camera);
  if (!glview || !glview->ctx) return false;
  const bool perspective = glview->cam.projection == Camera::ProjectionType::PERSPECTIVE;
  const auto mm =
    linearize_depth(glview->ctx->getDepthbuffer(), glview->clipNear, glview->clipFar, perspective);
  return export_pfm(output, mm, glview->ctx->width(), glview->ctx->height());
}

bool export_pfm(const OffscreenView& glview, std::ostream& output)
{
  PRINTD("export_pfm preview");
  if (!glview.ctx) return false;
  const bool perspective = glview.cam.projection == Camera::ProjectionType::PERSPECTIVE;
  const auto mm =
    linearize_depth(glview.ctx->getDepthbuffer(), glview.clipNear, glview.clipFar, perspective);
  return export_pfm(output, mm, glview.ctx->width(), glview.ctx->height());
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
bool export_depthmap(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options,
                     Camera& camera, DepthProfile profile, std::ostream& output)
{
  return false;
}
bool export_depthmap(const OffscreenView& glview, DepthProfile profile, std::ostream& output)
{
  return false;
}

#endif  // NULLGL
