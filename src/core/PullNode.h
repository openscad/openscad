#pragma once

#include "node.h"
// #include "Value.h"
#include "src/geometry/Geometry.h"

class PullNode : public AbstractPolyNode
{
public:
  VISITABLE();
  PullNode(std::shared_ptr<const ModuleInstantiation> mi) : AbstractPolyNode(std::move(mi)) {}
  std::string toString() const override;
  std::string name() const override { return "pull"; }
  Vector3d anchor;
  Vector3d dir;
};
