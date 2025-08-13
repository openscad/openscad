#pragma once

#include "node.h"
// #include "Value.h"
#include "src/geometry/Geometry.h"

class PolygonNode;
class WrapNode : public AbstractPolyNode
{
public:
  VISITABLE();
  WrapNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "wrap"; }
  double r;
  std::shared_ptr<const AbstractNode> shape;
  double fn, fa, fs;
};
