#pragma once

#include <map>
#include <string>
#include <list>
#include "linalg.h"
#include <boost/unordered/unordered_map.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace fs = boost::filesystem;

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

typedef std::map<RenderColor, Color4f> ColorScheme;

class RenderColorScheme
{
private:
        const fs::path path;
        
        boost::property_tree::ptree pt;
        std::string _name;
        int _index;
        bool _show_in_gui;
        
        ColorScheme _color_scheme;
        
public:
        RenderColorScheme(const fs::path path);
        virtual ~RenderColorScheme();
        
        const std::string & name() const;
        int index() const;
        bool valid() const;
        bool showInGui() const;
        ColorScheme & colorScheme();
        const boost::property_tree::ptree & propertyTree() const;

private:
        void addColor(RenderColor colorKey, std::string key);
};

class ColorMap
{
        typedef std::multimap<int, boost::shared_ptr<RenderColorScheme>, std::less<int> > colorscheme_set_t;
    
public:
	static ColorMap *inst(bool erase = false);

	const ColorScheme &defaultColorScheme() const;

	const boost::unordered_map<std::string, Color4f> &webColors() const { return webcolors; }
	const ColorScheme *findColorScheme(const std::string &name) const;
	std::list<std::string> colorSchemeNames(bool guiOnly = false) const;

	static Color4f getColor(const ColorScheme &cs, const RenderColor rc);
	
private:
	ColorMap();
	~ColorMap() {}
        colorscheme_set_t enumerateColorSchemes();

	boost::unordered_map<std::string, Color4f> webcolors;
        colorscheme_set_t colorschemes;
};
