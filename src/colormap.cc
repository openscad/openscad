#include "colormap.h"
#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>
#include "boosty.h"
#include "printutils.h"
#include "PlatformUtils.h"

using namespace boost::assign; // bring map_list_of() into scope

RenderColorScheme::RenderColorScheme(fs::path path) : path(path)
{
    try {
	boost::property_tree::read_json(boosty::stringy(path).c_str(), pt);
	_name = pt.get<std::string>("name");
	_index = pt.get<int>("index");
	_show_in_gui = pt.get<bool>("show-in-gui");
	
	addColor(BACKGROUND_COLOR, "background");
	addColor(OPENCSG_FACE_FRONT_COLOR, "opencsg-face-front");
	addColor(OPENCSG_FACE_BACK_COLOR, "opencsg-face-back");
	addColor(CGAL_FACE_FRONT_COLOR, "cgal-face-front");
	addColor(CGAL_FACE_2D_COLOR, "cgal-face-2d");
	addColor(CGAL_FACE_BACK_COLOR, "cgal-face-back");
	addColor(CGAL_EDGE_FRONT_COLOR, "cgal-edge-front");
	addColor(CGAL_EDGE_BACK_COLOR, "cgal-edge-back");
	addColor(CGAL_EDGE_2D_COLOR, "cgal-edge-2d");
	addColor(CROSSHAIR_COLOR, "crosshair");
    } catch (const std::exception & e) {
	PRINTB("Error reading color scheme file '%s': %s", path.c_str() % e.what());
	_name = "";
	_index = 0;
	_show_in_gui = false;
    }
}

RenderColorScheme::~RenderColorScheme()
{
}

bool RenderColorScheme::valid() const
{
    return !_name.empty();
}

const std::string & RenderColorScheme::name() const
{
    return _name;
}

int RenderColorScheme::index() const
{
    return _index;
}

bool RenderColorScheme::showInGui() const
{
    return _show_in_gui;
}

ColorScheme & RenderColorScheme::colorScheme()
{
    return _color_scheme;
}

const boost::property_tree::ptree & RenderColorScheme::propertyTree() const
{
    return pt;
}

void RenderColorScheme::addColor(RenderColor colorKey, std::string key)
{
    const boost::property_tree::ptree& colors = pt.get_child("colors");
    std::string color = colors.get<std::string>(key);
    if ((color.length() == 7) && (color.at(0) == '#')) {
	char *endptr;
	unsigned int val = strtol(color.substr(1).c_str(), &endptr, 16);
	int r = (val >> 16) & 0xff;
	int g = (val >> 8) & 0xff;
	int b = val & 0xff;
	_color_scheme.insert(ColorScheme::value_type(colorKey, Color4f(r, g, b)));
    } else {
	throw std::invalid_argument(std::string("invalid color value for key '") + key + "': '" + color + "'");
    }
}

ColorMap *ColorMap::inst(bool erase)
{
	static ColorMap *instance = new ColorMap;
	if (erase) {
		delete instance;
		instance = NULL;
	}
	return instance;
}

ColorMap::ColorMap() {
	boost::unordered_map<std::string, Color4f> tmpwebcolors = map_list_of
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
	webcolors = tmpwebcolors;
	
	colorschemes = enumerateColorSchemes();
}

const ColorScheme &ColorMap::defaultColorScheme() const
{
    return *findColorScheme("Cornfield");
}

const ColorScheme *ColorMap::findColorScheme(const std::string &name) const
{
    for (colorscheme_set_t::const_iterator it = colorschemes.begin();it != colorschemes.end();it++) {
	RenderColorScheme *scheme = (*it).second.get();
	if (name == scheme->name()) {
	    return &scheme->colorScheme();
	}
    }

    return NULL;
}

std::list<std::string> ColorMap::colorSchemeNames(bool guiOnly) const
{
    std::list<std::string> colorSchemes;
    for (colorscheme_set_t::const_iterator it = colorschemes.begin();it != colorschemes.end();it++) {
	RenderColorScheme *scheme = (*it).second.get();
	if (guiOnly && !scheme->showInGui()) {
	    continue;
	}
        colorSchemes.push_back(scheme->name());
    }
	
    return colorSchemes;
}

Color4f ColorMap::getColor(const ColorScheme &cs, const RenderColor rc)
{
	if (cs.count(rc)) return cs.at(rc);
	if (ColorMap::inst()->defaultColorScheme().count(rc)) return ColorMap::inst()->defaultColorScheme().at(rc);
	return Color4f(0, 0, 0, 127);
}

ColorMap::colorscheme_set_t ColorMap::enumerateColorSchemes()
{
    const fs::path resources = PlatformUtils::resourcesPath();
    const fs::path color_schemes = resources / "color-schemes" / "render";

    colorscheme_set_t result_set;
    fs::directory_iterator end_iter;
    
    if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
	for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
	    if (!fs::is_regular_file(dir_iter->status())) {
		continue;
	    }
	    
	    const fs::path path = (*dir_iter).path();
	    if (!(path.extension().string() == ".json")) {
		continue;
	    }
	    
	    RenderColorScheme *colorScheme = new RenderColorScheme(path);
	    if (colorScheme->valid()) {
		result_set.insert(colorscheme_set_t::value_type(colorScheme->index(), boost::shared_ptr<RenderColorScheme>(colorScheme)));
	    } else {
		delete colorScheme;
	    }
	}
    }
    
    return result_set;
}