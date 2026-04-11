#pragma once

#include "node.h"
#include "Value.h"
#include <src/geometry/linalg.h>
#include "src/geometry/PolySet.h"

class OversampleNode : public LeafNode
{
public:
  OversampleNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "oversample( n = " << n << ", round = " << round << ")";
    return stream.str();
  }
  std::string name() const override { return "oversample"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
  int n;
  int round;
};
