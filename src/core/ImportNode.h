#pragma once

#include <boost/optional.hpp>

#include "node.h"
#include "Value.h"

enum class ImportType {
  UNKNOWN,
  AMF,
  _3MF,
  STL,
  OFF,
  SVG,
  DXF,
  NEF3,
  OBJ,
};

class ImportNode : public LeafNode
{
public:
  constexpr static double SVG_DEFAULT_DPI = 72.0;

  VISITABLE();
  ImportNode(const ModuleInstantiation *mi, ImportType type) : LeafNode(mi), type(type) { }
  std::string toString() const override;
  std::string name() const override;

  ImportType type;
  Filename filename;
  boost::optional<std::string> id;
  boost::optional<std::string> layer;
  int convexity;
  bool center;
  double dpi;
  double fn, fs, fa;
  double origin_x, origin_y, scale;
  double width, height;
  const class Geometry *createGeometry() const override;
};
