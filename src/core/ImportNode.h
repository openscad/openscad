#pragma once

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
  std::string layername;
  std::string id;
  int convexity;
  bool center;
  double dpi;
  double fn, fs, fa;
  double origin_x, origin_y, scale;
  double width, height;
  const class Geometry *createGeometry() const override;
};
