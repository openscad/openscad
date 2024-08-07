#pragma once

#include <string>

enum RenderBackend3D {
  UnknownBackend,
  CGALBackend,
  ManifoldBackend,
};

#define DEFAULT_RENDERING_BACKEND_3D RenderBackend3D::CGALBackend

std::string renderBackend3DToString(RenderBackend3D backend);
RenderBackend3D renderBackend3DFromString(const std::string &backend);

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
