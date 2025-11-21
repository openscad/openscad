#pragma once

#include <memory>
#include <string>

#include "core/CurveDiscretizer.h"
#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/Value.h"
#include "geometry/linalg.h"

#ifdef ENABLE_PYTHON
#include <src/python/python_public.h>
#endif
class CurveDiscretizer;

class CurveDiscretizer;

class CurveDiscretizer;

class RotateExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RotateExtrudeNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : AbstractPolyNode(mi), discretizer(std::move(discretizer))
  {
    convexity = 0;
    origin_x = origin_y = scale = offset_x = offset_y = 0;
    angle = 360;
    start = 0;
  }

  std::string toString() const override;
  std::string name() const override { return "rotate_extrude"; }

  int convexity;
  double angle = 360, start = 0, origin_x = 0, origin_y = 0, scale = 1, offset_x = 0, offset_y = 0;
  double twist = 0;
  std::string method;
  Vector3d v;
#ifdef ENABLE_PYTHON
  void *profile_func;
  void *twist_func;
#endif
  CurveDiscretizer discretizer;
};
