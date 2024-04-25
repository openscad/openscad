#pragma once

#include "node.h"
#include "Value.h"
#include "linalg.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
    convexity = 0;
    fn = fs = fa = 0;
    origin_x = origin_y = scale = 0;
    angle = 360;
  }
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  double fn, fs, fa;
  double origin_x, origin_y, scale, angle;
  std::string method;
  Vector3d v;
  Filename filename;
  std::string layername;
};
