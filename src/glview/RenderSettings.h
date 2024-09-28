#pragma once

#include <string>

enum class RenderBackend3D {
  UnknownBackend,
  CGALBackend,
  ManifoldBackend,
};

inline constexpr RenderBackend3D DEFAULT_RENDERING_BACKEND_3D = RenderBackend3D::CGALBackend; // ManifoldBackend;

std::string renderBackend3DToString(RenderBackend3D backend);
RenderBackend3D renderBackend3DFromString(std::string backend);

class RenderSettings
{
public:
  static RenderSettings *inst(bool erase = false);

  RenderBackend3D backend3D;
  unsigned int openCSGTermLimit;
  unsigned int img_width;
  unsigned int img_height;
  double far_gl_clip_limit;
  std::string colorscheme;
private:
  RenderSettings();
};
