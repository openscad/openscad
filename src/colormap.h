#ifndef __colormap_h__
#define __colormap_h__

#include <map>
#include <string>
#include "linalg.h"
#include <boost/unordered/unordered_map.hpp>
#include <boost/assign/list_of.hpp>
#include "printutils.h"
using namespace boost::assign; // bring 'operator+=()' into scope

namespace OSColors {

extern boost::unordered_map<std::string, Color4f> webmap;

namespace RenderColors {
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
 }; // enum
}; // namespace OSColors::RenderColors

typedef std::map<RenderColors::RenderColor,Color4f> colorscheme;

extern colorscheme cornfield;
extern colorscheme metallic;
extern colorscheme sunset;
extern colorscheme sea;
extern colorscheme severny;
extern colorscheme monotone;
extern std::map< std::string, colorscheme > schemes;
extern colorscheme defaultColorScheme;

Color4f getValue( const colorscheme &cs, const RenderColors::RenderColor rc );

}; // namespace OSColors

#endif // __colormap_h__

