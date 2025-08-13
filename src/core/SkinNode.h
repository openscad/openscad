#pragma once

#include "node.h"
#include "Value.h"

class SkinNode : public AbstractPolyNode
{
public:
  VISITABLE();
  SkinNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { convexity = 0; }
  std::string toString() const override;
  std::string name() const override { return "skin"; }

  unsigned int convexity;
  bool has_segments = false;
  unsigned int segments = 0u;
  bool has_interpolate = false;
  bool interpolate = true;
  bool has_align_angle = false;
  unsigned int align_angle = 0;  // x axis going counter-clockwise
};
