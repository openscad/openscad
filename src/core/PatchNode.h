#pragma once

#include <string>
#include <vector>

#include "core/node.h"
#include "geometry/linalg.h"

class PatchNode : public LeafNode
{
public:
  PatchNode(std::shared_ptr<const ModuleInstantiation> mi) : LeafNode(std::move(mi)) {}
  PatchNode(const PatchNode& other);  // own copy ctor: keeps proj_func/
                                      // displacement_func correctly alive (Py_XINCREF)
  ~PatchNode() override;              // Py_XDECREF on proj_func/displacement_func

  std::string toString() const override;
  std::string name() const override { return "patch"; }
  std::unique_ptr<const class Geometry> createGeometry() const override;

  std::vector<Vector3d> outer;
  std::vector<std::vector<Vector3d>> holes;
  double grid_spacing_uv = 1.0;

  // Optional tangent/departure direction per boundary point (parallel to
  // 'outer', resp. each entry of 'holes'). Filled in by
  // python_patch_ring_from_shape() from a 2D shape's plane normal; stays
  // empty for a plain point list. Empty = the old, purely linear
  // behavior, see geometry/patch.h.
  std::vector<Vector3d> outer_normal;
  std::vector<std::vector<Vector3d>> holes_normal;

  // Whether the tangents above were requested at all (Python:
  // use_tangents=True). MUST be part of toString(): 'outer'/'holes' (the
  // plain 3D positions) can be identical across two calls while
  // outer_normal/holes_normal differ (once requested with, once without
  // curvature) - without this flag in the cache key, two such PatchNodes
  // would share the same geometry cache entry and the wrong result
  // (linear instead of curved, or vice versa) could be returned.
  bool use_tangents = false;

  // Python function objects (PyObject*), kept opaque so this header
  // doesn't need Python.h. Correctly refcounted in PatchNode.cc.
  void *proj_func = nullptr;
  void *displacement_func = nullptr;

  // Content-based hashes of the two functions (see python_func_content_hash),
  // computed once at node creation - used for toString()/caching.
  std::string proj_func_hash;
  std::string displacement_func_hash;
};
