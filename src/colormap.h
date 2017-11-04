#pragma once

#include <map>
#include <string>
#include <list>
#include "linalg.h"

#include "memory.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

namespace fs = boost::filesystem;

enum class RenderColor {
	BACKGROUND_COLOR,
	AXES_COLOR,
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

typedef std::map<RenderColor, Color4f> ColorScheme;

class RenderColorScheme
{
private:
	const fs::path _path;

	boost::property_tree::ptree pt;
	std::string _name;
	std::string _error;
	int _index;
	bool _show_in_gui;

	ColorScheme _color_scheme;

public:
	/**
	 * Constructor for the default color scheme Cornfield.
	 */
	RenderColorScheme();
	/**
	 * Constructor for reading external JSON files.
	 */
	RenderColorScheme(const fs::path path);
	virtual ~RenderColorScheme();

	const std::string &name() const;
	int index() const;
	bool valid() const;
	bool showInGui() const;
	ColorScheme &colorScheme();
	const boost::property_tree::ptree &propertyTree() const;

private:
	std::string path() const;
	std::string error() const;
	void addColor(RenderColor colorKey, std::string key);

	friend class ColorMap;
};

class ColorMap
{
	typedef std::multimap<int, shared_ptr<RenderColorScheme>, std::less<int>> colorscheme_set_t;

public:
	static ColorMap *inst(bool erase = false);

	const char *defaultColorSchemeName() const;
	const ColorScheme &defaultColorScheme() const;
	const ColorScheme *findColorScheme(const std::string &name) const;
	std::list<std::string> colorSchemeNames(bool guiOnly = false) const;

	static Color4f getColor(const ColorScheme &cs, const RenderColor rc);
	static Color4f getContrastColor(const Color4f &col);
	static Color4f getColorHSV(const Color4f &col);

private:
	ColorMap();
	virtual ~ColorMap();
	void dump() const;
	colorscheme_set_t enumerateColorSchemes();
	void enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path path);
	colorscheme_set_t colorSchemeSet;
};
