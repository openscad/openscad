/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "genlang/genlang.h"
#include <climits>
#include <Python.h>
#include "pyopenscad.h"
#include <TransformNode.h>
#include <CgalAdvNode.h>
#include <CsgOpNode.h>
#include "pyfunctions.h"

PyObject *python_csg_core(std::shared_ptr<CsgOpNode>& node,
                          const std::vector<std::shared_ptr<AbstractNode>>& childs)
{
  PyTypeObject *type = &PyOpenSCADType;
  for (size_t i = 0; i < childs.size(); i++) {
    const auto& child = childs[i];
    if (child.get() == void_node.get()) {
      if (node->type == OpenSCADOperator::DIFFERENCE && i == 0)
        return PyOpenSCADObjectFromNode(type, void_node);
      if (node->type == OpenSCADOperator::INTERSECTION) return PyOpenSCADObjectFromNode(type, void_node);
    } else if (child.get() == full_node.get()) {
      if (node->type == OpenSCADOperator::UNION) return PyOpenSCADObjectFromNode(type, full_node);
      if (node->type == OpenSCADOperator::DIFFERENCE) {
        if (i == 0) return PyOpenSCADObjectFromNode(type, full_node);  // eigentlich negativ
        else return PyOpenSCADObjectFromNode(type, void_node);
      }
    } else node->children.push_back(child);
  }
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE();
  int i;
  auto node = std::make_shared<CsgOpNode>(instance, mode);
  node->r = 0;
  node->fn = 1;
  PyObject *obj;
  // child_dict owns one strong dict ref per accepted arg; destructor
  // releases all of them on every exit path (including the early
  // `return nullptr` on parsing errors below).
  std::vector<PyObjectUniquePtr> child_dict;
  std::shared_ptr<AbstractNode> child;
  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      std::string keystr;
      if (!python_pyobject_to_utf8(key, keystr, "CSG keyword argument")) {
        return nullptr;
      }
      if (keystr == "r") {
        python_numberval(value, &(node->r), nullptr, 0);
      } else if (keystr == "fn") {
        double fn;
        python_numberval(value, &fn, nullptr);
        node->fn = (int)fn;
      } else {
        PyErr_SetString(PyExc_TypeError, "Unknown parameter name in CSG.");
        return nullptr;
      }
    }
  }
  std::vector<std::shared_ptr<AbstractNode>> child_solid;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    PyObject *dict = nullptr;
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    auto owned_dict = py_owned(dict);
    if (child != NULL) {
      child_solid.push_back(child);
      if (dict != nullptr) child_dict.push_back(std::move(owned_dict));
    } else {
      // Since round 1 of #596, PyOpenSCADObjectToNodeMulti can return
      // nullptr with a Python exception already set (e.g. MemoryError
      // from the dict-merge path). Synthesizing a new TypeError here
      // would overwrite the original; propagate it instead.
      if (PyErr_Occurred()) return nullptr;
      switch (mode) {
      case OpenSCADOperator::UNION:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing union. arguments must be solids or arrays.");
        return nullptr;
        break;
      case OpenSCADOperator::CONCAT:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing concat. arguments must be solids or arrays.");
        return nullptr;
        break;
      case OpenSCADOperator::DIFFERENCE:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing difference. arguments must be solids or arrays.");
        return nullptr;
        break;
      case OpenSCADOperator::INTERSECTION:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing intersection. arguments must be solids or arrays.");
        return nullptr;
        break;
      case OpenSCADOperator::MINKOWSKI: break;
      case OpenSCADOperator::HULL:      break;
      case OpenSCADOperator::FILL:      break;
      case OpenSCADOperator::RESIZE:    break;
      case OpenSCADOperator::OFFSET:    break;
      }
      return NULL;
    }
  }
  PyObject *pyresult = python_csg_core(node, child_solid);

  // Reverse-iterate so the 1st child wins on conflicting keys (later
  // writes overwrite earlier ones). Use the `i-- > 0` pattern so the
  // loop is well-defined when child_dict is empty -- python_csg_sub
  // can be called with zero variadic args (e.g. union()).
  for (size_t i = child_dict.size(); i-- > 0;) {
    PyObject *dict = child_dict[i].get();
    if (dict == nullptr) continue;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
      // PyDict_SetItem can fail (e.g. MemoryError on dict resize);
      // ignoring the return would leave a pending Python exception
      // while we hand `pyresult` back to the caller, which the
      // C-API contract forbids. Drop our reference and propagate.
      if (PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value) < 0) {
        Py_DECREF(pyresult);
        return nullptr;
      }
    }
  }
  return pyresult;
}

PyObject *python_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::UNION);
}

PyObject *python_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject *python_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::INTERSECTION);
}

PyObject *python_oo_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE();
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  node->r = 0;
  node->fn = 1;

  PyObject *obj;
  // Owns one strong dict ref per accepted arg (self plus tuple args).
  // Destructor releases everything on early returns and at end-of-scope.
  std::vector<PyObjectUniquePtr> child_dict;
  std::shared_ptr<AbstractNode> child;
  PyObject *dict = nullptr;

  child = PyOpenSCADObjectToNodeMulti(self, &dict);
  {
    auto owned_self_dict = py_owned(dict);
    if (child != NULL) {
      node->children.push_back(child);
      child_dict.push_back(std::move(owned_self_dict));
    }
  }

  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      std::string keystr;
      if (!python_pyobject_to_utf8(key, keystr, "CSG keyword argument")) {
        return nullptr;
      }
      if (keystr == "r") {
        python_numberval(value, &(node->r), nullptr, 0);
      } else if (keystr == "fn") {
        double fn;
        python_numberval(value, &fn, nullptr, 0);
        node->fn = (int)fn;
      } else {
        PyErr_SetString(PyExc_TypeError, "Unknown parameter name in CSG.");
        return nullptr;
      }
    }
  }
  std::vector<std::shared_ptr<AbstractNode>> child_solid;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    auto owned_arg_dict = py_owned(dict);
    if (child != NULL) {
      child_solid.push_back(child);
      child_dict.push_back(std::move(owned_arg_dict));
    } else {
      // See python_csg_sub above: propagate any pending exception
      // from PyOpenSCADObjectToNodeMulti rather than overwriting it
      // with a generic TypeError.
      if (PyErr_Occurred()) return nullptr;
      switch (mode) {
      case OpenSCADOperator::UNION:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing union. arguments must be solids or arrays.");
        break;
      case OpenSCADOperator::CONCAT:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing union. arguments must be solids or arrays.");
        break;
      case OpenSCADOperator::DIFFERENCE:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing difference. arguments must be solids or arrays.");
        break;
      case OpenSCADOperator::INTERSECTION:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing intersection. arguments must be solids or arrays.");
        break;
      case OpenSCADOperator::MINKOWSKI: break;
      case OpenSCADOperator::HULL:      break;
      case OpenSCADOperator::FILL:      break;
      case OpenSCADOperator::RESIZE:    break;
      case OpenSCADOperator::OFFSET:    break;
      }
      return NULL;
    }
  }

  PyObject *pyresult = python_csg_core(node, child_solid);
  // See python_csg_sub above for the rationale: 1st-child-wins reverse
  // merge with a size_t / `i-- > 0` loop so an empty child_dict (0-arg
  // method call) does not trigger implementation-defined size_t-to-int
  // conversion.
  for (size_t i = child_dict.size(); i-- > 0;) {
    PyObject *dict_item = child_dict[i].get();
    if (dict_item == nullptr) continue;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict_item, &pos, &key, &value)) {
      // Same propagation rule as in python_csg_sub: a PyDict_SetItem
      // failure leaves an exception pending, so propagate it instead
      // of returning pyresult while one is in flight.
      if (PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value) < 0) {
        Py_DECREF(pyresult);
        return nullptr;
      }
    }
  }
  return pyresult;
}

PyObject *python_oo_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::UNION);
}

PyObject *python_oo_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject *python_oo_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::INTERSECTION);
}

PyObject *python_nb_sub(PyObject *arg1, PyObject *arg2, OpenSCADOperator mode)
{
  DECLARE_INSTANCE();
  std::vector<std::shared_ptr<AbstractNode>> child;
  std::vector<PyObjectUniquePtr> child_dict;

  if (arg1 == Py_None && mode == OpenSCADOperator::UNION) {
    Py_INCREF(arg2);
    return arg2;
  }
  if (arg2 == Py_None && mode == OpenSCADOperator::UNION) {
    Py_INCREF(arg1);
    return arg1;
  }
  if (arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) {
    Py_INCREF(arg1);
    return arg1;
  }

  for (int i = 0; i < 2; i++) {
    PyObject *dict = nullptr;
    auto solid = PyOpenSCADObjectToNodeMulti(i == 1 ? arg2 : arg1, &dict);
    child_dict.push_back(py_owned(dict));
    if (solid != nullptr) child.push_back(solid);
    else {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
  }
  auto node = std::make_shared<CsgOpNode>(instance, mode);
  PyObject *pyresult = python_csg_core(node, child);

  python_retrieve_pyname(node);
  for (int i = 1; i >= 0; i--) {
    PyObject *dict_item = child_dict[i].get();
    if (dict_item != nullptr) {
      std::string name = child[i]->getPyName();
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(dict_item, &pos, &key, &value)) {
        PyObject *insert_key = key;
        PyObjectUniquePtr key_mod(nullptr, &PyObjectDeleter);
        if (name.size() > 0) {
          std::string key_str;
          if (python_pyobject_to_utf8(key, key_str, "operator handle name")) {
            std::string handle_name = name + "_" + key_str;
            key_mod.reset(PyUnicode_FromStringAndSize(handle_name.c_str(), handle_name.size()));
            if (key_mod.get() == nullptr) {
              /* OOM while building the handle name -- propagate the
               * MemoryError rather than silently fall back to the
               * original key (which would diverge from the documented
               * naming scheme). Drop our strong ref to pyresult on the
               * way out so we don't leak it. */
              Py_DECREF(pyresult);
              return nullptr;
            }
            insert_key = key_mod.get();
          } else {
            /* Non-str key, or some other non-fatal helper failure --
             * the rename is best-effort, so fall back to inserting
             * under the original key. Clear the helper's TypeError so
             * the next C-API call doesn't trip over a stale exception,
             * but propagate any non-Unicode exception (e.g.
             * MemoryError) up to the caller. */
            if (PyErr_Occurred() != nullptr && !PyErr_ExceptionMatches(PyExc_TypeError)) {
              Py_DECREF(pyresult);
              return nullptr;
            }
            PyErr_Clear();
          }
        }
        if (PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, insert_key, value) < 0) {
          /* PyDict_SetItem only fails on OOM or hash() raising; either
           * way the exception is set, so just propagate it. Drop our
           * strong ref to pyresult on the way out so we don't leak it. */
          Py_DECREF(pyresult);
          return nullptr;
        }
      }
    }
  }
  return pyresult;
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2,
                             int mode)  // 0: translate, 1: scale, 2: translateneg, 3=translate-exp
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict_raw = nullptr;

  PyTypeObject *type = PyOpenSCADObjectType(arg1);
  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict_raw);
  auto child_dict = py_owned(child_dict_raw);
  if (child == nullptr && PyErr_Occurred()) return NULL;
  if (arg2 == nullptr) {
    // arg2 == nullptr means "wrap arg1 in a fresh PyOpenSCADObject"
    // (used by the unary fall-through callers in pyfunctions.cc).
    // PyOpenSCADObjectFromNode dereferences `child`, so a soft-fail
    // null here would build a broken object; raise a clear TypeError
    // instead. The matrix-arithmetic branch further down explicitly
    // tolerates a null `child` (it operates on `arg1` directly via
    // python_number_trans), so don't gate it on `child` here.
    if (child == nullptr) return propagate_or_typeerror("Invalid type for Object in translate/scale");
    return PyOpenSCADObjectFromNode(type, child);
  }
  std::vector<Vector3d> vecs;
  int dragflags = 0;
  if (mode == 3) {
    if (!PyList_Check(arg2)) {
      PyErr_SetString(PyExc_TypeError, "explode arg must be a list");
      return NULL;
    }
    int n = PyList_Size(arg2);
    if (PyList_Size(arg2) > 3) {
      PyErr_SetString(PyExc_TypeError, "explode arg list can have maximal 3 directions");
      return NULL;
    }
    double dmy;
    std::vector<float> vals[3];
    for (int i = 0; i < 3; i++) vals[i].push_back(0.0);
    for (int i = 0; i < n; i++) {
      vals[i].clear();
      auto *item = PyList_GetItem(arg2, i);  // TODO fix here
      if (!python_numberval(item, &dmy, &dragflags, 1 << i)) vals[i].push_back(dmy);
      else if (PyList_Check(item)) {
        int m = PyList_Size(item);
        for (int j = 0; j < m; j++) {
          auto *item1 = PyList_GetItem(item, j);
          if (!python_numberval(item1, &dmy, nullptr, 0)) vals[i].push_back(dmy);
        }
      } else {
        PyErr_SetString(PyExc_TypeError, "Unknown explode spec");
        return NULL;
      }
    }
    for (auto z : vals[2])
      for (auto y : vals[1])
        for (auto x : vals[0]) vecs.push_back(Vector3d(x, y, z));
  } else vecs = python_vectors(arg2, 2, 3, &dragflags);

  if (mode == 0 && vecs.size() == 1) {  // translate on numbers
    PyObject *mat = python_number_trans(arg1, vecs[0], 4);
    if (mat != nullptr) return mat;
  }

  if (vecs.size() > 0) {
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    std::vector<std::shared_ptr<TransformNode>> nodes;
    for (size_t j = 0; j < vecs.size(); j++) {
      std::shared_ptr<TransformNode> node;
      switch (mode) {
      case 0:
      case 3:
        node = std::make_shared<TransformNode>(instance, "translate");
        node->matrix.translate(vecs[j]);
        break;
      case 1:
        node = std::make_shared<TransformNode>(instance, "scale");
        node->matrix.scale(vecs[j]);
        break;
      case 2:
        node = std::make_shared<TransformNode>(instance, "translate");
        node->matrix.translate(-vecs[j]);
        break;
      }
      node->children.push_back(child);
      nodes.push_back(node);
    }
    if (nodes.size() == 1) {
      nodes[0]->dragflags = dragflags;
      PyObject *pyresult = PyOpenSCADObjectFromNode(type, nodes[0]);
      if (child_dict.get() != nullptr) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(child_dict.get(), &pos, &key, &value)) {
          // Same pattern as the python_*_sub merge loops in
          // py_transform.cc: python_number_trans returns a NEW
          // reference, so wrap it in py_owned() to avoid leaking
          // one object per dict entry, and propagate any
          // PyDict_SetItem failure to the caller. python_number_trans
          // returning nullptr can mean either "value is not numeric,
          // fall back" (no exception) or "hard failure" (exception
          // set, e.g. allocation failure inside python_fromvector);
          // disambiguate via PyErr_Occurred().
          auto value1 = py_owned(python_number_trans(value, vecs[0], 4));
          if (value1.get() == nullptr && PyErr_Occurred()) {
            Py_DECREF(pyresult);
            return nullptr;
          }
          PyObject *to_insert = value1.get() != nullptr ? value1.get() : value;
          if (PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, to_insert) < 0) {
            Py_DECREF(pyresult);
            return nullptr;
          }
        }
      }
      return pyresult;
    } else {
      auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
      DECLARE_INSTANCE();
      for (auto x : nodes) node->children.push_back(x->clone());
      return PyOpenSCADObjectFromNode(type, node);
    }
  }
  PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
  return NULL;
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2)
{
  if (PyObject_IsInstance(arg2, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    return python_nb_sub(arg1, arg2, OpenSCADOperator::UNION);
  }
  return python_nb_sub_vec3(arg1, arg2, 0);
}  // solid+solid -> union; solid+vector -> translate

PyObject *python_nb_xor(PyObject *arg1, PyObject *arg2)
{
  if (PyObject_IsInstance(arg2, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    PyObject *dict1_raw = nullptr;
    PyObject *dict2_raw = nullptr;
    auto node1 = PyOpenSCADObjectToNode(arg1, &dict1_raw);
    auto node2 = PyOpenSCADObjectToNode(arg2, &dict2_raw);
    auto dict1 = py_owned(dict1_raw);
    auto dict2 = py_owned(dict2_raw);
    if (node1 == nullptr || node2 == nullptr) {
      PyErr_SetString(PyExc_TypeError, "Error during parsing hull. arguments must be solids.");
      return nullptr;
    }
    DECLARE_INSTANCE();
    auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::HULL);
    node->children.push_back(node1);
    node->children.push_back(node2);
    return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  }
  return python_nb_sub_vec3(arg1, arg2, 3);
}

PyObject *python_nb_remainder(PyObject *arg1, PyObject *arg2)
{
  if (PyObject_IsInstance(arg2, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    PyObject *dict1_raw = nullptr;
    PyObject *dict2_raw = nullptr;
    auto node1 = PyOpenSCADObjectToNode(arg1, &dict1_raw);
    auto node2 = PyOpenSCADObjectToNode(arg2, &dict2_raw);
    auto dict1 = py_owned(dict1_raw);
    auto dict2 = py_owned(dict2_raw);
    if (node1 == nullptr || node2 == nullptr) {
      PyErr_SetString(PyExc_TypeError, "Error during parsing hull. arguments must be solids.");
      return nullptr;
    }
    DECLARE_INSTANCE();
    auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::MINKOWSKI);
    node->children.push_back(node1);
    node->children.push_back(node2);
    return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  }
  Vector3d vec3(0, 0, 0);
  if (!python_vectorval(arg2, 1, 3, &(vec3[0]), &(vec3[1]), &(vec3[2]), nullptr)) {
    return python_rotate_sub(arg1, vec3, NAN, nullptr, 0);
  }

  PyErr_SetString(PyExc_TypeError, "Unknown types for % oprator");
  return nullptr;
}

PyObject *python_nb_mul(PyObject *arg1, PyObject *arg2)
{
  return python_nb_sub_vec3(arg1, arg2, 1);
}  // scale

PyObject *python_nb_or(PyObject *arg1, PyObject *arg2)
{
  return python_nb_sub(arg1, arg2, OpenSCADOperator::UNION);
}

PyObject *python_nb_subtract(PyObject *arg1, PyObject *arg2)
{
  double dmy;
  if (PyList_Check(arg2) && PyList_Size(arg2) > 0) {
    PyObject *sub = PyList_GetItem(arg2, 0);
    if (!python_numberval(sub, &dmy, nullptr, 0) || PyList_Check(sub)) {
      return python_nb_sub_vec3(arg1, arg2, 2);
    }
  }
  return python_nb_sub(arg1, arg2, OpenSCADOperator::DIFFERENCE);  // if its solid
}

PyObject *python_nb_and(PyObject *arg1, PyObject *arg2)
{
  return python_nb_sub(arg1, arg2, OpenSCADOperator::INTERSECTION);
}

PyObject *python_nb_matmult(PyObject *arg1, PyObject *arg2)
{
  return python_multmatrix_sub(arg1, arg2, 0);
}

PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  PyTypeObject *type = &PyOpenSCADType;
  int i;

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
  PyObject *obj;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    type = PyOpenSCADObjectType(obj);
    PyObject *dummydict_raw = nullptr;
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict_raw);
    auto dummydict = py_owned(dummydict_raw);
    if (child != NULL) {
      node->children.push_back(child);
    } else {
      // See python_csg_sub for rationale: propagate any pre-set
      // exception from PyOpenSCADObjectToNodeMulti instead of
      // overwriting it with a synthesized TypeError.
      if (PyErr_Occurred()) return NULL;
      switch (mode) {
      case CgalAdvType::HULL:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing hull. arguments must be solids or arrays.");
        break;
      case CgalAdvType::FILL:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing fill. arguments must be solids or arrays.");
        break;
      case CgalAdvType::RESIZE:    break;
      case CgalAdvType::MINKOWSKI: break;
      }
      return NULL;
    }
  }

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  int convexity = 2;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::MINKOWSKI);
  char *kwlist[] = {"obj1", "obj2", "convexity", NULL};
  PyObject *obj1, *obj2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|i", kwlist, &obj1, &obj2, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing minkowski(object1, object2[, convexity])");
    return NULL;
  }
  PyTypeObject *type = PyOpenSCADObjectType(obj1);
  PyObject *dict1_raw = nullptr;
  PyObject *dict2_raw = nullptr;
  // Validate both conversions before pushing into node->children:
  // PyOpenSCADObjectToNodeMulti can return nullptr with a Python
  // exception already set, and pushing a null shared_ptr would
  // build an invalid node and crash later in evaluation.
  child = PyOpenSCADObjectToNodeMulti(obj1, &dict1_raw);
  auto dict1 = py_owned(dict1_raw);
  if (child == nullptr) return propagate_or_typeerror("Invalid type for first object in minkowski");
  node->children.push_back(child);

  child = PyOpenSCADObjectToNodeMulti(obj2, &dict2_raw);
  auto dict2 = py_owned(dict2_raw);
  if (child == nullptr) return propagate_or_typeerror("Invalid type for second object in minkowski");
  node->children.push_back(child);

  node->convexity = convexity;

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_hull(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_adv_sub(self, args, kwargs, CgalAdvType::HULL);
}

PyObject *python_fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_adv_sub(self, args, kwargs, CgalAdvType::FILL);
}

static int python_resize_disambiguate_convexity(PyObject *args, PyObject *kwargs, PyObject **autosize,
                                                int *convexity, Py_ssize_t ambiguous_positional_count)
{
  PyObject *auto_arg = *autosize;
  if (auto_arg == nullptr || PyBool_Check(auto_arg) || !PyIndex_Check(auto_arg)) return 0;

  if (kwargs != nullptr) {
    if (PyDict_GetItemString(kwargs, "auto") != nullptr) return 0;
    if (PyDict_GetItemString(kwargs, "convexity") != nullptr) return 0;
  }

  if (args == nullptr || PyTuple_Size(args) != ambiguous_positional_count) return 0;

  PyObject *index = PyNumber_Index(auto_arg);
  if (index == nullptr) return 1;
  int overflow = 0;
  const long c = PyLong_AsLongAndOverflow(index, &overflow);
  Py_DECREF(index);
  if (PyErr_Occurred()) return 1;
  if (overflow != 0 || c < INT_MIN || c > INT_MAX) {
    PyErr_SetString(PyExc_OverflowError, "convexity value out of range");
    return 1;
  }
  *convexity = (int)c;
  *autosize = nullptr;
  return 0;
}

static int python_resize_newsize(PyObject *newsize, double *x, double *y, double *z)
{
  *x = *y = *z = 0;
  if (newsize == nullptr) return 0;

  if (PySequence_Check(newsize) && !PyBytes_Check(newsize) && !PyUnicode_Check(newsize)) {
    const Py_ssize_t n = PySequence_Size(newsize);
    if (n < 1 || n > 3) return 1;
    for (Py_ssize_t i = 0; i < n; ++i) {
      PyObject *item = PySequence_GetItem(newsize, i);
      if (item == nullptr) return 1;
      double *target = (i == 0) ? x : (i == 1) ? y : z;
      const int err = python_numberval(item, target, nullptr, 0);
      Py_DECREF(item);
      if (err) return 1;
    }
    return 0;
  }

  return python_vectorval(newsize, 1, 3, x, y, z);
}

static int python_parse_autosize(PyObject *autosize, Eigen::Matrix<bool, 3, 1>& out)
{
  out << false, false, false;
  if (autosize == nullptr) return 0;
  if (PyBool_Check(autosize)) {
    const bool enabled = PyObject_IsTrue(autosize);
    out << enabled, enabled, enabled;
    return 0;
  }
  if (PySequence_Check(autosize) && !PyBytes_Check(autosize) && !PyUnicode_Check(autosize)) {
    const Py_ssize_t n = PySequence_Size(autosize);
    if (n < 1 || n > 3) return 1;
    for (Py_ssize_t i = 0; i < n; ++i) {
      PyObject *item = PySequence_GetItem(autosize, i);
      if (item == nullptr) return 1;
      if (PyBool_Check(item)) {
        out[i] = PyObject_IsTrue(item);
      } else {
        double val = 0;
        if (python_numberval(item, &val, nullptr, 0)) {
          Py_DECREF(item);
          return 1;
        }
        out[i] = val != 0;
      }
      Py_DECREF(item);
    }
    return 0;
  }
  return 1;
}

PyObject *python_resize_core(PyObject *obj, PyObject *newsize, PyObject *autosize, int convexity)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::RESIZE);
  PyObject *child_dict_raw = nullptr;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict_raw);
  auto child_dict = py_owned(child_dict_raw);
  if (child == NULL) return propagate_or_typeerror("Invalid type for Object in resize");

  node->newsize << 0, 0, 0;
  if (newsize != NULL) {
    double x = 0, y = 0, z = 0;
    if (python_resize_newsize(newsize, &x, &y, &z)) {
      if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,
                        "Invalid resize dimensions: expected scalar or 1-3 element sequence");
      }
      return NULL;
    }
    node->newsize[0] = x;
    node->newsize[1] = y;
    node->newsize[2] = z;
  }

  if (python_parse_autosize(autosize, node->autosize)) {
    if (!PyErr_Occurred()) {
      PyErr_SetString(PyExc_TypeError,
                      "Invalid auto argument: expected bool or 1-3 element sequence of bools or "
                      "numbers");
    }
    return NULL;
  }

  node->children.push_back(child);
  node->convexity = convexity;

  auto pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict.get() != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict.get(), &pos, &key, &value)) {
      // Same propagation rule as python_csg_sub et al.: a
      // PyDict_SetItem failure leaves an exception pending, so
      // surface it instead of returning pyresult with a stale
      // exception in flight.
      if (PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value) < 0) {
        Py_DECREF(pyresult);
        return nullptr;
      }
    }
  }
  return pyresult;
}

PyObject *python_resize(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "newsize", "auto", "convexity", NULL};
  PyObject *obj;
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;
  int convexity = 2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOi", kwlist, &obj, &newsize, &autosize,
                                   &convexity)) {
    PyErr_SetString(PyExc_TypeError,
                    "Error during parsing resize(object, newsize[, auto[, convexity]])");
    return NULL;
  }
  if (python_resize_disambiguate_convexity(args, kwargs, &autosize, &convexity, 3)) return NULL;
  return python_resize_core(obj, newsize, autosize, convexity);
}

PyObject *python_oo_resize(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"newsize", "auto", "convexity", NULL};
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;
  int convexity = 2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOi", kwlist, &newsize, &autosize, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing resize(newsize[, auto[, convexity]])");
    return NULL;
  }
  if (python_resize_disambiguate_convexity(args, kwargs, &autosize, &convexity, 2)) return NULL;
  return python_resize_core(obj, newsize, autosize, convexity);
}
