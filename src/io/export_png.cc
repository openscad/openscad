#include "io/export.h"
#include "utils/printutils.h"
#include "glview/OffscreenView.h"
#include "glview/CsgInfo.h"
#include <ostream>
#include <cstdio>
#include <memory>
#include "glview/RenderSettings.h"

#ifndef NULLGL

#include "glview/cgal/CGALRenderer.h"
#ifdef USE_LEGACY_RENDERERS
#include "glview/cgal/LegacyCGALRenderer.h"
#endif


static void setupCamera(Camera& cam, const BoundingBox& bbox)
{
  if (cam.viewall) cam.viewAll(bbox);
}

bool export_png(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options, Camera& camera, std::ostream& output)
{
  PRINTD("export_png geom");
  std::unique_ptr<OffscreenView> glview;
  try {
    glview = std::make_unique<OffscreenView>(camera.pixel_width, camera.pixel_height);
  } catch (const OffscreenViewException &ex) {
    fprintf(stderr, "Can't create OffscreenView: %s.\n", ex.what());
    return false;
  }
  std::shared_ptr<Renderer> cgalRenderer;
#ifdef USE_LEGACY_RENDERERS
  cgalRenderer = std::make_shared<LegacyCGALRenderer>(root_geom);
#else
  cgalRenderer = std::make_shared<CGALRenderer>(root_geom);
#endif
  BoundingBox bbox = cgalRenderer->getBoundingBox();
  setupCamera(camera, bbox);

  glview->setCamera(camera);
  glview->setRenderer(cgalRenderer);
  glview->setColorScheme(RenderSettings::inst()->colorscheme);
  glview->setShowFaces(!options["wireframe"]);
  glview->setShowCrosshairs(options["crosshairs"]);
  glview->setShowAxes(options["axes"]);
  glview->setShowScaleProportional(options["scales"]);
  glview->setShowEdges(options["edges"]);
  glview->paintGL();
  glview->save(output);
  return true;
}

#ifdef ENABLE_OPENCSG
#include "glview/preview/OpenCSGRenderer.h"
#ifdef USE_LEGACY_RENDERERS
#include "glview/preview/LegacyOpenCSGRenderer.h"
#endif
#include <opencsg.h>
#endif
#include "glview/preview/ThrownTogetherRenderer.h"
#ifdef USE_LEGACY_RENDERERS
#include "glview/preview/LegacyThrownTogetherRenderer.h"
#endif

std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera)
{
  PRINTD("prepare_preview_common");
  CsgInfo csgInfo = CsgInfo();
  csgInfo.compile_products(tree);

  std::unique_ptr<OffscreenView> glview;
  try {
    glview = std::make_unique<OffscreenView>(camera.pixel_width, camera.pixel_height);
  } catch (const OffscreenViewException &ex) {
    LOG("Can't create OffscreenView: %1$s.", ex.what());
    return nullptr;
  }

  std::shared_ptr<Renderer> renderer;
  if (options.previewer == Previewer::OPENCSG) {
#ifdef ENABLE_OPENCSG
#ifdef USE_LEGACY_RENDERERS
    PRINTD("Initializing LegacyOpenCSGRenderer");
    renderer = std::make_shared<LegacyOpenCSGRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
#else
    PRINTD("Initializing OpenCSGRenderer");
    renderer = std::make_shared<OpenCSGRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
#endif
#else
    fprintf(stderr, "This openscad was built without OpenCSG support\n");
    return 0;
#endif
  } else {
#ifdef USE_LEGACY_RENDERERS
    PRINTD("Initializing LegacyThrownTogetherRenderer");
    renderer = std::make_shared<LegacyThrownTogetherRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
#else
    PRINTD("Initializing ThrownTogetherRenderer");
    renderer = std::make_shared<ThrownTogetherRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
#endif
  }

  glview->setRenderer(renderer);


#ifdef ENABLE_OPENCSG
  BoundingBox bbox = glview->getRenderer()->getBoundingBox();
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

#else // NULLGL

bool export_png(const std::shared_ptr<const Geometry>& root_geom, const ViewOptions& options, Camera& camera, std::ostream& output) { return false; }
std::unique_ptr<OffscreenView> prepare_preview(Tree& tree, const ViewOptions& options, Camera& camera) { return nullptr; }
bool export_png(const OffscreenView& glview, std::ostream& output) { return false; }

#endif // NULLGL
