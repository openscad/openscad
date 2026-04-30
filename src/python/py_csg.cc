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
  std::vector<PyObject *> child_dict;
  std::shared_ptr<AbstractNode> child;
  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str = PyBytes_AS_STRING(value1);
      if (value_str == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
        return nullptr;
      } else if (strcmp(value_str, "r") == 0) {
        python_numberval(value, &(node->r), nullptr, 0);
      } else if (strcmp(value_str, "fn") == 0) {
        double fn;
        python_numberval(value, &fn, nullptr);
        node->fn = (int)fn;
      } else {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
        return nullptr;
      }
    }
  }
  std::vector<std::shared_ptr<AbstractNode>> child_solid;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    PyObject *dict = nullptr;
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    if (dict != nullptr) {
      child_dict.push_back(dict);
    }
    if (child != NULL) {
      child_solid.push_back(child);
    } else {
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

  for (int i = child_dict.size() - 1; i >= 0; i--)  // merge from back  to give 1st child most priority
  {
    auto& dict = child_dict[i];
    if (dict == nullptr) continue;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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
  std::vector<PyObject *> child_dict;
  std::shared_ptr<AbstractNode> child;
  PyObject *dict;

  dict = nullptr;
  child = PyOpenSCADObjectToNodeMulti(self, &dict);
  if (child != NULL) {
    node->children.push_back(child);
    child_dict.push_back(dict);
  }

  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str = PyBytes_AS_STRING(value1);
      if (value_str == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
        return nullptr;
      } else if (strcmp(value_str, "r") == 0) {
        python_numberval(value, &(node->r), nullptr, 0);
      } else if (strcmp(value_str, "fn") == 0) {
        double fn;
        python_numberval(value, &fn, nullptr, 0);
        node->fn = (int)fn;
      } else {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
        return nullptr;
      }
    }
  }
  std::vector<std::shared_ptr<AbstractNode>> child_solid;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    child_dict.push_back(dict);
    if (child != NULL) {
      child_solid.push_back(child);
    } else {
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
  for (int i = child_dict.size() - 1; i >= 0; i--)  // merge from back  to give 1st child most priority
  {
    auto& dict = child_dict[i];
    if (dict == nullptr) continue;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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
  std::vector<PyObject *> child_dict;

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
    PyObject *dict;
    dict = nullptr;
    auto solid = PyOpenSCADObjectToNodeMulti(i == 1 ? arg2 : arg1, &dict);
    child_dict.push_back(dict);
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
    if (child_dict[i] != nullptr) {
      std::string name = child[i]->getPyName();
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(child_dict[i], &pos, &key, &value)) {
        if (name.size() > 0) {
          PyObject *key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
          const char *key_str = PyBytes_AS_STRING(key1);
          std::string handle_name = name + "_" + key_str;
          PyObject *key_mod =
            PyUnicode_FromStringAndSize(handle_name.c_str(), strlen(handle_name.c_str()));
          PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key_mod, value);
        } else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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
  PyObject *child_dict;

  PyTypeObject *type = PyOpenSCADObjectType(arg1);
  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict);
  if (arg2 == nullptr) return PyOpenSCADObjectFromNode(type, child);
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
      if (child_dict != nullptr) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(child_dict, &pos, &key, &value)) {
          PyObject *value1 = python_number_trans(value, vecs[0], 4);
          if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
          else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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
  return python_nb_sub_vec3(arg1, arg2, 0);
}  // translate

PyObject *python_nb_xor(PyObject *arg1, PyObject *arg2)
{
  PyObject *dummy_dict;
  if (PyObject_IsInstance(arg2, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    auto node1 = PyOpenSCADObjectToNode(arg1, &dummy_dict);
    auto node2 = PyOpenSCADObjectToNode(arg2, &dummy_dict);
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
    PyObject *dummy_dict;
    auto node1 = PyOpenSCADObjectToNode(arg1, &dummy_dict);
    auto node2 = PyOpenSCADObjectToNode(arg2, &dummy_dict);
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
  PyObject *dummydict;

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
  PyObject *obj;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    type = PyOpenSCADObjectType(obj);
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child != NULL) {
      node->children.push_back(child);
    } else {
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
  PyObject *dummydict;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|i", kwlist, &obj1, &obj2, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing minkowski(object1, object2[, convexity])");
    return NULL;
  }
  PyTypeObject *type = PyOpenSCADObjectType(obj1);
  child = PyOpenSCADObjectToNodeMulti(obj1, &dummydict);
  node->children.push_back(child);

  child = PyOpenSCADObjectToNodeMulti(obj2, &dummydict);
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

PyObject *python_resize_core(PyObject *obj, PyObject *newsize, PyObject *autosize, int convexity)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::RESIZE);
  PyObject *child_dict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in resize");
    return NULL;
  }

  node->autosize << false, false, false;
  if (newsize != NULL) {
    double x, y, z;
    if (python_vectorval(newsize, 3, 3, &x, &y, &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid resize dimensions");
      return NULL;
    }
    node->newsize[0] = x;
    node->newsize[1] = y;
    node->newsize[2] = z;
  }

  if (autosize != NULL) {
    double x, y, z;
    if (python_vectorval(newsize, 3, 3, &x, &y, &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid autosize dimensions");
      return NULL;
    }
    node->autosize[0] = x;
    node->autosize[1] = y;
    node->autosize[2] = z;
  }

  node->children.push_back(child);
  node->convexity = convexity;

  auto pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O!O!i", kwlist, &obj, &PyList_Type, &newsize,
                                   &PyList_Type, &autosize, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing resize(object,vec3)");
    return NULL;
  }
  return python_resize_core(obj, newsize, autosize, convexity);
}

PyObject *python_oo_resize(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"newsize", "auto", "convexity", NULL};
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;
  int convexity = 2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!O!i", kwlist, &PyList_Type, &newsize, &PyList_Type,
                                   &autosize, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing resize(object,vec3)");
    return NULL;
  }
  return python_resize_core(obj, newsize, autosize, convexity);
}
