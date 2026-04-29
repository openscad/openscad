/*
 *  PythonSCAD (www.pythonscad.org)
 *  Copyright (C) 2024-2026 Guenther Sohler <guenther.sohler@gmail.com> and
 *                          Nomike <nomike@nomike.com>
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
#include "geometry/linalg.h"
#include "geometry/GeometryUtils.h"
#include "pyconversion.h"
#include "pyopenscad.h"
#include "pydata.h"

int python_numberval(PyObject *number, double *result, int *flags, int flagor)
{
  if (number == nullptr) return 1;
  if (number == Py_False) return 1;
  if (number == Py_True) return 1;
  if (number == Py_None) return 1;
  if (PyFloat_Check(number)) {
    *result = PyFloat_AsDouble(number);
    return 0;
  }
  if (PyLong_Check(number)) {
    *result = PyLong_AsLong(number);
    return 0;
  }
  if (number->ob_type == &PyDataType && flags != nullptr) {
    *flags |= flagor;
    *result = PyDataObjectToValue(number);
    return 0;
  }
  if (PyNumber_Check(number)) {
    // Handle other python number protocol objects (e.g. numpy.int64)
    PyObject *f = PyNumber_Float(number);
    *result = PyFloat_AsDouble(f);
    return 0;
  }
  if (PyUnicode_Check(number) && flags != nullptr) {
    PyObjectUniquePtr str(PyUnicode_AsEncodedString(number, "utf-8", "~"), &PyObjectDeleter);
    const char *str1 = PyBytes_AS_STRING(str.get());
    sscanf(str1, "%lf", result);
    *flags |= flagor;
    return 0;
  }
  return 1;
}

std::vector<int> python_intlistval(PyObject *list)
{
  std::vector<int> result;
  if (PyLong_Check(list)) {
    result.push_back(PyLong_AsLong(list));
  }
  if (PyList_Check(list)) {
    for (int i = 0; i < PyList_Size(list); i++) {
      PyObject *item = PyList_GetItem(list, i);
      if (PyLong_Check(item)) {
        result.push_back(PyLong_AsLong(item));
      }
    }
  }
  return result;
}
/*
 * Tries to extract an 3D vector out of a python list
 */

int python_vectorval(PyObject *vec, int minval, int maxval, double *x, double *y, double *z, double *w,
                     int *flags)
{
  if (vec == nullptr) return 1;
  if (flags != nullptr) *flags = 0;
  if (PyList_Check(vec)) {
    if (PyList_Size(vec) < minval || PyList_Size(vec) > maxval) return 1;

    if (PyList_Size(vec) >= 1) {
      if (python_numberval(PyList_GetItem(vec, 0), x, flags, 1)) return 1;
    }
    if (PyList_Size(vec) >= 2) {
      if (python_numberval(PyList_GetItem(vec, 1), y, flags, 2)) return 1;
    }
    if (PyList_Size(vec) >= 3) {
      if (python_numberval(PyList_GetItem(vec, 2), z, flags, 4)) return 1;
    }
    if (PyList_Size(vec) >= 4 && w != NULL) {
      if (python_numberval(PyList_GetItem(vec, 3), w, flags, 8)) return 1;
    }
    return 0;
  }
  if (!python_numberval(vec, x, nullptr, 0)) {
    *y = *x;
    *z = *x;
    if (w != NULL) *w = *x;
    return 0;
  }
  if (vec->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *v = (PyOpenSCADVectorObject *)vec;
    *x = v->v[0];
    *y = v->v[1];
    *z = v->v[2];
    return 0;
  }
  return 1;
}

PyObject *python_frommatrix(const Matrix4d& mat)
{
  PyObject *pyo = PyList_New(4);
  for (int i = 0; i < 4; i++) {
    PyObject *row = PyList_New(4);
    for (int j = 0; j < 4; j++) PyList_SetItem(row, j, PyFloat_FromDouble(mat(i, j)));
    PyList_SetItem(pyo, i, row);
  }
  return pyo;
}

int python_tomatrix(PyObject *pyt, Matrix4d& mat)
{
  if (pyt == nullptr) return 1;
  double val;
  mat = Matrix4d::Identity();
  if (!PyList_Check(pyt)) return 1;
  for (int i = 0; i < std::min(4, (int)PyList_Size(pyt)); i++) {
    PyObject *row = PyList_GetItem(pyt, i);
    if (!PyList_Check(row)) return 1;
    for (int j = 0; j < std::min(4, (int)PyList_Size(row)); j++) {
      PyObject *cell = PyList_GetItem(row, j);
      if (python_numberval(cell, &val, nullptr, 0)) return 1;
      mat(i, j) = val;
    }
  }
  return 0;
}

int python_tovector(PyObject *pyt, Vector3d& vec)
{
  if (pyt == nullptr) return 1;
  if (pyt->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *v = (PyOpenSCADVectorObject *)pyt;
    for (int i = 0; i < 3; i++) vec[i] = v->v[i];
    return 0;
  }
  double val;
  if (!PyList_Check(pyt)) return 1;
  if (PyList_Size(pyt) != 3) return 1;
  for (int i = 0; i < 3; i++) {
    PyObject *cell = PyList_GetItem(pyt, i);
    if (python_numberval(cell, &val, nullptr, 0)) return 1;
    vec[i] = val;
  }
  return 0;
}

PyObject *python_fromvector(const Vector3d vec)
{
  PyOpenSCADVectorObject *pyvec =
    (PyOpenSCADVectorObject *)PyOpenSCADVectorType.tp_alloc(&PyOpenSCADVectorType, 0);
  if (pyvec)
    for (int i = 0; i < 3; i++) pyvec->v[i] = vec[i];
  return (PyObject *)pyvec;
}

std::vector<Vector3d> python_to2dvarpointlist(PyObject *pypoints)
{
  std::vector<Vector3d> points;
  Vector3d point;
  if (pypoints != NULL && PyList_Check(pypoints)) {
    if (PyList_Size(pypoints) == 0) {
      PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polygon");
      return points;
    }
    for (int i = 0; i < PyList_Size(pypoints); i++) {
      PyObject *element = PyList_GetItem(pypoints, i);
      point[2] = 0;  // default no radius
      if (python_vectorval(element, 2, 3, &point[0], &point[1], &point[2])) {
        PyErr_SetString(PyExc_TypeError, "Coordinate must contain 2 or 3 numbers");
        points.clear();
        return points;
      }
      points.push_back(point);
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polygon points must be a list of coordinates");
    points.clear();
    return points;
  }
  return points;
}

std::vector<std::vector<size_t>> python_to2dintlist(PyObject *pypaths)
{
  std::vector<std::vector<size_t>> result;
  if (pypaths != NULL && PyList_Check(pypaths)) {
    if (PyList_Size(pypaths) == 0) {
      PyErr_SetString(PyExc_TypeError, "must specify at least 1 path when specified");
      return result;
    }
    for (int i = 0; i < PyList_Size(pypaths); i++) {
      PyObject *element = PyList_GetItem(pypaths, i);
      if (PyList_Check(element)) {
        std::vector<size_t> path;
        for (int j = 0; j < PyList_Size(element); j++) {
          int pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
          if (pointIndex < 0) {  // TODO fix || pointIndex >= node->points.size()) {
            PyErr_SetString(PyExc_TypeError, "Polyhedron Point Index out of range");
            return result;
          }
          path.push_back(pointIndex);
        }
        result.push_back(std::move(path));
      } else {
        PyErr_SetString(PyExc_TypeError, "Polygon path must be a list of indices");
        result.clear();
      }
    }
  }
  return result;
}

PyObject *python_from2dvarpointlist(const std::vector<Vector3d>& ptlist)
{
  PyObject *result;
  int n = ptlist.size();
  result = PyList_New(n);
  for (int i = 0; i < n; i++) {
    int dim = 2;
    if (fabs(ptlist[i][2]) > 1e-6) dim = 3;
    PyObject *coord = PyList_New(dim);
    for (int j = 0; j < dim; j++) {
      PyList_SetItem(coord, j, PyFloat_FromDouble(ptlist[i][j]));
    }
    PyList_SetItem(result, i, coord);
  }

  return result;
}

PyObject *python_from3dpointlist(const std::vector<Vector3d>& ptlist)
{
  PyObject *result;
  int n = ptlist.size();
  result = PyList_New(n);
  for (int i = 0; i < n; i++) {
    int dim = 3;
    PyObject *coord = PyList_New(dim);
    for (int j = 0; j < dim; j++) {
      PyList_SetItem(coord, j, PyFloat_FromDouble(ptlist[i][j]));
    }
    PyList_SetItem(result, i, coord);
  }

  return result;
}

PyObject *python_from2dint(const std::vector<std::vector<size_t>>& intlist)
{
  PyObject *result;
  int n = intlist.size();
  result = PyList_New(n);
  for (int i = 0; i < n; i++) {
    int m = intlist[i].size();
    PyObject *subresult = PyList_New(m);
    for (int j = 0; j < m; j++) {
      PyList_SetItem(subresult, j, PyLong_FromLong(intlist[i][j]));
    }
    PyList_SetItem(result, i, subresult);
  }

  return result;
}

PyObject *python_from2dlong(const std::vector<IndexedFace>& intlist)
{
  PyObject *result;
  int n = intlist.size();
  result = PyList_New(n);
  for (int i = 0; i < n; i++) {
    int m = intlist[i].size();
    PyObject *subresult = PyList_New(m);
    for (int j = 0; j < m; j++) {
      PyList_SetItem(subresult, j, PyLong_FromLong(intlist[i][j]));
    }
    PyList_SetItem(result, i, subresult);
  }

  return result;
}

/*
 * Parses a Python object into one or more Vector3d values.
 * Accepts Python lists or OpenSCAD vector objects within the given dimensions.
 */

std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim, int *dragflags)
{
  std::vector<Vector3d> results;
  if (PyList_Check(vec)) {
    // check if its a valid vec<Vector3d>
    int valid = 1;
    for (int i = 0; valid && i < PyList_Size(vec); i++) {
      PyObject *item = PyList_GetItem(vec, i);
      if (!PyList_Check(item) && item->ob_type != &PyOpenSCADVectorType) valid = 0;
    }
    if (valid) {
      for (int j = 0; j < PyList_Size(vec); j++) {
        Vector3d result(0, 0, 0);
        PyObject *item = PyList_GetItem(vec, j);
        if (PyList_Check(item)) {
          if (PyList_Size(item) >= mindim && PyList_Size(item) <= maxdim) {
            for (int i = 0; i < PyList_Size(item); i++) {
              if (PyList_Size(item) > i) {
                if (python_numberval(PyList_GetItem(item, i), &result[i], nullptr, 0))
                  return results;  // Error
              }
            }
          }
        } else if (item->ob_type == &PyOpenSCADVectorType) {
          PyOpenSCADVectorObject *obj = (PyOpenSCADVectorObject *)item;
          for (int i = 0; i < 3; i++) result[i] = obj->v[i];
        }
        results.push_back(result);
      }
      return results;
    }
    Vector3d result(0, 0, 0);
    if (PyList_Size(vec) >= mindim && PyList_Size(vec) <= maxdim) {
      for (int i = 0; i < PyList_Size(vec); i++) {
        if (PyList_Size(vec) > i) {
          if (python_numberval(PyList_GetItem(vec, i), &result[i], dragflags, 1 << i))
            return results;  // Error
        }
      }
    }
    results.push_back(result);
  }
  Vector3d result(0, 0, 0);
  if (!python_numberval(vec, &result[0], nullptr, 0)) {
    result[1] = result[0];
    result[2] = result[1];
    results.push_back(result);
  }
  if (vec->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *obj = (PyOpenSCADVectorObject *)vec;
    for (int i = 0; i < 3; i++) result[i] = obj->v[i];
    results.push_back(result);
  }
  return results;  // Error
}
