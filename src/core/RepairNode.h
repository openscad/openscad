#pragma once

#include "node.h"
#include "src/geometry/Geometry.h"

class RepairNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RepairNode(std::shared_ptr<const ModuleInstantiation> mi) : AbstractPolyNode(std::move(mi)) {}
  std::string toString() const override;
  std::string name() const override { return "repair"; }
  Color4f color;
};
