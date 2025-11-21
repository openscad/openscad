#pragma once

#include <memory>
#include <string>

#include "core/CurveDiscretizer.h"
#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/Value.h"

class CurveDiscretizer;

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : AbstractPolyNode(mi), discretizer(std::move(discretizer))
  {
    convexity = 0;
    angle = 360;
    start = 0;
  }

  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  CurveDiscretizer discretizer;
  double angle, start;
};
