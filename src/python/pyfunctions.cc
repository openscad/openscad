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

#include <Python.h>
#include "python/pyopenscad.h"
#include "core/primitives.h"
#include "core/CsgOpNode.h"
#include "core/ColorNode.h"
#include "core/ColorUtil.h"
#include "core/FreetypeRenderer.h"
#include "core/TransformNode.h"
#include "core/LinearExtrudeNode.h"
#include "core/RotateExtrudeNode.h"
#include "core/CgalAdvNode.h"
#include "core/RoofNode.h"
#include "core/RenderNode.h"
#include "core/SurfaceNode.h"
#include "core/TextNode.h"
#include "core/CurveDiscretizer.h"
#include "core/OffsetNode.h"
#include "core/ProjectionNode.h"
#include "core/Tree.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "geometry/GeometryEvaluator.h"
#include "utils/degree_trig.h"
#include "io/fileutils.h"
#include "handle_dep.h"

PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<CubeNode>(instance);

  char *kwlist[] = {"size", "center", NULL};
  PyObject *size = NULL;

  PyObject *center = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &size, &center)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing cube(size)");
    return NULL;
  }

  if (size != NULL) {
    if (python_vectorval(size, 3, 3, &(node->x), &(node->y), &(node->z))) {
      PyErr_SetString(PyExc_TypeError, "Invalid Cube dimensions");
      return NULL;
    }
  }
  if (node->x <= 0 || node->y <= 0 || node->z <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cube Dimensions must be positive");
    return NULL;
  }
  node->center = false;
  if (center == Py_False || center == NULL)
    ;
  else if (center == Py_True) {
    for (int i = 0; i < 3; i++) node->center = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_sphere(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();

  char *kwlist[] = {"r", "d", NULL};
  double r = NAN;
  PyObject *rp = nullptr;
  double d = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Od", kwlist, &rp, &d)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing sphere(r|d)");
    return NULL;
  }
  if (rp != nullptr) {
    python_numberval(rp, &r);
  }
  if (!isnan(r)) {
    if (r <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter r must be positive");
      return NULL;
    }
    vr = r;
    if (!isnan(d)) {
      PyErr_SetString(PyExc_TypeError, "Cant specify r and d at the same time for sphere");
      return NULL;
    }
  }
  if (!isnan(d)) {
    if (d <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter d must be positive");
      return NULL;
    }
    vr = d / 2.0;
  }

  auto node = std::make_shared<SphereNode>(instance, CreateCurveDiscretizer(kwargs));

  node->r = vr;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();

  char *kwlist[] = {"h", "r1", "r2", "center", "r", "d", "d1", "d2", NULL};
  double h = NAN;
  double r = NAN;
  double r1 = NAN;
  double r2 = NAN;
  double d = NAN;
  double d1 = NAN;
  double d2 = NAN;

  PyObject *center = NULL;
  double vr1 = 1, vr2 = 1, vh = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dddOdddd", kwlist, &h, &r1, &r2, &center, &r, &d, &d1,
                                   &d2)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing cylinder(h,r|r1+r2|d1+d2)");
    return NULL;
  }

  if (h <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder height must be positive");
    return NULL;
  }
  vh = h;

  if (!isnan(d) && d <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d must be positive");
    return NULL;
  }
  if (!isnan(r1) && r1 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r1 must not be negative");
    return NULL;
  }
  if (!isnan(r2) && r2 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r2 must not be negative");
    return NULL;
  }
  if (!isnan(d1) && d1 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d1 must not be negative");
    return NULL;
  }
  if (!isnan(d2) && d2 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d2 must not be negative");
    return NULL;
  }

  if (!isnan(r1) && !isnan(r2)) {
    vr1 = r1;
    vr2 = r2;
  } else if (!isnan(r1) && isnan(r2)) {
    vr1 = r1;
    vr2 = r1;
  } else if (!isnan(d1) && !isnan(d2)) {
    vr1 = d1 / 2.0;
    vr2 = d2 / 2.0;
  } else if (!isnan(r)) {
    vr1 = r;
    vr2 = r;
  } else if (!isnan(d)) {
    vr1 = d / 2.0;
    vr2 = d / 2.0;
  }

  auto node = std::make_shared<CylinderNode>(instance, CreateCurveDiscretizer(kwargs));

  node->r1 = vr1;
  node->r2 = vr2;
  node->h = vh;

  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  unsigned int i, j, pointIndex;
  auto node = std::make_shared<PolyhedronNode>(instance);

  char *kwlist[] = {"points", "faces", "convexity", "triangles", NULL};
  PyObject *points = NULL;
  PyObject *faces = NULL;
  int convexity = 2;
  PyObject *triangles = NULL;

  PyObject *element;
  Vector3d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|iO!", kwlist, &PyList_Type, &points, &PyList_Type,
                                   &faces, &convexity, &PyList_Type, &triangles)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing polyhedron(points, faces)");
    return NULL;
  }

  if (points != NULL && PyList_Check(points)) {
    if (PyList_Size(points) == 0) {
      PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polyhedron");
      return NULL;
    }
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 3) {
        point[0] = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point[1] = PyFloat_AsDouble(PyList_GetItem(element, 1));
        point[2] = PyFloat_AsDouble(PyList_GetItem(element, 2));
        node->points.push_back(point);
      } else {
        PyErr_SetString(PyExc_TypeError, "Coordinate must exactly contain 3 numbers");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polyhedron Points must be a list of coordinates");
    return NULL;
  }

  if (triangles != NULL) {
    faces = triangles;
    //	LOG(message_group::Deprecated, inst->location(), parameters.documentRoot(),
    //"polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
  }

  if (faces != NULL && PyList_Check(faces)) {
    if (PyList_Size(faces) == 0) {
      PyErr_SetString(PyExc_TypeError, "must specify at least 1 face");
      return NULL;
    }
    for (i = 0; i < PyList_Size(faces); i++) {
      element = PyList_GetItem(faces, i);
      if (PyList_Check(element)) {
        IndexedFace face;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
          if (pointIndex < 0 || pointIndex >= node->points.size()) {
            PyErr_SetString(PyExc_TypeError, "Polyhedron Point Index out of range");
            return NULL;
          }
          face.push_back(pointIndex);
        }
        if (face.size() >= 3) {
          node->faces.push_back(std::move(face));
        } else {
          PyErr_SetString(PyExc_TypeError, "Polyhedron Face must sepcify at least 3 indices");
          return NULL;
        }

      } else {
        PyErr_SetString(PyExc_TypeError, "Polyhedron Face must be a list of indices");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polyhedron faces must be a list of indices");
    return NULL;
  }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_square(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<SquareNode>(instance);

  char *kwlist[] = {"dim", "center", NULL};
  PyObject *dim = NULL;

  PyObject *center = NULL;
  double z = NAN;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &dim, &center)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing square(dim)");
    return NULL;
  }
  if (dim != NULL) {
    if (python_vectorval(dim, 2, 2, &(node->x), &(node->y), &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid Square dimensions");
      return NULL;
    }
  }
  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}
PyObject *python_circle(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();

  char *kwlist[] = {"r", "d", NULL};
  double r = NAN;
  double d = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddd", kwlist, &r, &d)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing circle(r|d)");
    return NULL;
  }

  if (!isnan(r)) {
    if (r <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter r must be positive");
      return NULL;
    }
    vr = r;
    if (!isnan(d)) {
      PyErr_SetString(PyExc_TypeError, "Cant specify r and d at the same time for circle");
      return NULL;
    }
  }
  if (!isnan(d)) {
    if (d <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter d must be positive");
      return NULL;
    }
    vr = d / 2.0;
  }

  auto node = std::make_shared<CircleNode>(instance, CreateCurveDiscretizer(kwargs));

  node->r = vr;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_polygon(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  unsigned int i, j, pointIndex;
  auto node = std::make_shared<PolygonNode>(instance);

  char *kwlist[] = {"points", "paths", "convexity", NULL};
  PyObject *points = NULL;
  PyObject *paths = NULL;
  int convexity = 2;

  PyObject *element;
  Vector2d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!i", kwlist, &PyList_Type, &points, &PyList_Type,
                                   &paths, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing polygon(points,paths)");
    return NULL;
  }

  if (points != NULL && PyList_Check(points)) {
    if (PyList_Size(points) == 0) {
      PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polygon");
      return NULL;
    }
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 2) {
        point[0] = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point[1] = PyFloat_AsDouble(PyList_GetItem(element, 1));
        node->points.push_back(point);
      } else {
        PyErr_SetString(PyExc_TypeError, "Coordinate must exactly contain 2 numbers");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polygon points must be a list of coordinates");
    return NULL;
  }

  if (paths != NULL && PyList_Check(paths)) {
    if (PyList_Size(paths) == 0) {
      PyErr_SetString(PyExc_TypeError, "must specify at least 1 path when specified");
      return NULL;
    }
    for (i = 0; i < PyList_Size(paths); i++) {
      element = PyList_GetItem(paths, i);
      if (PyList_Check(element)) {
        std::vector<size_t> path;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
          if (pointIndex < 0 || pointIndex >= node->points.size()) {
            PyErr_SetString(PyExc_TypeError, "Polyhedron Point Index out of range");
            return NULL;
          }
          path.push_back(pointIndex);
        }
        node->paths.push_back(std::move(path));
      } else {
        PyErr_SetString(PyExc_TypeError, "Polygon path must be a list of indices");
        return NULL;
      }
    }
  }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

int python_tomatrix(PyObject *pyt, Matrix4d& mat)
{
  if (pyt == nullptr) return 1;
  PyObject *row, *cell;
  double val;
  if (!PyList_Check(pyt)) return 1;  // TODO crash wenn pyt eine funktion ist
  if (PyList_Size(pyt) != 4) return 1;
  for (int i = 0; i < 4; i++) {
    row = PyList_GetItem(pyt, i);
    if (!PyList_Check(row)) return 1;
    if (PyList_Size(row) != 4) return 1;
    for (int j = 0; j < 4; j++) {
      cell = PyList_GetItem(row, j);
      if (python_numberval(cell, &val)) return 1;
      mat(i, j) = val;
    }
  }
  return 0;
}
PyObject *python_frommatrix(const Matrix4d& mat)
{
  PyObject *pyo = PyList_New(4);
  PyObject *row;
  for (int i = 0; i < 4; i++) {
    row = PyList_New(4);
    for (int j = 0; j < 4; j++) PyList_SetItem(row, j, PyFloat_FromDouble(mat(i, j)));
    PyList_SetItem(pyo, i, row);
  }
  return pyo;
}

PyObject *python_matrix_scale(PyObject *mat, Vector3d scalevec)
{
  Transform3d matrix = Transform3d::Identity();
  matrix.scale(scalevec);
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  Vector3d n;
  for (int i = 0; i < 3; i++) {
    n = Vector3d(raw(0, i), raw(1, i), raw(2, i));  // TODO fix
    n = matrix * n;
    for (int j = 0; j < 3; j++) raw(j, i) = n[j];
  }
  return python_frommatrix(raw);
}

PyObject *python_scale_sub(PyObject *obj, Vector3d scalevec)
{
  PyObject *mat = python_matrix_scale(obj, scalevec);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<TransformNode>(instance, "scale");
  PyObject *child_dict;
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in scale");
    return NULL;
  }
  node->matrix.scale(scalevec);
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_matrix_scale(value, scalevec);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_scale_core(PyObject *obj, PyObject *val_v)
{
  double x = 1, y = 1, z = 1;
  if (python_vectorval(val_v, 2, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in scale, use 1 to 3 ordinates.");
    return NULL;
  }
  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0]) ||
        !std::isfinite(scalevec[1]) || !std::isfinite(scalevec[2])) {
      //      LOG(message_group::Warning, instance->location(), parameters.documentRoot(), "scale(%1$s)",
      //      parameters["v"].toEchoStringNoThrow());
    }
  }

  return python_scale_sub(obj, scalevec);
}

PyObject *python_scale(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj, val_v);
}

PyObject *python_oo_scale(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj, val_v);
}
PyObject *python_matrix_rot(PyObject *mat, Matrix3d rotvec)
{
  Transform3d matrix = Transform3d::Identity();
  matrix.rotate(rotvec);
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  Vector3d n;
  for (int i = 0; i < 4; i++) {
    n = Vector3d(raw(0, i), raw(1, i), raw(2, i));
    n = matrix * n;
    for (int j = 0; j < 3; j++) raw(j, i) = n[j];
  }
  return python_frommatrix(raw);
}

PyObject *python_rotate_sub(PyObject *obj, Vector3d vec3, double angle)
{
  Matrix3d M;
  if (isnan(angle)) {
    double sx = 0, sy = 0, sz = 0;
    double cx = 1, cy = 1, cz = 1;
    double a = 0.0;
    if (vec3[2] != 0) {
      a = vec3[2];
      sz = sin_degrees(a);
      cz = cos_degrees(a);
    }
    if (vec3[1] != 0) {
      a = vec3[1];
      sy = sin_degrees(a);
      cy = cos_degrees(a);
    }
    if (vec3[0] != 0) {
      a = vec3[0];
      sx = sin_degrees(a);
      cx = cos_degrees(a);
    }
    // clang-format off
    M << cy * cz, cz * sx * sy - cx * sz, cx * cz * sy + sx * sz,
         cy * sz, cx * cz + sx * sy * sz, -cz * sx + cx * sy * sz,
         -sy,     cy * sx,                cx * cy;
    // clang-format on
  } else {
    M = angle_axis_degrees(angle, vec3);
  }
  PyObject *mat = python_matrix_rot(obj, M);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "rotate");

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in rotate");
    return NULL;
  }
  node->matrix.rotate(M);

  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_matrix_rot(value, M);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, PyObject *val_v)
{
  Vector3d vec3(0, 0, 0);
  double angle;
  if (val_a != nullptr && PyList_Check(val_a) && val_v == nullptr) {
    if (PyList_Size(val_a) >= 1) vec3[0] = PyFloat_AsDouble(PyList_GetItem(val_a, 0));
    if (PyList_Size(val_a) >= 2) vec3[1] = PyFloat_AsDouble(PyList_GetItem(val_a, 1));
    if (PyList_Size(val_a) >= 3) vec3[2] = PyFloat_AsDouble(PyList_GetItem(val_a, 2));
    return python_rotate_sub(obj, vec3, NAN);
  } else if (val_a != nullptr && val_v != nullptr && !python_numberval(val_a, &angle) &&
             PyList_Check(val_v) && PyList_Size(val_v) == 3) {
    vec3[0] = PyFloat_AsDouble(PyList_GetItem(val_v, 0));
    vec3[1] = PyFloat_AsDouble(PyList_GetItem(val_v, 1));
    vec3[2] = PyFloat_AsDouble(PyList_GetItem(val_v, 2));
    return python_rotate_sub(obj, vec3, angle);
  }
  PyErr_SetString(PyExc_TypeError, "Invalid arguments to rotate()");
  return nullptr;
}

PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "a", "v", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  PyObject *obj = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O", kwlist, &obj, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_oo_rotate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"a", "v", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_matrix_mirror(PyObject *mat, Matrix4d m)
{
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  Vector4d n;
  for (int i = 0; i < 3; i++) {
    n = Vector4d(raw(0, i), raw(1, i), raw(2, i), 0);
    n = m * n;
    for (int j = 0; j < 3; j++) raw(j, i) = n[j];
  }
  return python_frommatrix(raw);
}

PyObject *python_mirror_sub(PyObject *obj, Matrix4d& m)
{
  PyObject *mat = python_matrix_mirror(obj, m);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "mirror");
  node->matrix = m;
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in mirror");
    return NULL;
  }
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_matrix_mirror(value, m);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_mirror_core(PyObject *obj, PyObject *val_v)
{
  Vector3d mirrorvec;
  double x = 1.0, y = 0.0, z = 0.0;
  if (python_vectorval(val_v, 2, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in mirror");
    return NULL;
  }
  // x /= sqrt(x*x + y*y + z*z)
  // y /= sqrt(x*x + y*y + z*z)
  // z /= sqrt(x*x + y*y + z*z)
  Matrix4d m = Matrix4d::Identity();
  if (x != 0.0 || y != 0.0 || z != 0.0) {
    // skip using sqrt to normalize the vector since each element of matrix contributes it with two
    // multiplied terms instead just divide directly within each matrix element simplified calculation
    // leads to less float errors
    double a = x * x + y * y + z * z;

    m << 1 - 2 * x * x / a, -2 * y * x / a, -2 * z * x / a, 0, -2 * x * y / a, 1 - 2 * y * y / a,
      -2 * z * y / a, 0, -2 * x * z / a, -2 * y * z / a, 1 - 2 * z * z / a, 0, 0, 0, 0, 1;
  }
  return python_mirror_sub(obj, m);
}

PyObject *python_mirror(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};

  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_oo_mirror(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};

  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_matrix_trans(PyObject *mat, Vector3d transvec)
{
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  for (int i = 0; i < 3; i++) raw(i, 3) += transvec[i];
  return python_frommatrix(raw);
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode);
PyObject *python_translate_core(PyObject *obj, PyObject *v)
{
  if (v == nullptr) return obj;
  return python_nb_sub_vec3(obj, v, 0);
}

PyObject *python_translate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_translate_core(obj, v);
}

PyObject *python_oo_translate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_translate_core(obj, v);
}

PyObject *python_multmatrix_sub(PyObject *pyobj, PyObject *pymat, int div)
{
  Matrix4d mat;
  if (!python_tomatrix(pymat, mat)) {
    double w = mat(3, 3);
    if (w != 1.0) mat = mat / w;
  } else {
    PyErr_SetString(PyExc_TypeError, "Matrix vector should be 4x4 array");
    return NULL;
  }
  if (div) {
    auto tmp = mat.inverse().eval();
    mat = tmp;
  }

  Matrix4d objmat;
  if (!python_tomatrix(pyobj, objmat)) {
    objmat = mat * objmat;
    return python_frommatrix(objmat);
  }

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "multmatrix");
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict;
  child = PyOpenSCADObjectToNodeMulti(pyobj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in multmatrix");
    return NULL;
  }

  node->matrix = mat;
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      Matrix4d raw;
      if (python_tomatrix(value, raw)) return nullptr;
      PyObject *value1 = python_frommatrix(node->matrix * raw);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist, &obj, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 0);
}

PyObject *python_oo_multmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 0);
}

PyObject *python_divmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist, &obj, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 1);
}

PyObject *python_oo_divmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 1);
}

PyObject *python_show_core(PyObject *obj)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in show");
    return NULL;
  }
  python_result_node = child;
  return Py_None;
}

PyObject *python_show(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *kwlist[] = {"obj", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_show_core(obj);
}

PyObject *python_oo_show(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_show_core(obj);
}

PyObject *python__getitem__(PyObject *obj, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *)obj;
  if (self->dict == nullptr) {
    return nullptr;
  }
  PyObject *result = PyDict_GetItem(self->dict, key);
  if (result == NULL) {
    PyObject *dummy_dict;
    std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    PyObject *keyname = PyUnicode_AsEncodedString(key, "utf-8", "~");
    std::string keystr = PyBytes_AS_STRING(keyname);
    result = Py_None;
  } else Py_INCREF(result);
  return result;
}

int python__setitem__(PyObject *dict, PyObject *key, PyObject *v)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *)dict;
  if (self->dict == NULL) {
    return 0;
  }
  Py_INCREF(v);
  PyDict_SetItem(self->dict, key, v);
  return 0;
}

PyObject *python_color_core(PyObject *obj, PyObject *color, double alpha)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in color");
    return NULL;
  }
  DECLARE_INSTANCE();
  auto node = std::make_shared<ColorNode>(instance);

  Vector4d col(0, 0, 0, alpha);
  if (!python_vectorval(color, 3, 4, &col[0], &col[1], &col[2], &col[3])) {
    node->color.setRgba(float(col[0]), float(col[1]), float(col[2]), float(col[3]));
  } else if (PyUnicode_Check(color)) {
    PyObject *value = PyUnicode_AsEncodedString(color, "utf-8", "~");
    char *colorname = PyBytes_AS_STRING(value);
    const auto color = OpenSCAD::parse_color(colorname);
    if (color) {
      node->color = *color;
      node->color.setAlpha(alpha);
    } else {
      PyErr_SetString(PyExc_TypeError, "Cannot parse color");
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown color representation");
    return nullptr;
  }

  node->children.push_back(child);

  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_color(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "c", "alpha", NULL};
  PyObject *obj = NULL;
  PyObject *color = NULL;
  double alpha = 1.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Od", kwlist, &obj, &color, &alpha)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha);
}

PyObject *python_oo_color(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"c", "alpha", NULL};
  PyObject *color = NULL;
  double alpha = 1.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Od", kwlist, &color, &alpha)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha);
}

PyObject *python_mesh_core(PyObject *obj, bool tessellate)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in mesh \n");
    return NULL;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);

  if (ps != nullptr) {
    if (tessellate == true) {
      ps = PolySetUtils::tessellate_faces(*ps);
    }
    // Now create Python Point array
    PyObject *ptarr = PyList_New(ps->vertices.size());
    for (unsigned int i = 0; i < ps->vertices.size(); i++) {
      PyObject *coord = PyList_New(3);
      for (int j = 0; j < 3; j++) PyList_SetItem(coord, j, PyFloat_FromDouble(ps->vertices[i][j]));
      PyList_SetItem(ptarr, i, coord);
      Py_XINCREF(coord);
    }
    Py_XINCREF(ptarr);
    // Now create Python Point array
    PyObject *polarr = PyList_New(ps->indices.size());
    for (unsigned int i = 0; i < ps->indices.size(); i++) {
      PyObject *face = PyList_New(ps->indices[i].size());
      for (unsigned int j = 0; j < ps->indices[i].size(); j++)
        PyList_SetItem(face, j, PyLong_FromLong(ps->indices[i][j]));
      PyList_SetItem(polarr, i, face);
      Py_XINCREF(face);
    }
    Py_XINCREF(polarr);

    PyObject *result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, ptarr);
    PyTuple_SetItem(result, 1, polarr);

    return result;
  }
  if (auto polygon2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    const std::vector<Outline2d> outlines = polygon2d->outlines();
    PyObject *pyth_outlines = PyList_New(outlines.size());
    for (unsigned int i = 0; i < outlines.size(); i++) {
      const Outline2d& outline = outlines[i];
      PyObject *pyth_outline = PyList_New(outline.vertices.size());
      for (unsigned int j = 0; j < outline.vertices.size(); j++) {
        Vector2d pt = outline.vertices[j];
        PyObject *pyth_pt = PyList_New(2);
        for (int k = 0; k < 2; k++) PyList_SetItem(pyth_pt, k, PyFloat_FromDouble(pt[k]));
        PyList_SetItem(pyth_outline, j, pyth_pt);
      }
      PyList_SetItem(pyth_outlines, i, pyth_outline);
    }
    return pyth_outlines;
  }
  return Py_None;
}

PyObject *python_mesh(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "triangulate", NULL};
  PyObject *obj = NULL;
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True);
}

PyObject *python_oo_mesh(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"triangulate", NULL};
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True);
}

PyObject *rotate_extrude_core(PyObject *obj, int convexity, double scale, double angle, PyObject *twist,
                              PyObject *origin, PyObject *offset, PyObject *vp, char *method,
                              CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RotateExtrudeNode>(instance, discretizer);
  if (1) {
    PyObject *dummydict;
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in rotate_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }

  node->convexity = convexity;
  node->angle = angle;

  double dummy;
  Vector3d v(0, 0, 0);
  if (vp != nullptr && !python_vectorval(vp, 3, 3, &v[0], &v[1], &v[2], &dummy)) {
  }

  if (node->convexity <= 0) node->convexity = 2;
  if (node->angle <= -360) node->angle = 360;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
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
  char *kwlist[] = {"obj",    "convexity", "scale", "angle",  "twist",
                    "origin", "offset",    "v",     "method", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iddOOOOs", kwlist, &obj, &convexity, &scale, &angle,
                                   &twist, &origin, &offset, &v, &method)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate_extrude(object,...)");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method,
                             CreateCurveDiscretizer(kwargs));
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
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iddOOOOs", kwlist, &convexity, &scale, &angle, &twist,
                                   &origin, &offset, &v, &method)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method,
                             CreateCurveDiscretizer(kwargs));
}

PyObject *linear_extrude_core(PyObject *obj, PyObject *height, int convexity, PyObject *origin,
                              PyObject *scale, PyObject *center, int slices, int segments,
                              PyObject *twist, CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<LinearExtrudeNode>(instance, discretizer);

  if (1) {
    PyObject *dummydict;
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in linear_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }

  Vector3d height_vec(0, 0, 0);
  double dummy;
  if (!python_numberval(height, &height_vec[2])) {
    node->height = height_vec;
  } else if (!python_vectorval(height, 3, 3, &height_vec[0], &height_vec[1], &height_vec[2], &dummy)) {
    node->height = height_vec;
  }

  node->convexity = convexity;

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
    node->twist = PyFloat_AsDouble(twist);
    node->has_twist = 1;
  } else node->has_twist = 0;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
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
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OiOOOiiO", kwlist, &obj, &height, &convexity,
                                   &origin, &scale, &center, &slices, &segments, &twist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj, height, convexity, origin, scale, center, slices, segments, twist,
                             CreateCurveDiscretizer(kwargs));
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
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OiOOOiiO", kwlist, &height, &convexity, &origin,
                                   &scale, &center, &slices, &segments, &twist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj, height, convexity, origin, scale, center, slices, segments, twist,
                             CreateCurveDiscretizer(kwargs));
}

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE();
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  PyObject *obj;
  std::vector<PyObject *> child_dict;
  std::shared_ptr<AbstractNode> child;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    PyObject *dict = nullptr;
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    if (dict != nullptr) {
      child_dict.push_back(dict);
    }
    if (child != NULL) {
      node->children.push_back(child);
    } else {
      switch (mode) {
      case OpenSCADOperator::UNION:
        PyErr_SetString(PyExc_TypeError,
                        "Error during parsing union. arguments must be solids or arrays.");
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
      }
      return NULL;
    }
  }

  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  for (int i = child_dict.size() - 1; i >= 0; i--)  // merge from back  to give 1st child most priority
  {
    auto& dict = child_dict[i];
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

  PyObject *obj;
  PyObject *child_dict;
  PyObject *dummy_dict;
  std::shared_ptr<AbstractNode> child;

  child = PyOpenSCADObjectToNodeMulti(self, &child_dict);
  if (child != NULL) node->children.push_back(child);

  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    if (i == 0) child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
    else child = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    if (child != NULL) {
      node->children.push_back(child);
    } else {
      switch (mode) {
      case OpenSCADOperator::UNION:
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
      }
      return NULL;
    }
  }

  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
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
  std::shared_ptr<AbstractNode> child[2];
  PyObject *child_dict[2];

  if (arg1 == Py_None && mode == OpenSCADOperator::UNION) return arg2;
  if (arg2 == Py_None && mode == OpenSCADOperator::UNION) return arg1;
  if (arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) return arg1;

  child[0] = PyOpenSCADObjectToNodeMulti(arg1, &child_dict[0]);
  if (child[0] == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
    return NULL;
  }
  child[1] = PyOpenSCADObjectToNodeMulti(arg2, &child_dict[1]);
  if (child[1] == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
    return NULL;
  }
  auto node = std::make_shared<CsgOpNode>(instance, mode);
  node->children.push_back(child[0]);
  node->children.push_back(child[1]);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  for (int i = 1; i >= 0; i--) {
    if (child_dict[i] != nullptr) {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(child_dict[i], &pos, &key, &value)) {
        PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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

  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict);
  std::vector<Vector3d> vecs;

  vecs = python_vectors(arg2, 2, 3);

  if (mode == 0 && vecs.size() == 1) {
    PyObject *mat = python_matrix_trans(arg1, vecs[0]);
    if (mat != nullptr) return mat;
  }

  if (vecs.size() > 0) {
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    std::vector<std::shared_ptr<TransformNode>> nodes;
    for (size_t j = 0; j < vecs.size(); j++) {
      auto node = std::make_shared<TransformNode>(instance, "transform");
      if (mode == 0 || mode == 3) node->matrix.translate(vecs[j]);
      if (mode == 1) node->matrix.scale(vecs[j]);
      if (mode == 2) node->matrix.translate(-vecs[j]);
      node->children.push_back(child);
      nodes.push_back(node);
    }
    if (nodes.size() == 1) {
      PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, nodes[0]);
      if (child_dict != nullptr) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(child_dict, &pos, &key, &value)) {
          PyObject *value1 = python_matrix_trans(value, vecs[0]);
          if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
          else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
        }
      }
      return pyresult;
    } else {
      auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
      DECLARE_INSTANCE();
      for (auto x : nodes) node->children.push_back(x->clone());
      return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
    }
  }
  PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
  return NULL;
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2)
{
  return python_nb_sub_vec3(arg1, arg2, 0);
}  // translate
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
    if (!python_numberval(sub, &dmy) || PyList_Check(sub)) {
      return python_nb_sub_vec3(arg1, arg2, 2);
    }
  }
  return python_nb_sub(arg1, arg2, OpenSCADOperator::DIFFERENCE);  // if its solid
}
PyObject *python_nb_and(PyObject *arg1, PyObject *arg2)
{
  return python_nb_sub(arg1, arg2, OpenSCADOperator::INTERSECTION);
}

PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  int i;
  PyObject *dummydict;

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
  PyObject *obj;
  for (i = 0; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
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

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  int convexity = 2;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::MINKOWSKI);
  char *kwlist[] = {"obj", "convexity", NULL};
  PyObject *objs = NULL;
  PyObject *obj;
  PyObject *dummydict;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist, &PyList_Type, &objs, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing minkowski(object)");
    return NULL;
  }
  n = PyList_Size(objs);
  for (i = 0; i < n; i++) {
    obj = PyList_GetItem(objs, i);
    if (Py_TYPE(obj) == &PyOpenSCADType) {
      child = PyOpenSCADObjectToNode(obj, &dummydict);
      node->children.push_back(child);
    } else {
      PyErr_SetString(PyExc_TypeError, "minkowski input data must be shapes");
      return NULL;
    }
  }
  node->convexity = convexity;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
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
  PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in resize");
    return NULL;
  }

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

  /* TODO what is that ?
     const auto& autosize = parameters["auto"];
     node->autosize << false, false, false;
     if (autosize.type() == Value::Type::VECTOR) {
     const auto& va = autosize.toVector();
     if (va.size() >= 1) node->autosize[0] = va[0].toBool();
     if (va.size() >= 2) node->autosize[1] = va[1].toBool();
     if (va.size() >= 3) node->autosize[2] = va[2].toBool();
     } else if (autosize.type() == Value::Type::BOOL) {
     node->autosize << autosize.toBool(), autosize.toBool(), autosize.toBool();
     }
   */

  node->children.push_back(child);
  node->convexity = convexity;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
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

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
PyObject *python_roof_core(PyObject *obj, const char *method, int convexity,
                           CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RoofNode>(instance, discretizer);
  PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in roof");
    return NULL;
  }

  if (method == NULL) {
    node->method = "voronoi";
  } else {
    node->method = method;
    if (!RoofNode::knownMethods.count(node->method)) {
      //      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
      //          "Unknown roof method '" + node->method + "'. Using 'voronoi'.");
      node->method = "voronoi";
    }
  }

  double tmp_convexity = convexity;
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_roof(PyObject *self, PyObject *args, PyObject *kwargs)
{
  double fn = NAN, fa = NAN, fs = NAN;
  char *kwlist[] = {"obj", "method", "convexity", NULL};
  PyObject *obj = NULL;
  const char *method = NULL;
  int convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sd", kwlist, &obj, &method, convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, CreateCurveDiscretizer(kwargs));
}

PyObject *python_oo_roof(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double fn = NAN, fa = NAN, fs = NAN;
  char *kwlist[] = {"method", "convexity", NULL};
  const char *method = NULL;
  int convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sd", kwlist, &method, convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, CreateCurveDiscretizer(kwargs));
}
#endif

PyObject *python_render_core(PyObject *obj, int convexity)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<RenderNode>(instance);

  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &dummydict);
  node->convexity = convexity;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_render(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "convexity", NULL};
  PyObject *obj = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist, &PyOpenSCADType, &obj, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_oo_render(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"convexity", NULL};
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_surface_core(const char *file, PyObject *center, PyObject *invert, int convexity)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<SurfaceNode>(instance);

  std::string fileval = file == NULL ? "" : file;
  std::string filename =
    lookup_file(fileval, instance->location().filePath().parent_path().string(), "");
  node->filename = filename;
  handle_dep(fs::path(filename).generic_string());

  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }
  node->convexity = 2;
  if (invert == Py_True) node->invert = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for invert parameter");
    return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_surface(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"file", "center", "convexity", "invert", NULL};
  const char *file = NULL;
  PyObject *center = NULL;
  PyObject *invert = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OlO", kwlist, &file, &center, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing surface(object)");
    return NULL;
  }

  return python_surface_core(file, center, invert, convexity);
}

std::optional<std::string> to_optional_string(const char *ptr)
{
  if (ptr != nullptr) {
    return std::string(ptr);
  }
  return {};
}

PyObject *python_text(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  char *kwlist[] = {"text",     "size",   "font",   "spacing", "direction",
                    "language", "script", "halign", "valign",  NULL};

  double size = 1.0, spacing = 1.0;

  const char *text = "", *font = NULL, *direction = "ltr", *language = "en", *script = "latin",
             *valign = "baseline", *halign = "left";

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssss", kwlist, &text, &size, &font, &spacing,
                                   &direction, &language, &script, &halign, &valign)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing text(string, ...))");
    return NULL;
  }

  auto node = std::make_shared<TextNode>(
    instance, FreetypeRenderer::Params(FreetypeRenderer::Params::ParamsOptions{
                .curve_discretizer = std::make_shared<CurveDiscretizer>(CreateCurveDiscretizer(kwargs)),
                .size = size,
                .spacing = spacing,
                .text = to_optional_string(text),
                .font = to_optional_string(font),
                .direction = to_optional_string(direction),
                .language = to_optional_string(language),
                .script = to_optional_string(script),
                .halign = to_optional_string(halign),
                .valign = to_optional_string(valign),
                .loc = instance->location(),
              }));

  node->params.detect_properties();

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_textmetrics(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  char *kwlist[] = {"text",     "size",   "font",   "spacing", "direction",
                    "language", "script", "halign", "valign",  NULL};

  double size = 1.0, spacing = 1.0;

  const char *text = "", *font = NULL, *direction = "ltr", *language = "en", *script = "latin",
             *valign = "baseline", *halign = "left";

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssss", kwlist, &text, &size, &font, &spacing,
                                   &direction, &language, &script, &valign, &halign)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing textmetrics");
    return NULL;
  }

  FreetypeRenderer::Params ftparams(FreetypeRenderer::Params::ParamsOptions{
    .curve_discretizer = {},
    .size = size,
    .spacing = spacing,
    .text = to_optional_string(text),
    .font = to_optional_string(font),
    .direction = to_optional_string(direction),
    .language = to_optional_string(language),
    .script = to_optional_string(script),
    .halign = to_optional_string(halign),
    .valign = to_optional_string(valign),
    .loc = instance->location(),
  });

  FreetypeRenderer::TextMetrics metrics(ftparams);
  if (!metrics.ok) {
    PyErr_SetString(PyExc_TypeError, "Invalid Metric");
    return NULL;
  }
  PyObject *offset = PyList_New(2);
  PyList_SetItem(offset, 0, PyFloat_FromDouble(metrics.x_offset));
  PyList_SetItem(offset, 1, PyFloat_FromDouble(metrics.y_offset));

  PyObject *advance = PyList_New(2);
  PyList_SetItem(advance, 0, PyFloat_FromDouble(metrics.advance_x));
  PyList_SetItem(advance, 1, PyFloat_FromDouble(metrics.advance_y));

  PyObject *position = PyList_New(2);
  PyList_SetItem(position, 0, PyFloat_FromDouble(metrics.bbox_x));
  PyList_SetItem(position, 1, PyFloat_FromDouble(metrics.bbox_y));

  PyObject *dims = PyList_New(2);
  PyList_SetItem(dims, 0, PyFloat_FromDouble(metrics.bbox_w));
  PyList_SetItem(dims, 1, PyFloat_FromDouble(metrics.bbox_h));

  PyObject *dict;
  dict = PyDict_New();
  PyDict_SetItemString(dict, "ascent", PyFloat_FromDouble(metrics.ascent));
  PyDict_SetItemString(dict, "descent", PyFloat_FromDouble(metrics.descent));
  PyDict_SetItemString(dict, "offset", offset);
  PyDict_SetItemString(dict, "advance", advance);
  PyDict_SetItemString(dict, "position", position);
  PyDict_SetItemString(dict, "size", dims);
  return (PyObject *)dict;
}

PyObject *python_offset_core(PyObject *obj, double r, double delta, PyObject *chamfer,
                             CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<OffsetNode>(instance, discretizer);

  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in offset");
    return NULL;
  }

  node->delta = 1;
  node->chamfer = false;
  node->join_type = Clipper2Lib::JoinType::Round;
  if (!isnan(r)) {
    node->delta = r;
  } else if (!isnan(delta)) {
    node->delta = delta;
    node->join_type = Clipper2Lib::JoinType::Miter;
    if (chamfer == Py_True) {
      node->chamfer = true;
      node->join_type = Clipper2Lib::JoinType::Square;
    } else if (chamfer == Py_False || chamfer == NULL) node->chamfer = 0;
    else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for chamfer parameter");
      return NULL;
    }
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_offset(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "r", "delta", "chamfer", NULL};
  PyObject *obj = NULL;
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ddO", kwlist, &obj, &r, &delta, &chamfer)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r,delta)");
    return NULL;
  }
  return python_offset_core(obj, r, delta, chamfer, CreateCurveDiscretizer(kwargs));
}

PyObject *python_oo_offset(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"r", "delta", "chamfer", NULL};
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddO", kwlist, &r, &delta, &chamfer)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r,delta)");
    return NULL;
  }
  return python_offset_core(obj, r, delta, chamfer, CreateCurveDiscretizer(kwargs));
}

PyObject *python_projection_core(PyObject *obj, const char *cutmode, int convexity)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<ProjectionNode>(instance);
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in projection");
    return NULL;
  }

  node->convexity = convexity;
  node->cut_mode = 0;
  if (cutmode != NULL && !strcasecmp(cutmode, "cut")) node->cut_mode = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_projection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "cut", "convexity", NULL};
  PyObject *obj = NULL;
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sl", kwlist, &obj, &cutmode, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_oo_projection(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"cut", "convexity", NULL};
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sl", kwlist, &cutmode, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_group(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<GroupNode>(instance);

  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  PyObject *dummydict;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyOpenSCADType, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  child = PyOpenSCADObjectToNode(obj, &dummydict);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_align_core(PyObject *obj, PyObject *pyrefmat, PyObject *pydstmat)
{
  if (obj->ob_type != &PyOpenSCADType) {
    PyErr_SetString(PyExc_TypeError, "Must specify Object as 1st parameter");
    return nullptr;
  }
  PyObject *child_dict = nullptr;
  std::shared_ptr<AbstractNode> dstnode = PyOpenSCADObjectToNode(obj, &child_dict);
  if (dstnode == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Invalid align object");
    return Py_None;
  }
  DECLARE_INSTANCE();
  auto multmatnode = std::make_shared<TransformNode>(instance, "align");
  multmatnode->children.push_back(dstnode);
  Matrix4d mat;
  Matrix4d MT = Matrix4d::Identity();

  if (!python_tomatrix(pyrefmat, mat)) MT = MT * mat;
  if (!python_tomatrix(pydstmat, mat)) MT = MT * mat.inverse();

  multmatnode->matrix = MT;

  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, multmatnode);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      if (!python_tomatrix(value, mat)) {
        mat = MT * mat;
        PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, python_frommatrix(mat));
      } else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_align(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "refmat", "objmat", NULL};
  PyObject *obj = NULL;
  PyObject *pyrefmat = NULL;
  PyObject *pyobjmat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O", kwlist, &obj, &pyrefmat, &pyobjmat)) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat);
}

PyObject *python_oo_align(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"refmat", "objmat", NULL};
  PyObject *pyrefmat = NULL;
  PyObject *pyobjmat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &pyrefmat, &pyobjmat)) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat);
}

PyObject *python_str(PyObject *self)
{
  std::ostringstream stream;
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(self, &dummydict);
  if (node != nullptr) stream << "OpenSCAD (" << (int)node->index() << ")";
  else stream << "Invalid OpenSCAD Object";

  return PyUnicode_FromStringAndSize(stream.str().c_str(), stream.str().size());
}

PyMethodDef PyOpenSCADFunctions[] = {
  {"square", (PyCFunction)python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction)python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction)python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"text", (PyCFunction)python_text, METH_VARARGS | METH_KEYWORDS, "Create Text."},
  {"textmetrics", (PyCFunction)python_textmetrics, METH_VARARGS | METH_KEYWORDS, "Get textmetrics."},
  {"cube", (PyCFunction)python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction)python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction)python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction)python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},
  {"translate", (PyCFunction)python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction)python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction)python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"mirror", (PyCFunction)python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction)python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"divmatrix", (PyCFunction)python_divmatrix, METH_VARARGS | METH_KEYWORDS, "Divmatrix Object."},
  {"offset", (PyCFunction)python_offset, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  {"roof", (PyCFunction)python_roof, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
#endif
  {"color", (PyCFunction)python_color, METH_VARARGS | METH_KEYWORDS, "Color Object."},
  {"show", (PyCFunction)python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {"linear_extrude", (PyCFunction)python_linear_extrude, METH_VARARGS | METH_KEYWORDS,
   "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction)python_rotate_extrude, METH_VARARGS | METH_KEYWORDS,
   "Rotate_extrude Object."},
  {"union", (PyCFunction)python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction)python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction)python_intersection, METH_VARARGS | METH_KEYWORDS,
   "Intersection Object."},
  {"hull", (PyCFunction)python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction)python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction)python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction)python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},
  {"projection", (PyCFunction)python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction)python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},
  {"mesh", (PyCFunction)python_mesh, METH_VARARGS | METH_KEYWORDS, "exports mesh."},
  {"render", (PyCFunction)python_render, METH_VARARGS | METH_KEYWORDS, "Render Object."},
  {"align", (PyCFunction)python_align, METH_VARARGS | METH_KEYWORDS, "Align Object to another."},
  {NULL, NULL, 0, NULL}};

#define OO_METHOD_ENTRY(name, desc) \
  {#name, (PyCFunction)python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {
  OO_METHOD_ENTRY(translate, "Move Object") OO_METHOD_ENTRY(rotate, "Rotate Object")

    OO_METHOD_ENTRY(union, "Union Object") OO_METHOD_ENTRY(difference, "Difference Object")
      OO_METHOD_ENTRY(intersection, "Intersection Object") OO_METHOD_ENTRY(scale, "Scale Object")
        OO_METHOD_ENTRY(mirror, "Mirror Object") OO_METHOD_ENTRY(multmatrix, "Multmatrix Object")
          OO_METHOD_ENTRY(divmatrix, "Divmatrix Object") OO_METHOD_ENTRY(offset, "Offset Object")
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
            OO_METHOD_ENTRY(roof, "Roof Object")
#endif
              OO_METHOD_ENTRY(color, "Color Object")
                OO_METHOD_ENTRY(linear_extrude, "Linear_extrude Object") OO_METHOD_ENTRY(
                  rotate_extrude, "Rotate_extrude Object") OO_METHOD_ENTRY(resize, "Resize Object")

                  OO_METHOD_ENTRY(mesh, "Mesh Object") OO_METHOD_ENTRY(align, "Align Object to another")

                    OO_METHOD_ENTRY(show, "Show Object") OO_METHOD_ENTRY(projection, "Projection Object")
                      OO_METHOD_ENTRY(render, "Render Object"){NULL, NULL, 0, NULL}};

PyNumberMethods PyOpenSCADNumbers = {
  python_nb_add,       // binaryfunc nb_add
  python_nb_subtract,  // binaryfunc nb_subtract
  python_nb_mul,       // binaryfunc nb_multiply
  0,                   // binaryfunc nb_remainder
  0,                   // binaryfunc nb_divmod
  0,                   // ternaryfunc nb_power
  0,                   // unaryfunc nb_negative
  0,                   // unaryfunc nb_positive
  0,                   // unaryfunc nb_absolute
  0,                   // inquiry nb_bool
  0,                   // unaryfunc nb_invert
  0,                   // binaryfunc nb_lshift
  0,                   // binaryfunc nb_rshift
  python_nb_and,       // binaryfunc nb_and
  0,                   // binaryfunc nb_xor
  python_nb_or,        // binaryfunc nb_or
  0,                   // unaryfunc nb_int
  0,                   // void *nb_reserved
  0,                   // unaryfunc nb_float

  0,  // binaryfunc nb_inplace_add
  0,  // binaryfunc nb_inplace_subtract
  0,  // binaryfunc nb_inplace_multiply
  0,  // binaryfunc nb_inplace_remainder
  0,  // ternaryfunc nb_inplace_power
  0,  // binaryfunc nb_inplace_lshift
  0,  // binaryfunc nb_inplace_rshift
  0,  // binaryfunc nb_inplace_and
  0,  // binaryfunc nb_inplace_xor
  0,  // binaryfunc nb_inplace_or

  0,  // binaryfunc nb_floor_divide
  0,  // binaryfunc nb_true_divide
  0,  // binaryfunc nb_inplace_floor_divide
  0,  // binaryfunc nb_inplace_true_divide

  0,  // unaryfunc nb_index

  0,  // binaryfunc nb_matrix_multiply
  0   // binaryfunc nb_inplace_matrix_multiply
};

PyMappingMethods PyOpenSCADMapping = {0, python__getitem__, python__setitem__};
