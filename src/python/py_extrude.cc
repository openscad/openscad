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

#include "linalg.h"
#include "GeometryUtils.h"
#include "genlang/genlang.h"
#include <Python.h>
#include "pyopenscad.h"
#include "pyfunctions.h"
#include "pyconversion.h"
#include <primitives.h>
#include <LinearExtrudeNode.h>
#include <RotateExtrudeNode.h>
#include <PathExtrudeNode.h>
#include <SkinNode.h>

// #include "GeometryEvaluator.h"
// #include "TransformNode.h"
// #include "utils/degree_trig.h"

PyObject *rotate_extrude_core(PyObject *obj, int convexity, double scale, double angle, PyObject *twist,
                              PyObject *origin, PyObject *offset, PyObject *vp, char *method,
                              CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RotateExtrudeNode>(instance, discretizer);
  PyTypeObject *type = &PyOpenSCADType;
  node->profile_func = NULL;
  node->twist_func = NULL;
  if (obj->ob_type == &PyFunction_Type) {
    Py_XINCREF(obj);  // TODO there to decref it ?
    node->profile_func = obj;
    auto dummy_node = std::make_shared<SquareNode>(instance);
    node->children.push_back(dummy_node);
  } else {
    PyObject *dummydict;
    type = PyOpenSCADObjectType(obj);
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in rotate_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }

  node->convexity = convexity;
  node->scale = scale;
  node->angle = angle;
  if (twist != NULL) {
    if (twist->ob_type == &PyFunction_Type) {
      Py_XINCREF(twist);  // TODO there to decref it ?
      node->twist_func = twist;
    } else node->twist = PyFloat_AsDouble(twist);
  }

  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }
  if (offset != NULL && PyList_Check(offset) && PyList_Size(offset) == 2) {
    node->offset_x = PyFloat_AsDouble(PyList_GetItem(offset, 0));
    node->offset_y = PyFloat_AsDouble(PyList_GetItem(offset, 1));
  }
  double dummy;
  Vector3d v(0, 0, 0);
  if (vp != nullptr && !python_vectorval(vp, 3, 3, &v[0], &v[1], &v[2], &dummy)) {
  }
  node->v = v;
  if (method != nullptr) node->method = method;
  else node->method = "centered";

  if (node->convexity <= 0) node->convexity = 2;
  if (node->scale <= 0) node->scale = 1;
  if (node->angle <= -360) node->angle = 360;

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist = NULL;
  PyObject *v = NULL;
  char *method = NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  char *kwlist[] = {"obj",    "convexity", "scale", "angle",  "twist",
                    "origin", "offset",    "v",     "method", NULL};
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iddOOOOsddd", kwlist, &obj, &convexity, &scale,
                                   &angle, &twist, &origin, &offset, &v, &method, &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate_extrude(object,...)");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method,
                             std::move(discretizer));
}

PyObject *python_oo_rotate_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist = NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  PyObject *v = NULL;
  char *method = NULL;
  char *kwlist[] = {"convexity", "scale", "angle", "twist", "origin", "offset", "v", "method", NULL};
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iddOOOOs", kwlist, &convexity, &scale, &angle, &twist,
                                   &origin, &offset, &v, &method)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method,
                             std::move(discretizer));
}

PyObject *linear_extrude_core(PyObject *obj, PyObject *height, int convexity, PyObject *origin,
                              PyObject *scale, PyObject *center, int slices, int segments,
                              PyObject *twist, CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<LinearExtrudeNode>(instance, discretizer);
  PyTypeObject *type = &PyOpenSCADType;
  node->profile_func = NULL;
  node->twist_func = NULL;
  if (obj->ob_type == &PyFunction_Type) {
    Py_XINCREF(obj);  // TODO there to decref it ?
    node->profile_func = obj;
    auto dummy_node = std::make_shared<SquareNode>(instance);
    node->children.push_back(dummy_node);
  } else {
    PyObject *dummydict;
    type = PyOpenSCADObjectType(obj);
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in linear_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }

  Vector3d height_vec(0, 0, 0);
  double dummy;
  if (!python_numberval(height, &height_vec[2], nullptr, 0)) {
    node->height = height_vec;
  } else if (!python_vectorval(height, 3, 3, &height_vec[0], &height_vec[1], &height_vec[2], &dummy)) {
    node->height = height_vec;
    node->has_heightvector = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "Height must be either a number or a vector\n");
    return NULL;
  }

  node->convexity = convexity;

  node->origin_x = 0.0;
  node->origin_y = 0.0;
  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }

  node->scale_x = 1.0;
  node->scale_y = 1.0;
  if (scale != NULL && PyList_Check(scale) && PyList_Size(scale) == 2) {
    node->scale_x = PyFloat_AsDouble(PyList_GetItem(scale, 0));
    node->scale_y = PyFloat_AsDouble(PyList_GetItem(scale, 1));
  }

  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }

  node->slices = slices;
  node->has_slices = slices != 1 ? 1 : 0;

  node->segments = segments;
  node->has_segments = segments != 1 ? 1 : 0;

  if (twist != NULL) {
    if (twist->ob_type == &PyFunction_Type) {
      Py_XINCREF(twist);  // TODO there to decref it ?
      node->twist_func = twist;
    } else node->twist = PyFloat_AsDouble(twist);
    node->has_twist = 1;
  } else node->has_twist = 0;
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  PyObject *height = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  PyObject *twist = NULL;

  char *kwlist[] = {"obj",    "height", "convexity", "origin", "scale",
                    "center", "slices", "segments",  "twist",  NULL};
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OiOOOiiO", kwlist, &obj, &height, &convexity,
                                   &origin, &scale, &center, &slices, &segments, &twist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj, height, convexity, origin, scale, center, slices, segments, twist,
                             std::move(discretizer));
}

PyObject *python_oo_linear_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  PyObject *height = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  PyObject *twist = NULL;

  char *kwlist[] = {"height", "convexity", "origin", "scale", "center",
                    "slices", "segments",  "twist",  NULL};
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OiOOOiiO", kwlist, &height, &convexity, &origin,
                                   &scale, &center, &slices, &segments, &twist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj, height, convexity, origin, scale, center, slices, segments, twist,
                             std::move(discretizer));
}

PyObject *path_extrude_core(PyObject *obj, PyObject *path, PyObject *xdir, int convexity,
                            PyObject *origin, PyObject *scale, PyObject *twist, PyObject *closed,
                            PyObject *allow_intersect, double fn, double fa, double fs)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<PathExtrudeNode>(instance);
  PyTypeObject *type = &PyOpenSCADType;
  node->profile_func = NULL;
  node->twist_func = NULL;
  if (obj->ob_type == &PyFunction_Type) {
    Py_XINCREF(obj);  // TODO there to decref it ?
    node->profile_func = obj;
    auto dummy_node = std::make_shared<SquareNode>(instance);
    node->children.push_back(dummy_node);
  } else {
    PyObject *dummydict;
    type = PyOpenSCADObjectType(obj);
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in path_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }
  if (path != NULL && PyList_Check(path)) {
    int n = PyList_Size(path);
    for (int i = 0; i < n; i++) {
      PyObject *point = PyList_GetItem(path, i);
      double x, y, z, w = 0;
      if (python_vectorval(point, 3, 4, &x, &y, &z, &w)) {
        PyErr_SetString(PyExc_TypeError, "Cannot parse vector in path_extrude path\n");
        return NULL;
      }
      Vector4d pt3d(x, y, z, w);
      if (i > 0 && node->path[i - 1] == pt3d) continue;  //  prevent double pts
      node->path.push_back(pt3d);
    }
  }
  node->xdir_x = 1;
  node->xdir_y = 0;
  node->xdir_z = 0;
  node->closed = false;
  if (closed == Py_True) node->closed = true;
  if (allow_intersect == Py_True) node->allow_intersect = true;
  if (xdir != NULL) {
    if (python_vectorval(xdir, 3, 3, &(node->xdir_x), &(node->xdir_y), &(node->xdir_z))) {
      PyErr_SetString(PyExc_TypeError, "error in path_extrude xdir parameter\n");
      return NULL;
    }
  }
  if (fabs(node->xdir_x) < 0.001 && fabs(node->xdir_y) < 0.001 && fabs(node->xdir_z) < 0.001) {
    PyErr_SetString(PyExc_TypeError, "error in path_extrude xdir parameter has zero size\n");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (fn != -1) node->fn = fn;
  if (fa != -1) node->fa = fa;
  if (fs != -1) node->fs = fs;

  node->convexity = convexity;

  node->origin_x = 0.0;
  node->origin_y = 0.0;
  if (origin != NULL) {
    double dummy;
    if (python_vectorval(origin, 2, 2, &(node->origin_x), &(node->origin_y), &dummy)) {
      PyErr_SetString(PyExc_TypeError, "error in path_extrude origin parameter\n");
      return NULL;
    }
  }

  node->scale_x = 1.0;
  node->scale_y = 1.0;
  if (scale != NULL) {
    double dummy;
    if (python_vectorval(scale, 2, 2, &(node->scale_x), &(node->scale_y), &dummy)) {
      PyErr_SetString(PyExc_TypeError, "error in path_extrude scale parameter\n");
      return NULL;
    }
  }

  if (scale != NULL && PyList_Check(scale) && PyList_Size(scale) == 2) {
    node->scale_x = PyFloat_AsDouble(PyList_GetItem(scale, 0));
    node->scale_y = PyFloat_AsDouble(PyList_GetItem(scale, 1));
  }
  if (twist != NULL) {
    if (twist->ob_type == &PyFunction_Type) {
      Py_XINCREF(twist);  // TODO there to decref it ?
      node->twist_func = twist;
    } else node->twist = PyFloat_AsDouble(twist);
    node->has_twist = 1;
  } else node->has_twist = 0;

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_path_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *path = NULL;
  PyObject *xdir = NULL;
  PyObject *closed = NULL;
  PyObject *allow_intersect = NULL;
  PyObject *twist = NULL;
  double fn = -1, fa = -1, fs = -1;

  char *kwlist[] = {"obj",   "path",   "xdir", "convexity", "origin", "scale",
                    "twist", "closed", "fn",   "fa",        "fs",     NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!|O!iOOOOOddd", kwlist, &obj, &PyList_Type, &path,
                                   &PyList_Type, &xdir, &convexity, &origin, &scale, &twist, &closed,
                                   &allow_intersect, &fn, &fs, &fs)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return path_extrude_core(obj, path, xdir, convexity, origin, scale, twist, closed, allow_intersect, fn,
                           fa, fs);
}

PyObject *python_concat(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::CONCAT);
  PyObject *obj;
  PyObject *obj1;
  PyObject *child_dict = nullptr;
  PyTypeObject *type = &PyOpenSCADType;
  // dont do union in any circumstance
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    if (PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
      type = PyOpenSCADObjectType(obj);
      node->children.push_back(((PyOpenSCADObject *)obj)->node);
    } else if (PyList_Check(obj)) {
      for (int j = 0; j < PyList_Size(obj); j++) {
        obj1 = PyList_GetItem(obj, j);
        if (PyObject_IsInstance(obj1, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
          type = PyOpenSCADObjectType(obj1);
          node->children.push_back(((PyOpenSCADObject *)obj1)->node);
        } else {
          PyErr_SetString(PyExc_TypeError, "Error during concat. arguments must be solids");
          return nullptr;
        }
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Error during concat. arguments must be solids");
      return nullptr;
    }
  }

  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_skin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  int i;

  auto node = std::make_shared<SkinNode>(instance);
  PyTypeObject *type = &PyOpenSCADType;
  PyObject *obj;
  PyObject *child_dict = nullptr;
  PyObject *dummy_dict = nullptr;
  std::shared_ptr<AbstractNode> child;
  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str = PyBytes_AS_STRING(value1);
      double tmp;
      if (value_str == nullptr) {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
        return nullptr;
      } else if (strcmp(value_str, "convexity") == 0) {
        python_numberval(value, &tmp, nullptr, 0);
        node->convexity = (int)tmp;
      } else if (strcmp(value_str, "align_angle") == 0) {
        python_numberval(value, &tmp, nullptr, 0);
        node->align_angle = tmp;
        node->has_align_angle = true;
      } else if (strcmp(value_str, "segments") == 0) {
        python_numberval(value, &tmp, nullptr, 0);
        node->has_segments = true;
        node->segments = (int)tmp;
      } else if (strcmp(value_str, "interpolate") == 0) {
        python_numberval(value, &tmp, nullptr, 0);
        node->has_interpolate = true;
        node->interpolate = tmp;
      } else {
        PyErr_SetString(PyExc_TypeError, "Unkown parameter name in skin.");
        return nullptr;
      }
    }
  }
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    if (i == 0) {
      type = PyOpenSCADObjectType(obj);
      child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
    } else child = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    if (child != NULL) {
      node->children.push_back(child);
    } else {
      PyErr_SetString(PyExc_TypeError, "Error during skin. arguments must be solids or arrays.");
      return nullptr;
    }
  }

  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_oo_path_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *path = NULL;
  PyObject *xdir = NULL;
  PyObject *closed = NULL;
  PyObject *allow_intersect = NULL;
  PyObject *twist = NULL;
  double fn = -1, fa = -1, fs = -1;

  char *kwlist[] = {"path",   "xdir", "convexity", "origin", "scale", "twist",
                    "closed", "fn",   "fa",        "fs",     NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!iOOOOOddd", kwlist, &PyList_Type, &path,
                                   &PyList_Type, &xdir, &convexity, &origin, &scale, &twist, &closed,
                                   &allow_intersect, &fn, &fs, &fs)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return path_extrude_core(obj, path, xdir, convexity, origin, scale, twist, closed, allow_intersect, fn,
                           fa, fs);
}
