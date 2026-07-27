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
#include <algorithm>
#include <cmath>
#include <cstdio>
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
    // Handle other python number protocol objects (e.g. numpy.int64,
    // numpy.float64). This is what lets NumPy scalar elements flow
    // through the same coercion path as native Python numbers.
    PyObject *f = PyNumber_Float(number);
    if (f == nullptr) {
      PyErr_Clear();
      return 1;
    }
    *result = PyFloat_AsDouble(f);
    Py_DECREF(f);
    return 0;
  }
  if (PyUnicode_Check(number) && flags != nullptr) {
    /* python_numberval() is a best-effort coercion that returns
     * 0/1 and does not propagate Python exceptions to the caller.
     * The helper can fail with a TypeError on unencodable str
     * contents (lone surrogates under the "strict" handler) or on
     * a misbehaving custom utf-8 codec, and can propagate
     * MemoryError / KeyboardInterrupt verbatim. We have no way to
     * surface any of those cleanly here, so clear the indicator
     * and report "not coercible" to the caller. */
    std::string str_utf8;
    if (!python_pyobject_to_utf8(number, str_utf8, "python_numberval()")) {
      PyErr_Clear();
      return 1;
    }
    /* sscanf returns the number of input items successfully matched
     * (1 here on a parsed double, 0 on no match, EOF on empty input).
     * Writing into a local first means we never touch *result on the
     * not-a-number path -- the caller's variable keeps whatever it
     * was initialised to, so a non-numeric str surfaces as "not
     * coercible" instead of silently propagating *result's prior
     * (possibly uninitialised) value while still flipping flagor. */
    double parsed;
    if (std::sscanf(str_utf8.c_str(), "%lf", &parsed) != 1) {
      return 1;
    }
    *result = parsed;
    *flags |= flagor;
    return 0;
  }
  return 1;
}

int python_indexval(PyObject *number, long *result)
{
  if (number == nullptr) return 1;
  PyObject *index = PyNumber_Index(number);
  if (index == nullptr) return 1;
  const long value = PyLong_AsLong(index);
  Py_DECREF(index);
  if (value == -1 && PyErr_Occurred()) return 1;
  *result = value;
  return 0;
}

/*
 * Returns true when `o` should be treated as a numeric coordinate/index
 * container: a list, tuple, or any object implementing the sequence
 * protocol. Accepting the generic sequence protocol is what lets NumPy
 * ndarrays (and any other array-like) flow through exactly the same code
 * paths as native Python lists, without a build-time dependency on NumPy.
 *
 * Strings/bytes are sequences too but are never valid coordinate data, and
 * a PyOpenSCAD object exposes its children through the sequence protocol --
 * none of those should be mistaken for a coordinate list, so they are
 * excluded here. The native PyOpenSCADVector type is handled explicitly by
 * every caller before this helper is consulted.
 */
bool python_is_sequence(PyObject *o)
{
  if (o == nullptr) return false;
  if (PyUnicode_Check(o) || PyBytes_Check(o) || PyByteArray_Check(o)) return false;
  if (PyDict_Check(o)) return false;
  if (PyObject_TypeCheck(o, &PyOpenSCADType)) return false;
  return PyList_Check(o) || PyTuple_Check(o) || PySequence_Check(o);
}

std::vector<int> python_intlistval(PyObject *list)
{
  std::vector<int> result;
  if (list == nullptr) return result;
  long value;
  if (!python_indexval(list, &value)) {
    result.push_back(static_cast<int>(value));
    return result;
  }
  PyErr_Clear();
  // Accept lists, tuples and NumPy arrays of integers (or numpy int scalars).
  if (python_is_sequence(list)) {
    PyObject *seq = PySequence_Fast(list, "expected a sequence of integers");
    if (seq == nullptr) {
      PyErr_Clear();
      return result;
    }
    Py_ssize_t size = PySequence_Fast_GET_SIZE(seq);
    for (Py_ssize_t i = 0; i < size; i++) {
      if (!python_indexval(PySequence_Fast_GET_ITEM(seq, i), &value)) {
        result.push_back(static_cast<int>(value));
      } else {
        PyErr_Clear();
      }
    }
    Py_DECREF(seq);
  }
  return result;
}

/*
 * Tries to extract a 1-4 dimensional vector out of a python list, tuple,
 * NumPy array, PyOpenSCADVector or scalar (broadcast). Returns 0 on
 * success, 1 on failure.
 */

int python_vectorval(PyObject *vec, int minval, int maxval, double *x, double *y, double *z, double *w,
                     int *flags)
{
  if (vec == nullptr) return 1;
  if (flags != nullptr) *flags = 0;
  if (vec->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *v = (PyOpenSCADVectorObject *)vec;
    *x = v->v[0];
    *y = v->v[1];
    *z = v->v[2];
    return 0;
  }
  if (python_is_sequence(vec)) {
    PyObject *seq = PySequence_Fast(vec, "expected a coordinate sequence");
    if (seq == nullptr) {
      PyErr_Clear();
      return 1;
    }
    Py_ssize_t size = PySequence_Fast_GET_SIZE(seq);
    if (size < minval || size > maxval) {
      Py_DECREF(seq);
      return 1;
    }
    int rc = 0;
    if (size >= 1) rc = python_numberval(PySequence_Fast_GET_ITEM(seq, 0), x, flags, 1);
    if (!rc && size >= 2) rc = python_numberval(PySequence_Fast_GET_ITEM(seq, 1), y, flags, 2);
    if (!rc && size >= 3) rc = python_numberval(PySequence_Fast_GET_ITEM(seq, 2), z, flags, 4);
    if (!rc && size >= 4 && w != NULL)
      rc = python_numberval(PySequence_Fast_GET_ITEM(seq, 3), w, flags, 8);
    Py_DECREF(seq);
    return rc ? 1 : 0;
  }
  if (!python_numberval(vec, x, nullptr, 0)) {
    *y = *x;
    *z = *x;
    if (w != NULL) *w = *x;
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
  mat = Matrix4d::Identity();
  if (!python_is_sequence(pyt)) return 1;
  PyObject *rows = PySequence_Fast(pyt, "expected a matrix (sequence of rows)");
  if (rows == nullptr) {
    PyErr_Clear();
    return 1;
  }
  Py_ssize_t nrows = PySequence_Fast_GET_SIZE(rows);
  if (nrows != 4) {
    Py_DECREF(rows);
    return 1;
  }
  int rc = 0;
  for (Py_ssize_t i = 0; i < 4 && !rc; i++) {
    PyObject *row = PySequence_Fast_GET_ITEM(rows, i);
    if (!python_is_sequence(row)) {
      rc = 1;
      break;
    }
    PyObject *cells = PySequence_Fast(row, "expected a matrix row (sequence)");
    if (cells == nullptr) {
      PyErr_Clear();
      rc = 1;
      break;
    }
    Py_ssize_t ncols = PySequence_Fast_GET_SIZE(cells);
    if (ncols != 4) {
      Py_DECREF(cells);
      rc = 1;
      break;
    }
    for (Py_ssize_t j = 0; j < 4; j++) {
      double val;
      if (python_numberval(PySequence_Fast_GET_ITEM(cells, j), &val, nullptr, 0)) {
        rc = 1;
        break;
      }
      mat(i, j) = val;
    }
    Py_DECREF(cells);
  }
  Py_DECREF(rows);
  return rc;
}

int python_tovector(PyObject *pyt, Vector3d& vec)
{
  if (pyt == nullptr) return 1;
  if (pyt->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *v = (PyOpenSCADVectorObject *)pyt;
    for (int i = 0; i < 3; i++) vec[i] = v->v[i];
    return 0;
  }
  if (!python_is_sequence(pyt)) return 1;
  PyObject *seq = PySequence_Fast(pyt, "expected a 3D coordinate sequence");
  if (seq == nullptr) {
    PyErr_Clear();
    return 1;
  }
  if (PySequence_Fast_GET_SIZE(seq) != 3) {
    Py_DECREF(seq);
    return 1;
  }
  int rc = 0;
  for (int i = 0; i < 3; i++) {
    double val;
    if (python_numberval(PySequence_Fast_GET_ITEM(seq, i), &val, nullptr, 0)) {
      rc = 1;
      break;
    }
    vec[i] = val;
  }
  Py_DECREF(seq);
  return rc;
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
  if (pypoints == nullptr || !python_is_sequence(pypoints)) {
    PyErr_SetString(PyExc_TypeError, "Polygon points must be a list of coordinates");
    return points;
  }
  PyObject *seq = PySequence_Fast(pypoints, "expected a list of coordinates");
  if (seq == nullptr) {
    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError, "Polygon points must be a list of coordinates");
    return points;
  }
  Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
  if (n == 0) {
    Py_DECREF(seq);
    PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polygon");
    return points;
  }
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject *element = PySequence_Fast_GET_ITEM(seq, i);
    Vector3d point(0, 0, 0);  // default no radius in the 3rd component
    if (!python_is_sequence(element) ||
        python_vectorval(element, 2, 3, &point[0], &point[1], &point[2])) {
      Py_DECREF(seq);
      PyErr_SetString(PyExc_TypeError, "Coordinate must contain 2 or 3 numbers");
      points.clear();
      return points;
    }
    points.push_back(point);
  }
  Py_DECREF(seq);
  return points;
}

std::vector<std::vector<size_t>> python_to2dintlist(PyObject *pypaths)
{
  std::vector<std::vector<size_t>> result;
  if (pypaths == nullptr) return result;
  if (!python_is_sequence(pypaths)) {
    PyErr_SetString(PyExc_TypeError, "Polygon paths must be a sequence of index sequences");
    return result;
  }
  PyObject *seq = PySequence_Fast(pypaths, "expected a list of paths");
  if (seq == nullptr) {
    return result;
  }
  Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
  if (n == 0) {
    Py_DECREF(seq);
    PyErr_SetString(PyExc_TypeError, "must specify at least 1 path when specified");
    return result;
  }
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject *element = PySequence_Fast_GET_ITEM(seq, i);
    if (!python_is_sequence(element)) {
      PyErr_SetString(PyExc_TypeError, "Polygon path must be a list of indices");
      result.clear();
      Py_DECREF(seq);
      return result;
    }
    PyObject *sub = PySequence_Fast(element, "expected a list of indices");
    if (sub == nullptr) {
      result.clear();
      Py_DECREF(seq);
      return result;
    }
    std::vector<size_t> path;
    Py_ssize_t m = PySequence_Fast_GET_SIZE(sub);
    for (Py_ssize_t j = 0; j < m; j++) {
      long pointIndex;
      if (python_indexval(PySequence_Fast_GET_ITEM(sub, j), &pointIndex)) {
        PyErr_SetString(PyExc_TypeError, "Polygon Point Index must be an integer");
        Py_DECREF(sub);
        Py_DECREF(seq);
        result.clear();
        return result;
      }
      if (pointIndex < 0) {  // TODO fix || pointIndex >= node->points.size()) {
        PyErr_SetString(PyExc_TypeError, "Polygon Point Index out of range");
        Py_DECREF(sub);
        Py_DECREF(seq);
        return result;
      }
      path.push_back(pointIndex);
    }
    Py_DECREF(sub);
    result.push_back(std::move(path));
  }
  Py_DECREF(seq);
  return result;
}

PyObject *python_from2dvarpointlist(const std::vector<Vector3d>& ptlist)
{
  int n = ptlist.size();
  PyObject *result = PyList_New(n);
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
  int n = ptlist.size();
  PyObject *result = PyList_New(n);
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
  int n = intlist.size();
  PyObject *result = PyList_New(n);
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
  int n = intlist.size();
  PyObject *result = PyList_New(n);
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
 * Accepts Python lists/tuples, NumPy arrays or OpenSCAD vector objects
 * within the given dimensions.
 */

std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim, int *dragflags)
{
  std::vector<Vector3d> results;
  if (vec == nullptr) return results;
  if (python_is_sequence(vec)) {
    PyObject *seq = PySequence_Fast(vec, "expected a vector or list of vectors");
    if (seq == nullptr) {
      return results;
    }
    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    // Decide whether this is a list of vectors or a single flat vector by
    // checking whether every element is itself a sequence/vector.
    bool listOfVectors = (n > 0);
    for (Py_ssize_t i = 0; i < n; i++) {
      PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
      if (!python_is_sequence(item) && item->ob_type != &PyOpenSCADVectorType) {
        listOfVectors = false;
        break;
      }
    }
    if (listOfVectors) {
      for (Py_ssize_t j = 0; j < n; j++) {
        Vector3d result(0, 0, 0);
        PyObject *item = PySequence_Fast_GET_ITEM(seq, j);
        if (item->ob_type == &PyOpenSCADVectorType) {
          PyOpenSCADVectorObject *obj = (PyOpenSCADVectorObject *)item;
          for (int i = 0; i < 3; i++) result[i] = obj->v[i];
        } else {
          PyObject *inner = PySequence_Fast(item, "expected a vector");
          if (inner == nullptr) {
            Py_DECREF(seq);
            return results;
          }
          Py_ssize_t m = PySequence_Fast_GET_SIZE(inner);
          if (m < mindim || m > maxdim) {
            PyErr_Format(PyExc_TypeError, "expected a vector with %d to %d elements", mindim, maxdim);
            Py_DECREF(inner);
            Py_DECREF(seq);
            return results;
          }
          for (Py_ssize_t i = 0; i < m && i < 3; i++) {
            if (python_numberval(PySequence_Fast_GET_ITEM(inner, i), &result[i], nullptr, 0)) {
              Py_DECREF(inner);
              Py_DECREF(seq);
              return results;  // Error
            }
          }
          Py_DECREF(inner);
        }
        results.push_back(result);
      }
      Py_DECREF(seq);
      return results;
    }
    // A single flat vector: [x, y, z]
    Vector3d result(0, 0, 0);
    if (n < mindim || n > maxdim) {
      PyErr_Format(PyExc_TypeError, "expected a vector with %d to %d elements", mindim, maxdim);
      Py_DECREF(seq);
      return results;
    }
    for (Py_ssize_t i = 0; i < n && i < 3; i++) {
      if (python_numberval(PySequence_Fast_GET_ITEM(seq, i), &result[i], dragflags, 1 << i)) {
        Py_DECREF(seq);
        return results;  // Error
      }
    }
    Py_DECREF(seq);
    results.push_back(result);
    return results;
  }
  // Scalar broadcast: n -> (n, n, n)
  Vector3d result(0, 0, 0);
  if (!python_numberval(vec, &result[0], nullptr, 0)) {
    result[1] = result[0];
    result[2] = result[0];
    results.push_back(result);
    return results;
  }
  if (vec->ob_type == &PyOpenSCADVectorType) {
    PyOpenSCADVectorObject *obj = (PyOpenSCADVectorObject *)vec;
    for (int i = 0; i < 3; i++) result[i] = obj->v[i];
    results.push_back(result);
  }
  return results;
}
