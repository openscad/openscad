#include "rendersettings.h"
#include "printutils.h"

RenderSettings *RenderSettings::inst(bool erase)
{
  static auto instance = new RenderSettings;
  if (erase) {
    delete instance;
    instance = nullptr;
  }
  return instance;
}

RenderSettings::RenderSettings()
{
  openCSGTermLimit = 100000;
  far_gl_clip_limit = 100000.0;
  img_width = 512;
  img_height = 512;
  colorscheme = "Cornfield";
}
