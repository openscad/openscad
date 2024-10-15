#pragma once

#include <string>

#include "core/node.h"
#include "core/Value.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
    convexity = 0;
    fn = fs = fa = 0;
    origin_x = origin_y = scale = 0;
    angle = 360;
    scale_x = scale_y = 1;
    trans_x = trans_y = 0;
  }
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  double fn, fs, fa;
  double origin_x, origin_y, scale, angle, scale_x, scale_y, trans_x, trans_y;
  Filename filename;
  std::string layername;
};
