#pragma once

#include <memory>
#include <string>

#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/Value.h"

class TessellationControl;

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi, std::shared_ptr<TessellationControl> tessFIXME)
    : AbstractPolyNode(mi), tessFIXME(std::move(tessFIXME))
  {
    convexity = 0;
    angle = 360;
    start = 0;
  }
  ~RotateExtrudeNode();
  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  std::shared_ptr<TessellationControl> tessFIXME;
  double angle, start;
};
