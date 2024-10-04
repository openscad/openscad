#include "glview/RenderSettings.h"
#include <stdexcept>
#include <string>
#include "utils/printutils.h"

std::string renderBackend3DToString(RenderBackend3D backend) {
  switch (backend) {
  case RenderBackend3D::CGALBackend:
    return "CGAL";
  case RenderBackend3D::ManifoldBackend:
    return "Manifold";
  default:
    throw std::runtime_error("Unknown rendering backend");
  }
}

RenderBackend3D renderBackend3DFromString(std::string backend) {
  boost::algorithm::to_lower(backend);
  if (backend == "cgal") {
    return RenderBackend3D::CGALBackend;
  } else if (backend == "manifold") {
    return RenderBackend3D::ManifoldBackend;
  } else {
    if (!backend.empty()) {
      LOG(message_group::Warning,
          "Unknown rendering backend '%1$s'. Using default '%2$s'.",
          backend.c_str(),
          renderBackend3DToString(DEFAULT_RENDERING_BACKEND_3D).c_str());
    }
    return DEFAULT_RENDERING_BACKEND_3D;
  }
}

RenderSettings *RenderSettings::inst(bool erase) {
  static auto instance = new RenderSettings;
  if (erase) {
    delete instance;
    instance = nullptr;
  }
  return instance;
}

RenderSettings::RenderSettings() {
  backend3D = DEFAULT_RENDERING_BACKEND_3D;
  openCSGTermLimit = 100000;
  far_gl_clip_limit = 100000.0;
  img_width = 512;
  img_height = 512;
  colorscheme = "Cornfield";
}
