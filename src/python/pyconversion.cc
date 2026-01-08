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
