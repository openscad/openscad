#include "colormap.h"
#include "printutils.h"
#include "PlatformUtils.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

namespace fs=boost::filesystem;

static const char *DEFAULT_COLOR_SCHEME_NAME = "Cornfield";

// See http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
static void rgbtohsv(float r, float g, float b, float &h, float &s, float &v)
{
    float K = 0.f;

    if (g < b)
    {
        std::swap(g, b);
        K = -1.f;
    }

    if (r < g)
    {
        std::swap(r, g);
        K = -2.f / 6.f - K;
    }

    float chroma = r - std::min(g, b);
    h = fabs(K + (g - b) / (6.f * chroma + 1e-20f));
    s = chroma / (r + 1e-20f);
    v = r;
}

RenderColorScheme::RenderColorScheme() : _path("")
{
	_name = DEFAULT_COLOR_SCHEME_NAME;
	_index = 1000;
	_show_in_gui = true;

	_color_scheme.insert(ColorScheme::value_type(RenderColor::BACKGROUND_COLOR, Color4f(0xff, 0xff, 0xe5)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::AXES_COLOR, Color4f(0x00, 0x00, 0x00)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::OPENCSG_FACE_FRONT_COLOR, Color4f(0xf9, 0xd7, 0x2c)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::OPENCSG_FACE_BACK_COLOR, Color4f(0x9d, 0xcb, 0x51)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_FACE_FRONT_COLOR, Color4f(0xf9, 0xd7, 0x2c)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_FACE_2D_COLOR, Color4f(0x00, 0xbf, 0x99)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_FACE_BACK_COLOR, Color4f(0x9d, 0xcb, 0x51)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_EDGE_FRONT_COLOR, Color4f(0xff, 0xec, 0x5e)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_EDGE_BACK_COLOR, Color4f(0xab, 0xd8, 0x56)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CGAL_EDGE_2D_COLOR, Color4f(0xff, 0x00, 0x00)));
	_color_scheme.insert(ColorScheme::value_type(RenderColor::CROSSHAIR_COLOR, Color4f(0x80, 0x00, 0x00)));
}

RenderColorScheme::RenderColorScheme(fs::path path) : _path(path)
{
    try {
	boost::property_tree::read_json(path.generic_string().c_str(), pt);
	_name = pt.get<std::string>("name");
	_index = pt.get<int>("index");
	_show_in_gui = pt.get<bool>("show-in-gui");
	
	addColor(RenderColor::BACKGROUND_COLOR, "background");
	addColor(RenderColor::AXES_COLOR, "axes-color");
	addColor(RenderColor::OPENCSG_FACE_FRONT_COLOR, "opencsg-face-front");
	addColor(RenderColor::OPENCSG_FACE_BACK_COLOR, "opencsg-face-back");
	addColor(RenderColor::CGAL_FACE_FRONT_COLOR, "cgal-face-front");
	addColor(RenderColor::CGAL_FACE_2D_COLOR, "cgal-face-2d");
	addColor(RenderColor::CGAL_FACE_BACK_COLOR, "cgal-face-back");
	addColor(RenderColor::CGAL_EDGE_FRONT_COLOR, "cgal-edge-front");
	addColor(RenderColor::CGAL_EDGE_BACK_COLOR, "cgal-edge-back");
	addColor(RenderColor::CGAL_EDGE_2D_COLOR, "cgal-edge-2d");
	addColor(RenderColor::CROSSHAIR_COLOR, "crosshair");
    } catch (const std::exception & e) {
			PRINTB("Error reading color scheme file '%s': %s", path.generic_string().c_str() % e.what());
	_error = e.what();
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

std::string RenderColorScheme::path() const
{
    return _path.string();
}

std::string RenderColorScheme::error() const
{
    return _error;
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
		instance = nullptr;
	}
	return instance;
}

ColorMap::ColorMap()
{
    colorSchemeSet = enumerateColorSchemes();

    dump();
}

ColorMap::~ColorMap()
{
}

const char * ColorMap::defaultColorSchemeName() const
{
    return DEFAULT_COLOR_SCHEME_NAME;
}

const ColorScheme &ColorMap::defaultColorScheme() const
{
    return *findColorScheme(DEFAULT_COLOR_SCHEME_NAME);
}

const ColorScheme *ColorMap::findColorScheme(const std::string &name) const
{
    for (colorscheme_set_t::const_iterator it = colorSchemeSet.begin();it != colorSchemeSet.end();it++) {
	RenderColorScheme *scheme = (*it).second.get();
	if (name == scheme->name()) {
	    return &scheme->colorScheme();
	}
    }

    return nullptr;
}

void ColorMap::dump() const
{
    PRINTD("Listing available color schemes...");
    
    std::list<std::string> names = colorSchemeNames();
    unsigned int length = 0;
    for (std::list<std::string>::const_iterator it = names.begin();it != names.end();it++) {
	length = (*it).length() > length ? (*it).length() : length;
    }

    for (colorscheme_set_t::const_iterator it = colorSchemeSet.begin();it != colorSchemeSet.end();it++) {
	const RenderColorScheme *cs = (*it).second.get();
	const char gui = cs->showInGui() ? 'G' : '-';
	if (cs->path().empty()) {
	    PRINTDB("%6d:%c: %s (built-in)", cs->index() % gui % boost::io::group(std::setw(length), cs->name()));
	} else {
	    PRINTDB("%6d:%c: %s from %s", cs->index() % gui % boost::io::group(std::setw(length), cs->name()) % cs->path());
	}
    }
    PRINTD("done.");
}

std::list<std::string> ColorMap::colorSchemeNames(bool guiOnly) const
{
    std::list<std::string> colorSchemeNames;
    for (colorscheme_set_t::const_iterator it = colorSchemeSet.begin();it != colorSchemeSet.end();it++) {
	const RenderColorScheme *scheme = (*it).second.get();
	if (guiOnly && !scheme->showInGui()) {
	    continue;
	}
        colorSchemeNames.push_back(scheme->name());
    }
	
    return colorSchemeNames;
}

Color4f ColorMap::getColor(const ColorScheme &cs, const RenderColor rc)
{
	if (cs.count(rc)) return cs.at(rc);
	if (ColorMap::inst()->defaultColorScheme().count(rc)) return ColorMap::inst()->defaultColorScheme().at(rc);
	return Color4f(0, 0, 0, 127);
}

Color4f ColorMap::getColorHSV(const Color4f &col)
{
	float h, s, v;
	rgbtohsv(col[0], col[1], col[2], h, s, v);
	return Color4f(h, s, v, col[3]);
}

/**
 * Calculate contrast color. Based on the article 
 * http://gamedev.stackexchange.com/questions/38536/given-a-rgb-color-x-how-to-find-the-most-contrasting-color-y
 * 
 * @param col the input color
 * @return a color with high contrast to the input color
 */
Color4f ColorMap::getContrastColor(const Color4f &col)
{
	Color4f hsv = ColorMap::getColorHSV(col);
	float Y = 0.2126 * col[0] + 0.7152 * col[1] + 0.0722 * col[2];
	float S = hsv[1];

	if (S < 0.5) {
		// low saturation, choose between black / white based on luminance Y
		float val = Y > 0.5 ? 0.0f : 1.0f;
		return Color4f(val, val, val, 1.0f);
	} else {
		float H = 360 * hsv[0];
		if ((H < 60) || (H > 300)) {
			return Color4f(0.0f, 1.0f, 1.0f, 1.0f); // red -> cyan
		} else if (H < 180) {
			return Color4f(1.0f, 0.0f, 1.0f, 1.0f); // green -> magenta
		} else {
			return Color4f(1.0f, 1.0f, 0.0f, 1.0f); // blue -> yellow
		}
	}
}

void ColorMap::enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path basePath)
{
    const fs::path color_schemes = basePath / "color-schemes" / "render";

    PRINTDB("Enumerating color schemes from '%s'", color_schemes.generic_string().c_str());
    
    fs::directory_iterator end_iter;
    
    if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
	for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
	    if (!fs::is_regular_file(dir_iter->status())) {
		continue;
	    }
	    
	    const fs::path path = (*dir_iter).path();
	    if (!(path.extension() == ".json")) {
		continue;
	    }
	    
	    RenderColorScheme *colorScheme = new RenderColorScheme(path);
	    if (colorScheme->valid() && (findColorScheme(colorScheme->name()) == nullptr)) {
		result_set.insert(colorscheme_set_t::value_type(colorScheme->index(), shared_ptr<RenderColorScheme>(colorScheme)));
		PRINTDB("Found file '%s' with color scheme '%s' and index %d",
			colorScheme->path() % colorScheme->name() % colorScheme->index());
	    } else {
		PRINTDB("Invalid file '%s': %s", colorScheme->path() % colorScheme->error());
		delete colorScheme;
	    }
	}
    }
}

ColorMap::colorscheme_set_t ColorMap::enumerateColorSchemes()
{
    colorscheme_set_t result_set;

    RenderColorScheme *defaultColorScheme = new RenderColorScheme();
    result_set.insert(colorscheme_set_t::value_type(defaultColorScheme->index(),
	    shared_ptr<RenderColorScheme>(defaultColorScheme)));
    enumerateColorSchemesInPath(result_set, PlatformUtils::resourceBasePath());
    enumerateColorSchemesInPath(result_set, PlatformUtils::userConfigPath());
    
    return result_set;
}
