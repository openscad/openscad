#pragma once

#include <string>

#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/Value.h"
#include "core/TessellationControl.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi, TessellationControl&& tess)
    : AbstractPolyNode(mi), tess(tess)
  {
    convexity = 0;
    angle = 360;
    start = 0;
  }
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  TessellationControl tess;
  double angle, start;
};
