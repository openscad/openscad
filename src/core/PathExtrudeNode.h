#pragma once

#include "node.h"
#include "Value.h"
#include <src/geometry/linalg.h>
#ifdef ENABLE_PYTHON
#include <src/python/python_public.h>
#endif

class PathExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  PathExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }

  double origin_x = 0.0, origin_y = 0.0;
  double fn = 0.0, fs = 0.0, fa = 0.0;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  unsigned int convexity = 1u;
  unsigned int slices = 1u, segments = 0u;
  bool has_twist = false, has_slices = false, has_segments = false;
  double xdir_x = 1.0, xdir_y = 0.0, xdir_z = 0.0;
  std::vector<Vector4d> path;
  bool closed = false;
  bool allow_intersect = false;
#ifdef ENABLE_PYTHON
  void *profile_func;
  void *twist_func;
#endif
};
