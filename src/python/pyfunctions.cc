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
#include "src/python/pyopenscad.h"
#include "src/core/primitives.h"

PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CubeNode>(instance);

  char *kwlist[] = {"size", "center", NULL};
  PyObject *size = NULL;

  PyObject *center = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &size,
                                   &center)){
    PyErr_SetString(PyExc_TypeError, "Error during parsing cube(size)");
    return NULL;
  }	  

  if (size != NULL) {
    if (python_vectorval(size, 3, 3, &(node->x), &(node->y), &(node->z))) {
      PyErr_SetString(PyExc_TypeError, "Invalid Cube dimensions");
      return NULL;
    }
  }
  if(node->x <= 0 || node->y <= 0 || node ->z <= 0) {
      PyErr_SetString(PyExc_TypeError, "Cube Dimensions must be positive");
      return NULL;
  }
  node->center = false;
  if (center == Py_False || center == NULL ) ;
  else if (center == Py_True){
    for(int i=0;i<3;i++) node->center=true;
  } else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_show_core(PyObject *obj)
{
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in output");
    return NULL;
  }
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  python_result_node = child;
  return Py_None;
}

PyObject *python_show(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *kwlist[] = {"obj", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &obj
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_show_core(obj);
}

PyMethodDef PyOpenSCADFunctions[] = {
  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"show", (PyCFunction) python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {NULL, NULL, 0, NULL}
};

