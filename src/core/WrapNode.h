#pragma once

#include "node.h"
//#include "Value.h"
#include "Geometry.h"

class WrapNode : public AbstractPolyNode
{
public:
  VISITABLE();
  WrapNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "wrap"; }
  double r;
  double fn, fa, fs;
};
