#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <cstdio>
#include <memory>
#include "RenderSettings.h"

#ifndef NULLGL

#include "CGALRenderer.h"
#ifdef ENABLE_LEGACY_RENDERERS
#include "LegacyCGALRenderer.h"
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
  if (Feature::ExperimentalVxORenderers.is_enabled()) {
    cgalRenderer = std::make_shared<CGALRenderer>(root_geom);
  }
#ifdef ENABLE_LEGACY_RENDERERS
  else {
    cgalRenderer = std::make_shared<LegacyCGALRenderer>(root_geom);
  }
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
#include "OpenCSGRenderer.h"
#ifdef ENABLE_LEGACY_RENDERERS
#include "LegacyOpenCSGRenderer.h"
#endif
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"
#ifdef ENABLE_LEGACY_RENDERERS
#include "LegacyThrownTogetherRenderer.h"
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
    if (Feature::ExperimentalVxORenderers.is_enabled()) {
      renderer = std::make_shared<OpenCSGRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
    }
#ifdef ENABLE_LEGACY_RENDERERS
    else {
      renderer = std::make_shared<LegacyOpenCSGRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
    }
#endif
#else
    fprintf(stderr, "This openscad was built without OpenCSG support\n");
    return 0;
#endif
  } else {
    if (Feature::ExperimentalVxORenderers.is_enabled()) {
      renderer = std::make_shared<ThrownTogetherRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
    }
#ifdef ENABLE_LEGACY_RENDERERS
    else {
      renderer = std::make_shared<LegacyThrownTogetherRenderer>(csgInfo.root_products, csgInfo.highlights_products, csgInfo.background_products);
    }
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
