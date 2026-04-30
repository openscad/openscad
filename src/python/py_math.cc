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

#include <math.h>
#include "genlang/genlang.h"
#include "linalg.h"
#include "GeometryUtils.h"
#include <Python.h>
#include "pyfunctions.h"
#include "pyopenscad.h"
#include "python/pyconversion.h"

PyObject *python_math_sub1(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"value", NULL};
  double arg;
  double result = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist, &arg)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing math function");
    return NULL;
  }
  switch (mode) {
  case 0: result = sin(arg * M_PI / 180.0); break;
  case 1: result = cos(arg * M_PI / 180.0); break;
  case 2: result = tan(arg * M_PI / 180.0); break;
  case 3: result = asin(arg) * 180.0 / M_PI; break;
  case 4: result = acos(arg) * 180.0 / M_PI; break;
  case 5: result = atan(arg) * 180.0 / M_PI; break;
  }
  return PyFloat_FromDouble(result);
}

PyObject *python_math_sub2(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  int dragflags = 0;
  char *kwlist[] = {"vec1", "vec2", NULL};
  PyObject *obj1 = nullptr;
  PyObject *obj2 = nullptr;
  Vector3d vec31(0, 0, 0);
  Vector3d vec32(0, 0, 0);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj1, &obj2)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing norm(vec3)");
    return NULL;
  }
  python_vectorval(obj1, 1, 3, &(vec31[0]), &(vec31[1]), &(vec31[2]), nullptr, &dragflags);
  python_vectorval(obj2, 1, 3, &(vec32[0]), &(vec32[1]), &(vec32[2]), nullptr, &dragflags);

  switch (mode) {
  case 0: return PyFloat_FromDouble(vec31.dot(vec32)); break;
  case 1: return python_fromvector(vec31.cross(vec32)); break;
  }
  Py_RETURN_NONE;
}

PyObject *python_sin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 0);
}

PyObject *python_cos(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 1);
}

PyObject *python_tan(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 2);
}

PyObject *python_asin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 3);
}

PyObject *python_acos(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 4);
}

PyObject *python_atan(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub1(self, args, kwargs, 5);
}

PyObject *python_dot(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub2(self, args, kwargs, 0);
}

PyObject *python_cross(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_math_sub2(self, args, kwargs, 1);
}

PyObject *python_norm(PyObject *self, PyObject *args, PyObject *kwargs)
{
  int dragflags = 0;
  char *kwlist[] = {"vec", NULL};
  double result = 0;
  PyObject *obj = nullptr;
  Vector3d vec3(0, 0, 0);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing norm(vec3)");
    return NULL;
  }
  python_vectorval(obj, 1, 3, &(vec3[0]), &(vec3[1]), &(vec3[2]), nullptr, &dragflags);

  result = sqrt(vec3[0] * vec3[0] + vec3[1] * vec3[1] + vec3[2] * vec3[2]);
  return PyFloat_FromDouble(result);
}

// --- PyOpenSCADVector

static PyObject *PyOpenSCADVector_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyOpenSCADVectorObject *self;
  self = (PyOpenSCADVectorObject *)type->tp_alloc(type, 0);

  if (self) {
    self->v[0] = 0;
    self->v[1] = 0;
    self->v[2] = 0;
  }

  return (PyObject *)self;
}

PyObject *python_vector(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"x", "y", "z", NULL};
  double x, y, z;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddd", kwlist, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing vector");
    return nullptr;
  }
  PyOpenSCADVectorObject *vec;
  vec = (PyOpenSCADVectorObject *)PyOpenSCADVectorType.tp_alloc(&PyOpenSCADVectorType, 0);
  if (vec) {
    vec->v[0] = x;
    vec->v[1] = y;
    vec->v[2] = z;
  }
  return (PyObject *)vec;
}

PyObject *PyOpenSCADVector_sub(PyObject *a, PyObject *b, int mode)
{
  if (!PyObject_TypeCheck(a, &PyOpenSCADVectorType)) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  PyOpenSCADVectorObject *v1 = (PyOpenSCADVectorObject *)a;
  double scale;
  if (mode == 3 && !python_numberval(b, &scale, nullptr, 0)) {  // scalar multiply
    PyOpenSCADVectorObject *result = PyObject_New(PyOpenSCADVectorObject, &PyOpenSCADVectorType);
    if (!result) return NULL;
    for (int i = 0; i < 3; i++) result->v[i] = v1->v[i] * scale;
    return (PyObject *)result;
  }

  if (!PyObject_TypeCheck(b, &PyOpenSCADVectorType)) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  PyOpenSCADVectorObject *v2 = (PyOpenSCADVectorObject *)b;

  if (mode == 3) {  // cross product
    Vector3d v1e(v1->v[0], v1->v[1], v1->v[2]);
    Vector3d v2e(v2->v[0], v2->v[1], v2->v[2]);
    return python_fromvector(v1e.cross(v2e));
  }

  double res[3];
  switch (mode) {
  case 0:  // add
    for (int i = 0; i < 3; i++) res[i] = v1->v[i] + v2->v[i];
    break;
  case 1:  // subtract
    for (int i = 0; i < 3; i++) res[i] = v1->v[i] - v2->v[i];
    break;
  case 2:  // dot
  case 4:  // norm
    for (int i = 0; i < 3; i++) res[i] = v1->v[i] * v2->v[i];
    if (mode == 4) return PyFloat_FromDouble(sqrt(res[0] + res[1] + res[2]));
    return PyFloat_FromDouble(res[0] + res[1] + res[2]);
    break;
  }
  PyOpenSCADVectorObject *result = PyObject_New(PyOpenSCADVectorObject, &PyOpenSCADVectorType);
  if (!result) return NULL;
  for (int i = 0; i < 3; i++) result->v[i] = res[i];

  return (PyObject *)result;  // TODO copy from res
}

PyObject *PyOpenSCADVector_add(PyObject *a, PyObject *b)
{
  return PyOpenSCADVector_sub(a, b, 0);
}
PyObject *PyOpenSCADVector_subtract(PyObject *a, PyObject *b)
{
  return PyOpenSCADVector_sub(a, b, 1);
}
PyObject *PyOpenSCADVector_multiply(PyObject *a, PyObject *b)
{
  return PyOpenSCADVector_sub(a, b, 3);
}

static PyNumberMethods PyOpenSCADVector_number_methods = {
  .nb_add = PyOpenSCADVector_add,
  .nb_subtract = PyOpenSCADVector_subtract,
  .nb_multiply = PyOpenSCADVector_multiply,
};

static PyObject *PyOpenSCADVector_repr(PyOpenSCADVectorObject *self)
{
  std::ostringstream stream;
  stream << "vector(" << self->v[0] << "," << self->v[1] << "," << self->v[2] << ")";
  return PyUnicode_FromStringAndSize(stream.str().c_str(), stream.str().size());
}

PyObject *PyOpenSCADVector_dot(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"vec1", NULL};
  PyObject *obj = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing dot");
    return NULL;
  }
  return PyOpenSCADVector_sub(self, obj, 2);
}

PyObject *PyOpenSCADVector_norm(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "norm() takes no arguments");
    return NULL;
  }
  return PyOpenSCADVector_sub(self, self, 4);
}

PyMethodDef PyOpenSCADVectorMethods[] = {
  {"dot", (PyCFunction)PyOpenSCADVector_dot, METH_VARARGS | METH_KEYWORDS, "Dot Product"},
  {"norm", (PyCFunction)PyOpenSCADVector_norm, METH_VARARGS | METH_KEYWORDS, "Length"},
  {NULL, NULL, 0, NULL}

};

PyObject *PyOpenSCADVector__getitem__(PyObject *obj, PyObject *key)
{
  PyOpenSCADVectorObject *self = (PyOpenSCADVectorObject *)obj;
  PyObject *result;
  if (!PyLong_Check(key)) {
    PyErr_SetString(PyExc_TypeError, "vector indices must be integers");
    return NULL;
  }
  long ind = PyLong_AsLong(key);
  if (ind == -1 && PyErr_Occurred()) {
    return NULL;
  }

  if (ind < 0 || ind >= 3) {
    PyErr_SetString(PyExc_IndexError, "vector index out of range");
    return NULL;
  }

  return PyFloat_FromDouble(self->v[ind]);

  PyErr_SetString(PyExc_TypeError, "Vector subscript must be a number between 0 and 2 ");
  return NULL;
}

int PyOpenSCADVector__setitem__(PyObject *obj, PyObject *key, PyObject *v)
{
  double result;
  if (!PyLong_Check(key)) {
    PyErr_SetString(PyExc_TypeError, "vector indices must be integers");
    return -1;
  }

  long ind = PyLong_AsLong(key);
  if (ind == -1 && PyErr_Occurred()) {
    return -1;
  }

  if (ind < 0 || ind >= 3) {
    PyErr_SetString(PyExc_IndexError, "vector index out of range");
    return -1;
  }

  if (python_numberval(v, &result, nullptr, 0)) {
    if (!PyErr_Occurred()) {
      PyErr_SetString(PyExc_TypeError, "vector elements must be numeric");
    }
  }
  PyOpenSCADVectorObject *self = (PyOpenSCADVectorObject *)obj;
  self->v[ind] = result;
  return 0;
}

PyMappingMethods PyOpenSCADVectorMapping = {0, PyOpenSCADVector__getitem__, PyOpenSCADVector__setitem__};

PyTypeObject PyOpenSCADVectorType = {
  PyVarObject_HEAD_INIT(NULL, 0).tp_name = "PyOpenSCADVector",
  .tp_basicsize = sizeof(PyOpenSCADVectorObject),
  .tp_itemsize = 0,
  .tp_repr = (reprfunc)PyOpenSCADVector_repr,
  .tp_as_number = &PyOpenSCADVector_number_methods,
  .tp_as_mapping = &PyOpenSCADVectorMapping,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = PyOpenSCADVectorMethods,
  .tp_init = (initproc)python_vector,
  .tp_new = PyOpenSCADVector_new,
};
