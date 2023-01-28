#pragma once

#include "node.h"
#include "Value.h"
#include <linalg.h>
#ifdef ENABLE_PYTHON
#include <Python.h>
#endif

class PathExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  PathExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }

  double origin_x = 0.0, origin_y = 0.0;
  double fn = 0.0, fs = 0.0, fa = 0.0;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  unsigned int convexity = 1u;
  unsigned int slices = 1u, segments = 0u;
  bool has_twist = false, has_slices = false, has_segments = false;
  bool center = false;
  double xdir_x=1.0, xdir_y=0.0, xdir_z=0.0;
  std::vector<Vector3d> path;
  bool closed=false;
 #ifdef ENABLE_PYTHON
  PyObject *profile_func;
 #endif  

  Filename filename;
  std::string layername;
};
