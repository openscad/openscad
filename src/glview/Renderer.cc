#include "glview/Renderer.h"
#include "geometry/linalg.h"
#include "glview/ShaderUtils.h"
#include "core/Selection.h"
#include "glview/ColorMap.h"
#include "utils/printutils.h"
#include "platform/PlatformUtils.h"
#include "glview/system-gl.h"

#include <cstdio>
#include <sstream>
#include <Eigen/LU>
#include <fstream>
#include <string>
#include <vector>

#ifndef NULLGL


Renderer::Renderer()
{
  PRINTD("Renderer() start");

  // Setup default colors
  // The main colors, MATERIAL and CUTOUT, come from this object's
  // colorscheme. Colorschemes don't currently hold information
  // for Highlight/Background colors
  // but it wouldn't be too hard to make them do so.

  // MATERIAL is set by this object's colorscheme
  // CUTOUT is set by this object's colorscheme
  colormap_[ColorMode::HIGHLIGHT] = {255, 81, 81, 128};
  colormap_[ColorMode::BACKGROUND] = {180, 180, 180, 128};
  // MATERIAL_EDGES is set by this object's colorscheme
  // CUTOUT_EDGES is set by this object's colorscheme
  colormap_[ColorMode::HIGHLIGHT_EDGES] = {255, 171, 86, 128};
  colormap_[ColorMode::BACKGROUND_EDGES] = {150, 150, 150, 128};

  Renderer::setColorScheme(ColorMap::inst()->defaultColorScheme());

  PRINTD("Renderer() end");
}

bool Renderer::getColorSchemeColor(Renderer::ColorMode colormode, Color4f& outcolor) const
{
  if (const auto it = colormap_.find(colormode); it != colormap_.end()) {
    outcolor = it->second;
    return true;
  }
  return false;
}

bool Renderer::getShaderColor(Renderer::ColorMode colormode, const Color4f& object_color,
                              Color4f& outcolor) const
{
  // If an object was colored, use any set components from that color, except in pure highlight mode
  if ((colormode == ColorMode::BACKGROUND || colormode != ColorMode::HIGHLIGHT)) {
    if (object_color.hasRgb()) outcolor.setRgb(object_color.r(), object_color.g(), object_color.b());
    if (object_color.hasAlpha()) outcolor.setAlpha(object_color.a());
    if (outcolor.isValid()) return true;
  }

  // Fill in missing components with the color from the colorscheme
  Color4f basecol;
  if (Renderer::getColorSchemeColor(colormode, basecol)) {
    if (!outcolor.hasRgb()) outcolor.setRgb(basecol.r(), basecol.g(), basecol.b());
    if (!outcolor.hasAlpha()) outcolor.setAlpha(basecol.a());
    return true;
  }

  return false;
}

/* fill colormap_ with matching entries from the colorscheme. note
   this does not change Highlight or Background colors as they are not
   represented in the colorscheme (yet). Also edgecolors are currently the
   same for CGAL & OpenCSG */
void Renderer::setColorScheme(const ColorScheme& cs)
{
  PRINTD("setColorScheme");
  colormap_[ColorMode::MATERIAL] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_FRONT_COLOR);
  colormap_[ColorMode::CUTOUT] = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_BACK_COLOR);
  colormap_[ColorMode::MATERIAL_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_FRONT_COLOR);
  colormap_[ColorMode::CUTOUT_EDGES] = ColorMap::getColor(cs, RenderColor::CGAL_EDGE_BACK_COLOR);
  colormap_[ColorMode::EMPTY_SPACE] = ColorMap::getColor(cs, RenderColor::BACKGROUND_COLOR);
  colorscheme_ = &cs;
}

std::vector<SelectedObject> Renderer::findModelObject(const Vector3d& /*near_pt*/,
                                                      const Vector3d& /*far_pt*/, int /*mouse_x*/,
                                                      int /*mouse_y*/, double /*tolerance*/)
{
  return {};
}
#else  // NULLGL

Renderer::Renderer() : colorscheme_(nullptr)
{
}
bool Renderer::getColorSchemeColor(Renderer::ColorMode colormode, Color4f& outcolor) const
{
  return false;
}
bool Renderer::getShaderColor(Renderer::ColorMode colormode, const Color4f& object_color,
                              Color4f& outcolor) const
{
  return false;
}
void Renderer::setColorScheme(const ColorScheme& cs)
{
}
std::vector<SelectedObject> Renderer::findModelObject(const Vector3d& /*near_pt*/,
                                                      const Vector3d& /*far_pt*/, int /*mouse_x*/,
                                                      int /*mouse_y*/, double /*tolerance*/)
{
  return {};
}

#endif  // NULLGL
