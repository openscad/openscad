#include "colormap.h"
#include <boost/assign/list_of.hpp>
#include "printutils.h"
using namespace boost::assign; // bring map_list_of() into scope

boost::unordered_map<std::string, Color4f> webcolors;
OSColors::colorscheme cornfield;
OSColors::colorscheme metallic;
OSColors::colorscheme sunset;
OSColors::colorscheme starnight;
OSColors::colorscheme monotone;
boost::unordered_map<std::string, OSColors::colorscheme> colorschemes;

void OSColors::init() {
webcolors = map_list_of
    ("aliceblue", Color4f(240, 248, 255))
    ("antiquewhite", Color4f(250, 235, 215))
    ("aqua", Color4f(0, 255, 255))
    ("aquamarine", Color4f(127, 255, 212))
    ("azure", Color4f(240, 255, 255))
    ("beige", Color4f(245, 245, 220))
    ("bisque", Color4f(255, 228, 196))
    ("black", Color4f(0, 0, 0))
    ("blanchedalmond", Color4f(255, 235, 205))
    ("blue", Color4f(0, 0, 255))
    ("blueviolet", Color4f(138, 43, 226))
    ("brown", Color4f(165, 42, 42))
    ("burlywood", Color4f(222, 184, 135))
    ("cadetblue", Color4f(95, 158, 160))
    ("chartreuse", Color4f(127, 255, 0))
    ("chocolate", Color4f(210, 105, 30))
    ("coral", Color4f(255, 127, 80))
    ("cornflowerblue", Color4f(100, 149, 237))
    ("cornsilk", Color4f(255, 248, 220))
    ("crimson", Color4f(220, 20, 60))
    ("cyan", Color4f(0, 255, 255))
    ("darkblue", Color4f(0, 0, 139))
    ("darkcyan", Color4f(0, 139, 139))
    ("darkgoldenrod", Color4f(184, 134, 11))
    ("darkgray", Color4f(169, 169, 169))
    ("darkgreen", Color4f(0, 100, 0))
    ("darkgrey", Color4f(169, 169, 169))
    ("darkkhaki", Color4f(189, 183, 107))
    ("darkmagenta", Color4f(139, 0, 139))
    ("darkolivegreen", Color4f(85, 107, 47))
    ("darkorange", Color4f(255, 140, 0))
    ("darkorchid", Color4f(153, 50, 204))
    ("darkred", Color4f(139, 0, 0))
    ("darksalmon", Color4f(233, 150, 122))
    ("darkseagreen", Color4f(143, 188, 143))
    ("darkslateblue", Color4f(72, 61, 139))
    ("darkslategray", Color4f(47, 79, 79))
    ("darkslategrey", Color4f(47, 79, 79))
    ("darkturquoise", Color4f(0, 206, 209))
    ("darkviolet", Color4f(148, 0, 211))
    ("deeppink", Color4f(255, 20, 147))
    ("deepskyblue", Color4f(0, 191, 255))
    ("dimgray", Color4f(105, 105, 105))
    ("dimgrey", Color4f(105, 105, 105))
    ("dodgerblue", Color4f(30, 144, 255))
    ("firebrick", Color4f(178, 34, 34))
    ("floralwhite", Color4f(255, 250, 240))
    ("forestgreen", Color4f(34, 139, 34))
    ("fuchsia", Color4f(255, 0, 255))
    ("gainsboro", Color4f(220, 220, 220))
    ("ghostwhite", Color4f(248, 248, 255))
    ("gold", Color4f(255, 215, 0))
    ("goldenrod", Color4f(218, 165, 32))
    ("gray", Color4f(128, 128, 128))
    ("green", Color4f(0, 128, 0))
    ("greenyellow", Color4f(173, 255, 47))
    ("grey", Color4f(128, 128, 128))
    ("honeydew", Color4f(240, 255, 240))
    ("hotpink", Color4f(255, 105, 180))
    ("indianred", Color4f(205, 92, 92))
    ("indigo", Color4f(75, 0, 130))
    ("ivory", Color4f(255, 255, 240))
    ("khaki", Color4f(240, 230, 140))
    ("lavender", Color4f(230, 230, 250))
    ("lavenderblush", Color4f(255, 240, 245))
    ("lawngreen", Color4f(124, 252, 0))
    ("lemonchiffon", Color4f(255, 250, 205))
    ("lightblue", Color4f(173, 216, 230))
    ("lightcoral", Color4f(240, 128, 128))
    ("lightcyan", Color4f(224, 255, 255))
    ("lightgoldenrodyellow", Color4f(250, 250, 210))
    ("lightgray", Color4f(211, 211, 211))
    ("lightgreen", Color4f(144, 238, 144))
    ("lightgrey", Color4f(211, 211, 211))
    ("lightpink", Color4f(255, 182, 193))
    ("lightsalmon", Color4f(255, 160, 122))
    ("lightseagreen", Color4f(32, 178, 170))
    ("lightskyblue", Color4f(135, 206, 250))
    ("lightslategray", Color4f(119, 136, 153))
    ("lightslategrey", Color4f(119, 136, 153))
    ("lightsteelblue", Color4f(176, 196, 222))
    ("lightyellow", Color4f(255, 255, 224))
    ("lime", Color4f(0, 255, 0))
    ("limegreen", Color4f(50, 205, 50))
    ("linen", Color4f(250, 240, 230))
    ("magenta", Color4f(255, 0, 255))
    ("maroon", Color4f(128, 0, 0))
    ("mediumaquamarine", Color4f(102, 205, 170))
    ("mediumblue", Color4f(0, 0, 205))
    ("mediumorchid", Color4f(186, 85, 211))
    ("mediumpurple", Color4f(147, 112, 219))
    ("mediumseagreen", Color4f(60, 179, 113))
    ("mediumslateblue", Color4f(123, 104, 238))
    ("mediumspringgreen", Color4f(0, 250, 154))
    ("mediumturquoise", Color4f(72, 209, 204))
    ("mediumvioletred", Color4f(199, 21, 133))
    ("midnightblue", Color4f(25, 25, 112))
    ("mintcream", Color4f(245, 255, 250))
    ("mistyrose", Color4f(255, 228, 225))
    ("moccasin", Color4f(255, 228, 181))
    ("navajowhite", Color4f(255, 222, 173))
    ("navy", Color4f(0, 0, 128))
    ("oldlace", Color4f(253, 245, 230))
    ("olive", Color4f(128, 128, 0))
    ("olivedrab", Color4f(107, 142, 35))
    ("orange", Color4f(255, 165, 0))
    ("orangered", Color4f(255, 69, 0))
    ("orchid", Color4f(218, 112, 214))
    ("palegoldenrod", Color4f(238, 232, 170))
    ("palegreen", Color4f(152, 251, 152))
    ("paleturquoise", Color4f(175, 238, 238))
    ("palevioletred", Color4f(219, 112, 147))
    ("papayawhip", Color4f(255, 239, 213))
    ("peachpuff", Color4f(255, 218, 185))
    ("peru", Color4f(205, 133, 63))
    ("pink", Color4f(255, 192, 203))
    ("plum", Color4f(221, 160, 221))
    ("powderblue", Color4f(176, 224, 230))
    ("purple", Color4f(128, 0, 128))
    ("red", Color4f(255, 0, 0))
    ("rosybrown", Color4f(188, 143, 143))
    ("royalblue", Color4f(65, 105, 225))
    ("saddlebrown", Color4f(139, 69, 19))
    ("salmon", Color4f(250, 128, 114))
    ("sandybrown", Color4f(244, 164, 96))
    ("seagreen", Color4f(46, 139, 87))
    ("seashell", Color4f(255, 245, 238))
    ("sienna", Color4f(160, 82, 45))
    ("silver", Color4f(192, 192, 192))
    ("skyblue", Color4f(135, 206, 235))
    ("slateblue", Color4f(106, 90, 205))
    ("slategray", Color4f(112, 128, 144))
    ("slategrey", Color4f(112, 128, 144))
    ("snow", Color4f(255, 250, 250))
    ("springgreen", Color4f(0, 255, 127))
    ("steelblue", Color4f(70, 130, 180))
    ("tan", Color4f(210, 180, 140))
    ("teal", Color4f(0, 128, 128))
    ("thistle", Color4f(216, 191, 216))
    ("tomato", Color4f(255, 99, 71))
    ("transparent", Color4f(0, 0, 0, 0))
    ("turquoise", Color4f(64, 224, 208))
    ("violet", Color4f(238, 130, 238))
    ("wheat", Color4f(245, 222, 179))
    ("white", Color4f(255, 255, 255))
    ("whitesmoke", Color4f(245, 245, 245))
    ("yellow", Color4f(255, 255, 0))
    ("yellowgreen", Color4f(154, 205, 50));

cornfield = map_list_of
  (BACKGROUND_COLOR,         Color4f(0xfa, 0xfa, 0xfa))
  (OPENCSG_FACE_FRONT_COLOR, Color4f(0xfd, 0xd6, 0x3a))
  (OPENCSG_FACE_BACK_COLOR,  Color4f(0x16, 0xa0, 0x85))
  (CGAL_FACE_FRONT_COLOR,    Color4f(0xfd, 0xd6, 0x3a))
  (CGAL_FACE_BACK_COLOR,     Color4f(0x16, 0xa0, 0x85))
  (CGAL_FACE_2D_COLOR,       Color4f(0x00, 0xff, 0x00))
  (CGAL_EDGE_FRONT_COLOR,    Color4f(0xff, 0xec, 0x5e))
  (CGAL_EDGE_BACK_COLOR,     Color4f(0xab, 0xd8, 0x56))
  (CGAL_EDGE_2D_COLOR,       Color4f(0xff, 0x00, 0x00))
  (CROSSHAIR_COLOR,          Color4f(0x80, 0x00, 0x00));

metallic = map_list_of
  (BACKGROUND_COLOR,         Color4f(0x39, 0x39, 0x39))
  (OPENCSG_FACE_FRONT_COLOR, Color4f(0xcf, 0xcf, 0xcf))
  (OPENCSG_FACE_BACK_COLOR,  Color4f(0x5e, 0x5e, 0x5e))
  (CGAL_FACE_FRONT_COLOR,    Color4f(0xcf, 0xcf, 0xcf))
  (CGAL_FACE_BACK_COLOR,     Color4f(0x5e, 0x5e, 0x5e))
  (CGAL_FACE_2D_COLOR,       Color4f(0x00, 0xbf, 0x99))
  (CGAL_EDGE_FRONT_COLOR,    Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_BACK_COLOR,     Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_2D_COLOR,       Color4f(0xff, 0x00, 0x00))
  (CROSSHAIR_COLOR,          Color4f(0x11, 0x11, 0x11));

sunset = map_list_of
  (BACKGROUND_COLOR,         Color4f(0xfa, 0xfa, 0xfa))
  (OPENCSG_FACE_FRONT_COLOR, Color4f(0xb4, 0x2f, 0x2f))
  (OPENCSG_FACE_BACK_COLOR,  Color4f(0x56, 0x17, 0x17))
  (CGAL_FACE_FRONT_COLOR,    Color4f(0xb4, 0x2f, 0x2f))
  (CGAL_FACE_BACK_COLOR,     Color4f(0x88, 0x22, 0x33))
  (CGAL_FACE_2D_COLOR,       Color4f(0x00, 0xbf, 0x99))
  (CGAL_EDGE_FRONT_COLOR,    Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_BACK_COLOR,     Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_2D_COLOR,       Color4f(0xff, 0x00, 0x00))
  (CROSSHAIR_COLOR,          Color4f(0x11, 0x11, 0x11));

starnight = map_list_of
  (BACKGROUND_COLOR,         Color4f(0x33, 0x33, 0x33))
  (OPENCSG_FACE_FRONT_COLOR, Color4f(0xee, 0xee, 0xee))
  (OPENCSG_FACE_BACK_COLOR,  Color4f(0x0b, 0xab, 0xc8))
  (CGAL_FACE_FRONT_COLOR,    Color4f(0xee, 0xee, 0xee))
  (CGAL_FACE_BACK_COLOR,     Color4f(0x0b, 0xab, 0xc8))
  (CGAL_FACE_2D_COLOR,       webcolors["mediumpurple"])
  (CGAL_EDGE_FRONT_COLOR,    Color4f(0x00, 0x00, 0xff))
  (CGAL_EDGE_BACK_COLOR,     Color4f(0x00, 0x00, 0xff))
  (CGAL_EDGE_2D_COLOR,       webcolors["magenta"])
  (CROSSHAIR_COLOR,          Color4f(0xf0, 0xf0, 0xf0));

// Monotone - no difference between 'back face' and 'front face'
monotone = map_list_of
  (BACKGROUND_COLOR,         Color4f(0xff, 0xff, 0xe5))
  (OPENCSG_FACE_FRONT_COLOR, Color4f(0xf9, 0xd7, 0x2c))
  (OPENCSG_FACE_BACK_COLOR,  Color4f(0xf9, 0xd7, 0x2c))
  (CGAL_FACE_FRONT_COLOR,    Color4f(0xf9, 0xd7, 0x2c))
  (CGAL_FACE_BACK_COLOR,     Color4f(0xf9, 0xd7, 0x2c))
  (CGAL_FACE_2D_COLOR,       Color4f(0x00, 0xbf, 0x99))
  (CGAL_EDGE_FRONT_COLOR,    Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_BACK_COLOR,     Color4f(0xff, 0x00, 0x00))
  (CGAL_EDGE_2D_COLOR,       Color4f(0xff, 0x00, 0x00))
  (CROSSHAIR_COLOR,          Color4f(0x80, 0x00, 0x00));

colorschemes = map_list_of
  ("Cornfield", cornfield)
  ("Metallic", metallic)
  ("Sunset", sunset)
  ("Starnight", starnight)
  ("Monotone", monotone); // Hidden, not in GUI
}

const OSColors::colorscheme &OSColors::defaultColorScheme()
{
	return cornfield;
}

const boost::unordered_map<std::string, Color4f> &OSColors::webColors()
{
	return webcolors;
}

const OSColors::colorscheme *OSColors::colorScheme(const std::string &name)
{
	if (colorschemes.find(name) != colorschemes.end()) return &colorschemes.at(name);
	return NULL;
}

std::list<std::string> OSColors::colorSchemes()
{
	std::list<std::string> names;
	for (boost::unordered_map<std::string, OSColors::colorscheme>::const_iterator iter=colorschemes.begin(); iter!=colorschemes.end(); iter++) {
		names.push_back(iter->first);
	}
	return names;
}

Color4f OSColors::getValue(const OSColors::colorscheme &cs, const OSColors::RenderColor rc)
{
	if (cs.count(rc)) return cs.at(rc);
	if (OSColors::defaultColorScheme().count(rc)) return defaultColorScheme().at(rc);
	return Color4f(0, 0, 0, 127);
}

/*
void printcolorscheme(const OSColors::colorscheme &cs)
{
	for (OSColors::colorscheme::const_iterator j=cs.begin();j!=cs.end();j++)
		PRINTB("%i %s",j->first % j->second.transpose());
}

void printcolorschemes()
{
	for (boost::unordered_map<std::string, OSColors::colorscheme>::const_iterator i=colorschemes.begin();i!=colorschemes.end();i++) {
		PRINTB("-- %s --", i->first);
		for (OSColors::colorscheme::const_iterator j=i->second.begin();j!=i->second.end();j++)
			PRINTB("%i %s",j->first % j->second.transpose());
	}
}
*/
