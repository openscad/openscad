#pragma once

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <list>
#include <memory.h>

#include "geometry/linalg.h"

#include <filesystem>
#include <boost/property_tree/ptree.hpp>

namespace fs = std::filesystem;

enum class RenderColor {
  BACKGROUND_COLOR,
  BACKGROUND_STOP_COLOR,
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

using ColorScheme = std::map<RenderColor, Color4f>;

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
  RenderColorScheme(const fs::path& path);
  virtual ~RenderColorScheme() = default;

  [[nodiscard]] const std::string& name() const;
  [[nodiscard]] int index() const;
  [[nodiscard]] bool valid() const;
  [[nodiscard]] bool showInGui() const;
  ColorScheme& colorScheme();
  [[nodiscard]] const boost::property_tree::ptree& propertyTree() const;

private:
  [[nodiscard]] std::string path() const;
  [[nodiscard]] std::string error() const;
  void addColor(RenderColor colorKey, const std::string& key);

  friend class ColorMap;
};

class ColorMap
{
  using colorscheme_set_t = std::multimap<int, std::shared_ptr<RenderColorScheme>, std::less<>>;

public:
  static ColorMap *inst(bool erase = false);

  [[nodiscard]] const char *defaultColorSchemeName() const;
  [[nodiscard]] const ColorScheme& defaultColorScheme() const;
  [[nodiscard]] const ColorScheme *findColorScheme(const std::string& name) const;
  [[nodiscard]] std::list<std::string> colorSchemeNames(bool guiOnly = false) const;

  static Color4f getColor(const ColorScheme& cs, const RenderColor rc);
  static Color4f getContrastColor(const Color4f& col);
  static Color4f getColorHSV(const Color4f& col);

private:
  ColorMap();
  virtual ~ColorMap() = default;
  void dump() const;
  colorscheme_set_t enumerateColorSchemes();
  void enumerateColorSchemesInPath(colorscheme_set_t& result_set, const fs::path& path);
  colorscheme_set_t colorSchemeSet;
};
