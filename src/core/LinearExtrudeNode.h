#pragma once

#include "node.h"
#include "Value.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  LinearExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }

  double height = 100.0;
  double origin_x = 0.0, origin_y = 0.0;
  double fn = 0.0, fs = 0.0, fa = 0.0;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  int convexity = 1;
  int slices = 1, segments = 0;
  bool has_twist = false, has_slices = false, has_segments = false;
  bool center = false;

  Filename filename;
  std::string layername;
};
