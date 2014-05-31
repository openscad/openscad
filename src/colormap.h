#pragma once

#include <map>
#include <string>
#include "linalg.h"
#include <boost/unordered/unordered_map.hpp>
#include <boost/assign/list_of.hpp>
#include "printutils.h"
using namespace boost::assign; // bring 'operator+=()' into scope

namespace OSColors {

extern boost::unordered_map<std::string, Color4f> webcolors;

enum RenderColor {
  BACKGROUND_COLOR,
  OPENCSG_FACE_FRONT_COLOR,
  OPENCSG_FACE_BACK_COLOR,
  CGAL_FACE_FRONT_COLOR,
  CGAL_FACE_2D_COLOR,
  CGAL_FACE_BACK_COLOR,
  CGAL_EDGE_FRONT_COLOR,
  CGAL_EDGE_BACK_COLOR,
  CGAL_EDGE_2D_COLOR,
  CROSSHAIR_COLOR
};

typedef std::map<RenderColor, Color4f> colorscheme;

extern colorscheme cornfield;
extern colorscheme metallic;
extern colorscheme sunset;
extern colorscheme starnight;
extern colorscheme monotone;
extern boost::unordered_map<std::string, OSColors::colorscheme> colorschemes;
extern colorscheme &defaultColorScheme;

Color4f getValue(const colorscheme &cs, const RenderColor rc);

}; // namespace OSColors

