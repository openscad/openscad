#pragma once

#include "node.h"
#include "Value.h"
#include <src/geometry/linalg.h>
#include "src/geometry/PolySet.h"

class FilletNode : public LeafNode
{
public:
  FilletNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "fillet( r = " << r << " fn = " << fn << " minang = " << minang << " )";
    return stream.str();
  }
  std::string name() const override { return "fillet"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
  double r;
  double minang;
  int fn = 2;
};
