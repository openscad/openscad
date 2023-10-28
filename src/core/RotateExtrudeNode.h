#pragma once

#include "node.h"
#include "Value.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode() : RotateExtrudeNode(new ModuleInstantiation("rotate_extrude")) {}
  RotateExtrudeNode(ModuleInstantiation *mi) : AbstractPolyNode(mi) {
    convexity = 0;
    fn = fs = fa = 0;
    origin_x = origin_y = scale = 0;
    angle = 360;
  }
  RotateExtrudeNode(double angle, int convexity);
  RotateExtrudeNode(double angle);
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  double fn, fs, fa;
  double origin_x, origin_y, scale, angle;
  Filename filename;
  std::string layername;
};
