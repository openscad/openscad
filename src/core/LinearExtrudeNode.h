#pragma once

#include <string>

#include "core/CurveDiscretizer.h"
#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/Value.h"
#include "geometry/linalg.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  LinearExtrudeNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : AbstractPolyNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }

  Vector3d height = Vector3d(0, 0, 1);
  CurveDiscretizer discretizer;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  unsigned int convexity = 1u;
  unsigned int slices = 1u, segments = 0u;
  bool has_twist = false, has_slices = false, has_segments = false;
  bool center = false;
};
