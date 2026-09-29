#include "core/PatchNode.h"

#include <Python.h>

#include <cstdint>
#include <sstream>

#include "geometry/PolySet.h"
#include "geometry/patch.h"
#include "python/pyconversion.h"
#include "python/pyopenscad.h"
#include "utils/printutils.h"

// ---------------------------------------------------------------------
// Refcounting: proj_func/displacement_func are incref'd/decref'd here
// (not at the call site in py_primitives.cc), so that EVERY way a
// PatchNode is created or destroyed - including via the generic
// copy-constructor clone path in node_clone.cc (NodeCloneFunc(PatchNode),
// std::make_shared<PatchNode>(*node)) - is handled correctly automatically.
// (See the SheetNode debugging history: a missing INCREF led to
// use-after-free, a missing INCREF on clone to double-free.)
// ---------------------------------------------------------------------

PatchNode::PatchNode(const PatchNode& other) : LeafNode(other)
{
  outer = other.outer;
  holes = other.holes;
  outer_normal = other.outer_normal;
  holes_normal = other.holes_normal;
  use_tangents = other.use_tangents;
  grid_spacing_uv = other.grid_spacing_uv;
  proj_func_hash = other.proj_func_hash;
  displacement_func_hash = other.displacement_func_hash;

  proj_func = other.proj_func;
  displacement_func = other.displacement_func;

  PyGILState_STATE gstate = PyGILState_Ensure();
  Py_XINCREF(static_cast<PyObject *>(proj_func));
  Py_XINCREF(static_cast<PyObject *>(displacement_func));
  PyGILState_Release(gstate);
}

PatchNode::~PatchNode()
{
  if (!Py_IsInitialized()) return;
  PyGILState_STATE gstate = PyGILState_Ensure();
  Py_XDECREF(static_cast<PyObject *>(proj_func));
  Py_XDECREF(static_cast<PyObject *>(displacement_func));
  PyGILState_Release(gstate);
}

namespace {

uint64_t fnv1a_mix(uint64_t h, const char *buf, size_t len)
{
  for (size_t i = 0; i < len; i++) {
    h ^= static_cast<unsigned char>(buf[i]);
    h *= 1099511628211ULL;
  }
  return h;
}

uint64_t hashPoints(uint64_t h, const std::vector<Vector3d>& pts)
{
  const uint64_t pointCount = pts.size();
  h = fnv1a_mix(h, reinterpret_cast<const char *>(&pointCount), sizeof(pointCount));

  for (const auto& p : pts) {
    h = fnv1a_mix(h, reinterpret_cast<const char *>(p.data()), sizeof(double) * 3);
  }
  return h;
}

// -------------------- GIL-safe Python callback calls --------------------
// Convention: proj(p) / displacement(p) each take ONE argument - a
// 3-element Python list [x,y,z] - as in the examples discussed earlier
// (proj_xy(p), lambda p: bump(p[0], p[1], p[2])).

PyObject *pointToPyList(const Vector3d& p)
{
  PyObject *px = PyFloat_FromDouble(p.x());
  PyObject *py = PyFloat_FromDouble(p.y());
  PyObject *pz = PyFloat_FromDouble(p.z());
  PyObject *list = PyList_New(3);
  PyList_SET_ITEM(list, 0, px);  // steals references
  PyList_SET_ITEM(list, 1, py);
  PyList_SET_ITEM(list, 2, pz);
  return list;
}

bool callProjFunc(PyObject *func, const Vector3d& p, Vector2d& out)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject *point = pointToPyList(p);
  PyObject *args = PyTuple_Pack(1, point);
  Py_DECREF(point);

  PyObject *result = PyObject_CallObject(func, args);
  Py_DECREF(args);

  bool ok = false;
  if (result != nullptr) {
    double x = 0, y = 0;
    if (python_vectorval(result, 2, 2, &x, &y, nullptr, nullptr, nullptr) == 0) {
      out = Vector2d(x, y);
      ok = true;
    } else {
      LOG(message_group::Error, "patch(): proj() must return a 2-vector [u,v].");
    }
    Py_DECREF(result);
  } else {
    std::string errorstr;
    python_catch_error(errorstr);
    LOG(message_group::Error, "patch(): error calling proj(): %1$s", errorstr.c_str());
  }

  PyGILState_Release(gstate);
  return ok;
}

bool callDisplacementFunc(PyObject *func, const Vector3d& p, double& out)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject *point = pointToPyList(p);
  PyObject *args = PyTuple_Pack(1, point);
  Py_DECREF(point);

  PyObject *result = PyObject_CallObject(func, args);
  Py_DECREF(args);

  bool ok = false;
  if (result != nullptr) {
    if (PyFloat_Check(result) || PyLong_Check(result)) {
      out = PyFloat_AsDouble(result);
      ok = true;
    } else {
      LOG(message_group::Error, "patch(): displacement() must return a number.");
    }
    Py_DECREF(result);
  } else {
    std::string errorstr;
    python_catch_error(errorstr);
    LOG(message_group::Error, "patch(): error calling displacement(): %1$s", errorstr.c_str());
  }

  PyGILState_Release(gstate);
  return ok;
}

}  // namespace

std::string PatchNode::toString() const
{
  uint64_t h = 1469598103934665603ULL;
  h = hashPoints(h, outer);
  for (const auto& hole : holes) h = hashPoints(h, hole);
  // Fold the tangents into the hash too: 'outer'/'holes' (the plain 3D
  // positions) can be identical across two PatchNodes while
  // outer_normal/holes_normal (and hence the result) differ - see the
  // comment on 'use_tangents' in PatchNode.h.
  h = hashPoints(h, outer_normal);
  for (const auto& hn : holes_normal) h = hashPoints(h, hn);

  std::ostringstream stream;
  stream << this->name() << "(outer_n=" << outer.size() << ", holes_n=" << holes.size()
         << ", points_hash=" << std::hex << h << std::dec << ", proj=" << proj_func_hash
         << ", displacement=" << displacement_func_hash << ", grid_spacing_uv=" << grid_spacing_uv
         << ", use_tangents=" << (use_tangents ? 1 : 0) << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> PatchNode::createGeometry() const
{
  // -------- DEBUG --------
  // toString() is (per its own comment above) exactly the basis for the
  // geometry cache key. If two different patch() calls in the same
  // script produce the same string here (e.g. because proj_func_hash
  // collides for two functionally different "proj" functions that
  // happen to share a name), that fully explains a cache mix-up between
  // the two calls.

  auto *proj_py = static_cast<PyObject *>(proj_func);
  auto *disp_py = static_cast<PyObject *>(displacement_func);

  bool failed = false;

  // No proj() given (proj_py == nullptr, e.g. patch() called without a
  // proj argument) -> pass along an empty std::function so patch()
  // itself picks a projection automatically (see computeAutoProj() in
  // geometry/patch.cc). Do NOT call any fallback function here already -
  // an empty std::function is the agreed-upon signal for "please
  // determine automatically".
  std::function<Vector2d(const Vector3d&)> projFn;
  if (proj_py != nullptr) {
    projFn = [proj_py, &failed](const Vector3d& p) -> Vector2d {
      Vector2d out(0, 0);
      if (!callProjFunc(proj_py, p, out)) failed = true;
      return out;
    };
  }
  auto dispFn = [&](const Vector3d& p) -> double {
    double out = 0.0;
    if (disp_py != nullptr) {
      if (!callDisplacementFunc(disp_py, p, out)) failed = true;
    }
    return out;
  };

  auto result = patch(outer, holes, projFn, grid_spacing_uv, dispFn, outer_normal, holes_normal);

  if (failed || result == nullptr) {
    LOG(message_group::Error, "patch(): geometry generation failed.");
    return std::make_unique<PolySet>(3);  // empty but valid result instead of a crash
  }
  return result;
}
