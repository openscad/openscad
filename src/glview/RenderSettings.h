#pragma once

#include <string>

class RenderSettings
{
public:
  static RenderSettings *inst(bool erase = false);

  unsigned int openCSGTermLimit;
  unsigned int img_width;
  unsigned int img_height;
  double far_gl_clip_limit;
  std::string colorscheme;
private:
  RenderSettings();
};
