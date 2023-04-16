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
#include "pyopenscad.h"
#include "CsgOpNode.h"
#include "PlatformUtils.h"

// https://docs.python.org/3.10/extending/newtypes.html 

static void PyOpenSCADObject_dealloc(PyOpenSCADObject *self)
{
  Py_XDECREF(self->dict);
//  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PyOpenSCADObject_alloc(PyTypeObject *cls, Py_ssize_t nitems)
{
  return PyType_GenericAlloc(cls, nitems);
}

static PyObject *PyOpenSCADObject_new(PyTypeObject *type, PyObject *args,  PyObject *kwds)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)  type->tp_alloc(type, 0);
  self->node = NULL;
  self->dict = PyDict_New();
  Py_XINCREF(self->dict);
  Py_XINCREF(self);
  return (PyObject *)self;
}

PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, std::shared_ptr<AbstractNode> node)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)  type->tp_alloc(type, 0);
  if (self != NULL) {
    self->node = node;
    self->dict = PyDict_New();
    Py_XINCREF(self->dict);
    Py_XINCREF(self);
    return (PyObject *)self;
  }
  return NULL;
}

int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj) {
  int i, n;
  PyObject *obj;
  std::shared_ptr<AbstractNode> child;
  if (PyList_Check(more_obj)) {
    n = PyList_Size(more_obj);
    for (i = 0; i < n; i++) {
      obj = PyList_GetItem(more_obj, i);
      child = PyOpenSCADObjectToNode(obj);
      children.push_back(child);
    }
  } else if (Py_TYPE(more_obj) == &PyOpenSCADType) {
    child = PyOpenSCADObjectToNode(more_obj);
    children.push_back(child);
  } else return 1;
  return 0;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj)
{
  std::shared_ptr<AbstractNode> result = ((PyOpenSCADObject *) obj)->node;
  Py_XDECREF(obj); 
  return result;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs)
{
  std::shared_ptr<AbstractNode> result;
  if (Py_TYPE(objs) == &PyOpenSCADType) {
    result = ((PyOpenSCADObject *) objs)->node;
  } else if (PyList_Check(objs)) {

    DECLARE_INSTANCE
    auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    int n = PyList_Size(objs);
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj);
      node->children.push_back(child);
      Py_XDECREF(obj);
    }
    result=node;
  } else result=NULL;
  Py_XDECREF(objs);
  return result;
}

int python_numberval(PyObject *number, double *result)
{
  if (PyFloat_Check(number)) {
    *result = PyFloat_AsDouble(number);
    return 0;
  }
  if (PyLong_Check(number)) {
    *result = PyLong_AsLong(number);
    return 0;
  }
  return 1;
}

int python_vectorval(PyObject *vec, double *x, double *y, double *z)
{
  *x = 1;
  *y = 1;
  *z = 1;
  if (PyList_Check(vec)) {
    if (PyList_Size(vec) >= 2) {
      if (python_numberval(PyList_GetItem(vec, 0), x)) return 1;
      if (python_numberval(PyList_GetItem(vec, 1), y)) return 1;
    }
    if (PyList_Check(vec) && PyList_Size(vec) >= 3) {
      if (python_numberval(PyList_GetItem(vec, 2), z)) return 1;
    }
    return 0;
  }
  if (!python_numberval(vec, x)) {
    *y = *x;
    *z = *x;
    return 0;
  }
  return 1;
}

static int PyOpenSCADInit(PyOpenSCADObject *self, PyObject *arfs, PyObject *kwds)
{
  return 0;
}

static PyMethodDef PyOpenSCADFunctions[] = {
  {"square", (PyCFunction) python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction) python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction) python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"text", (PyCFunction) python_text, METH_VARARGS | METH_KEYWORDS, "Create Text."},
  {"textmetrics", (PyCFunction) python_textmetrics, METH_VARARGS | METH_KEYWORDS, "Get textmetrics."},

  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction) python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction) python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction) python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},

  {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction) python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction) python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"mirror", (PyCFunction) python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction) python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"offset", (PyCFunction) python_offset, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
  {"roof", (PyCFunction) python_roof, METH_VARARGS | METH_KEYWORDS, "Roof Object."},

  {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},

  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"hull", (PyCFunction) python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction) python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction) python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction) python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},
  {"render", (PyCFunction) python_render, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"group", (PyCFunction) python_group, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},

  {"projection", (PyCFunction) python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction) python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},
  {"osimport", (PyCFunction) python_import, METH_VARARGS | METH_KEYWORDS, "Import Object."},
  {"color", (PyCFunction) python_color, METH_VARARGS | METH_KEYWORDS, "Import Object."},

  {"output", (PyCFunction) python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"version", (PyCFunction) python_version, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"version_num", (PyCFunction) python_version_num, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {NULL, NULL, 0, NULL}
};
// TODO missing
//  dxf_dim
//  dxf_cross
//  intersection_for
//  dxf_linear_extrude
//  dxf_rotate_extrude
//  fontmetrics

PyObject *python_oo_args(PyObject *self, PyObject *args)
{
  int i;
  PyObject *item;
  int n = PyTuple_Size(args);
  PyObject *new_args = PyTuple_New(n + 1);
//	Py_INCREF(new_args); // dont decref either,  dont know why its not working

  Py_INCREF(self);
  int ret = PyTuple_SetItem(new_args, 0, self);
  item = PyTuple_GetItem(new_args, 0);

  for (i = 0; i < PyTuple_Size(args); i++) {
    item = PyTuple_GetItem(args, i);
    Py_INCREF(item);
    PyTuple_SetItem(new_args, i + 1, item);
  }
  return new_args;
}

void get_fnas(double& fn, double& fa, double& fs) {
  PyObject *mainModule = PyImport_AddModule("__main__");
  if (mainModule == NULL) return;
  PyObject *varFn = PyObject_GetAttrString(mainModule, "fn");
  PyObject *varFa = PyObject_GetAttrString(mainModule, "fa");
  PyObject *varFs = PyObject_GetAttrString(mainModule, "fs");
  if (varFn != NULL) fn = PyFloat_AsDouble(varFn);
  if (varFa != NULL) fa = PyFloat_AsDouble(varFa);
  if (varFs != NULL) fs = PyFloat_AsDouble(varFs);
}

static PyMethodDef PyOpenSCADMethods[] = {
  {"translate", (PyCFunction) python_translate_oo, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction) python_rotate_oo, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction) python_scale_oo, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"union", (PyCFunction) python_union_oo, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference_oo, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection_oo, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"mirror", (PyCFunction) python_mirror_oo, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction) python_multmatrix_oo, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"linear_extrude", (PyCFunction) python_linear_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},
  {"offset", (PyCFunction) python_offset_oo, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
  {"roof", (PyCFunction) python_roof_oo, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
  {"output", (PyCFunction) python_output_oo, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"color", (PyCFunction) python_color_oo, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {NULL, NULL, 0, NULL}
};

PyNumberMethods PyOpenSCADNumbers =
{
  &python_nb_add,
  &python_nb_substract,
  &python_nb_multiply
};

PyMappingMethods PyOpenSCADMapping =
{
  0,
  python__getitem__,
  python__setitem__
};

PyTypeObject PyOpenSCADType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "PyOpenSCAD",             			/* tp_name */
    sizeof(PyOpenSCADObject), 			/* tp_basicsize */
    0,                         			/* tp_itemsize */
    (destructor) PyOpenSCADObject_dealloc,	/* tp_dealloc */
    0,                         			/* vectorcall_offset */
    0,                         			/* tp_getattr */
    0,                         			/* tp_setattr */
    0,                         			/* tp_as_async */
    0,                         			/* tp_repr */
    &PyOpenSCADNumbers,        			/* tp_as_number */
    0,                         			/* tp_as_sequence */
    &PyOpenSCADMapping,        			/* tp_as_mapping */
    0,                         			/* tp_hash  */
    0,                         			/* tp_call */
    0,                         			/* tp_str */
    0,                         			/* tp_getattro */
    0,                         			/* tp_setattro */
    0,                         			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
    "PyOpenSCAD Object",          		/* tp_doc */
    0,                         			/* tp_traverse */
    0,                         			/* tp_clear */
    0,                         			/* tp_richcompare */
    0,                         			/* tp_weaklistoffset */
    0,                         			/* tp_iter */
    0,                         			/* tp_iternext */
    PyOpenSCADMethods,             		/* tp_methods */
    0,             				/* tp_members */
    0,                         			/* tp_getset */
    0,                         			/* tp_base */
    0,                         			/* tp_dict */
    0,                         			/* tp_descr_get */
    0,                         			/* tp_descr_set */
    0,                         			/* tp_dictoffset */
    (initproc) PyOpenSCADInit,      		/* tp_init */
    PyOpenSCADObject_alloc,    			/* tp_alloc */
    PyOpenSCADObject_new,                	/* tp_new */
};


static PyModuleDef OpenSCADModule = {
  PyModuleDef_HEAD_INIT,
  "openscad",
  "Example module that creates an extension type.",
  -1,
  PyOpenSCADFunctions,
  NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_PyOpenSCAD(void)
{
  PyObject *m;

  if (PyType_Ready(&PyOpenSCADType) < 0) return NULL;
  m = PyModule_Create(&OpenSCADModule);
  if (m == NULL) return NULL;

  Py_INCREF(&PyOpenSCADType);
  PyModule_AddObject(m, "openscad", (PyObject *)&PyOpenSCADType);
  return m;
}

std::shared_ptr<AbstractNode> python_result_node = NULL;

static PyObject *PyInit_openscad(void)
{
  return PyModule_Create(&OpenSCADModule);
}

static PyObject *pythonInitDict=NULL;

std::string evaluatePython(const std::string & code, double time)
{
  std::string error;
  python_result_node = NULL;
  PyObject *pyExcType;
  PyObject *pyExcValue;
  PyObject *pyExcTraceback;

    if(pythonInitDict) {
      if (Py_FinalizeEx() < 0) {
        exit(120);
      }
      pythonInitDict=NULL;
    }

    if(!pythonInitDict) {
	    char run_str[80];
	    PyImport_AppendInittab("openscad", &PyInit_openscad);
	    PyConfig config;
            PyConfig_InitPythonConfig(&config);
	    wchar_t libdir[256];
	    swprintf(libdir, 256, L"%s/../lib/pylib/",PlatformUtils::applicationPath().c_str());
	    PyConfig_SetString(&config, &config.pythonpath_env, libdir);
	    // Py_Initialize();
            Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);

	    PyObject *py_main = PyImport_AddModule("__main__");
	    pythonInitDict = PyModule_GetDict(py_main);
	    PyInit_PyOpenSCAD();
	    sprintf(run_str,"from openscad import *\nfa=12.0\nfn=0.0\nfs=2.0\nt=%g",time);
	    PyRun_String(run_str, Py_file_input, pythonInitDict, pythonInitDict);
    }
    PyObject *result = PyRun_String(code.c_str(), Py_file_input, pythonInitDict, pythonInitDict);

    PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
    PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);

    PyObject* str_exc_value = PyObject_Repr(pyExcValue);
    PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "~");
    const char *strExcValue =  PyBytes_AS_STRING(pyExcValueStr);
    if(strExcValue != NULL  && !strcmp(strExcValue,"<NULL>")) error="";
    else error=strdup(strExcValue);

    Py_XDECREF(pyExcType);
    Py_XDECREF(pyExcValue);
    Py_XDECREF(pyExcTraceback);

    Py_XDECREF(str_exc_value);
    Py_XDECREF(pyExcValueStr);

    return error;
}

