#pragma once

#include <optional>
#include <string>

enum class RenderBackend3D {
  UnknownBackend,
  CGALBackend,
  ManifoldBackend,
};

inline constexpr RenderBackend3D DEFAULT_RENDERING_BACKEND_3D = RenderBackend3D::ManifoldBackend;

std::string renderBackend3DToString(RenderBackend3D backend);
std::optional<RenderBackend3D> renderBackend3DFromString(std::string backend);

class RenderSettings
{
public:
  static RenderSettings *inst(bool erase = false);

  RenderBackend3D backend3D;
  unsigned int openCSGTermLimit;
  double far_gl_clip_limit;
  std::string colorscheme;

private:
  RenderSettings();
};
