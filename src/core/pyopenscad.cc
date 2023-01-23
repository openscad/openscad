// Author: Sohler Guenther
// Date: 2023-01-01
// Purpose: Extend openscad with an python interpreter

#include <Python.h>
#include "pyopenscad.h"
#include "CsgOpNode.h"

// https://docs.python.it/html/ext/dnt-basics.html

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self)
{
    Py_XDECREF(self->dict);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * PyOpenSCADObject_new(PyTypeObject *type,PyObject *args,  PyObject *kwds)
{
    PyOpenSCADObject *self;
    self = (PyOpenSCADObject *)  type->tp_alloc(type,0);
    self->node=NULL;
    self->dict=PyDict_New();
    Py_XINCREF(self->dict);
    return (PyObject *)self;
}

PyObject * PyOpenSCADObjectFromNode(PyTypeObject *type, std::shared_ptr<AbstractNode> node)
{
    PyOpenSCADObject *self;
    self = (PyOpenSCADObject *)  type->tp_alloc(type,0);
    if (self != NULL) {
	self->node=node;
	self->dict=PyDict_New();
        Py_XINCREF(self->dict);
    }
    Py_XINCREF(self);
    return (PyObject *)self;
}


int python_more_obj(std::vector<std::shared_ptr<AbstractNode>> &children,PyObject *more_obj) {
  int i,n;
  PyObject *obj;
  std::shared_ptr<AbstractNode> child;
  if(PyList_Check(more_obj))
  {
	  n = PyList_Size(more_obj);
	  for(i=0;i<n;i++) {
		obj = PyList_GetItem(more_obj,i);
		child = PyOpenSCADObjectToNode(obj);
	        children.push_back(child);
	  }
   } else if( Py_TYPE(more_obj) == &PyOpenSCADType) {
	child = PyOpenSCADObjectToNode(more_obj);
        children.push_back(child);
   } else return 1;
  return 0;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj)
{
    Py_XDECREF(obj);
    return ((PyOpenSCADObject *) obj)->node;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs)
{
    Py_XDECREF(objs);
   if( Py_TYPE(objs) == &PyOpenSCADType) {
    	return ((PyOpenSCADObject *) objs)->node;
   } else if(PyList_Check(objs)) {
	   // TODO also decref the list ?
	  auto node = std::make_shared<CsgOpNode>(&todo_fix_inst, OpenSCADOperator::UNION);

	  int n = PyList_Size(objs);
	  for(int i=0;i<n;i++) {
		PyObject *obj = PyList_GetItem(objs,i);
		std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj);
		node->children.push_back(child);
	  }
	  return node;
   } else return NULL;
}

int python_numberval(PyObject *number, double *result)
{
	if(PyFloat_Check(number)) {
		*result = PyFloat_AsDouble(number);
		return 0;
	}
	if(PyLong_Check(number)) {
		*result = PyLong_AsLong(number);
		return 0;
	}
	return 1;
}
int python_vectorval(PyObject *vec, double *x, double *y, double *z)
{
  *x=1;
  *y=1;
  *z=1;
  if(PyList_Check(vec)) {
	if(PyList_Size(vec) >= 2) {
		if(python_numberval(PyList_GetItem(vec, 0),x)) return 1;
		if(python_numberval(PyList_GetItem(vec, 1),y)) return 1;
	}
	if(PyList_Check(vec) && PyList_Size(vec) >= 3) {
		if(python_numberval(PyList_GetItem(vec, 2),z)) return 1;
	}
	return 0;
  }
  if(!python_numberval(vec, x))
  {
	*y=*x;
	*z=*x;
	return 0;
  }
  return 1;
}


static int PyOpenSCADInit(PyOpenSCADObject *self, PyObject *arfs,PyObject *kwds)
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
	int n=PyTuple_Size(args);
	PyObject *new_args = PyTuple_New(n+1);
//	Py_INCREF(new_args); // dont decref either,  dont know why its not working

        Py_INCREF(self);
	int ret=PyTuple_SetItem(new_args,0,self);
	item = PyTuple_GetItem(new_args,0);

	for(i=0;i<PyTuple_Size(args);i++)
	{
		item = PyTuple_GetItem(args,i);
		Py_INCREF(item);
		PyTuple_SetItem(new_args,i+1,item);
	}
	return new_args;
}

void get_fnas(double &fn, double &fa, double &fs) {
  PyObject *mainModule = PyImport_AddModule("__main__");
  if(mainModule == NULL) return;
  PyObject *varFn = PyObject_GetAttrString(mainModule, "fn");
  PyObject *varFa = PyObject_GetAttrString(mainModule, "fa");
  PyObject *varFs = PyObject_GetAttrString(mainModule, "fs");
  if(varFn != NULL)  fn = PyFloat_AsDouble(varFn);
  if(varFa != NULL)  fa = PyFloat_AsDouble(varFa);
  if(varFs != NULL)  fs = PyFloat_AsDouble(varFs);
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
	python__getitem__ ,
	python__setitem__
};


PyTypeObject PyOpenSCADType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name			= "PyOpenSCAD", 
    .tp_basicsize		= sizeof(PyOpenSCADObject),
    .tp_dealloc			= (destructor) PyOpenSCADObject_dealloc ,
    .tp_as_number		= &PyOpenSCADNumbers, 
    .tp_as_mapping		= &PyOpenSCADMapping, 
    .tp_flags			= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc			= "PyOpenSCAD objects",
    .tp_methods			= PyOpenSCADMethods,	
    .tp_init			= (initproc) PyOpenSCADInit,
    .tp_new			= PyOpenSCADObject_new
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
    PyObject* m;

    if(PyType_Ready(&PyOpenSCADType) < 0)
	    return NULL;
    m = PyModule_Create(&OpenSCADModule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyOpenSCADType);
    PyModule_AddObject(m, "openscad", (PyObject *)&PyOpenSCADType);
    return m;
}

std::shared_ptr<AbstractNode> python_result_node=NULL;

static PyObject* PyInit_openscad(void)
{
    return PyModule_Create(&OpenSCADModule);
}


std::string todo_fix_name;
AssignmentList todo_fix_asslist;
ModuleInstantiation todo_fix_inst(todo_fix_name,todo_fix_asslist,Location::NONE);

static PyObject *pythonInitDict=NULL;

char *evaluatePython(const char *code)
{
    char *error;
    python_result_node=NULL;
    PyObject *pyExcType;
    PyObject *pyExcValue;
    PyObject *pyExcTraceback;

    if(pythonInitDict) {
    }

    if(!pythonInitDict) {
	    PyImport_AppendInittab("openscad", &PyInit_openscad);
	    Py_Initialize();

	    PyObject *py_main = PyImport_AddModule("__main__");
	    pythonInitDict = PyModule_GetDict(py_main);
	    PyInit_PyOpenSCAD();
	    PyRun_String("from openscad import *\nfa=12.0\nfn=0.0\nfs=2.0\n", Py_file_input, pythonInitDict, pythonInitDict);
    }
    PyObject *result = PyRun_String(code, Py_file_input, pythonInitDict, pythonInitDict);

    PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
    PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);

    PyObject* str_exc_value = PyObject_Repr(pyExcValue);
    PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "~");
    const char *strExcValue =  PyBytes_AS_STRING(pyExcValueStr);
    if(strExcValue != NULL  && !strcmp(strExcValue,"<NULL>")) error=NULL;
    else error=strdup(strExcValue);

    Py_XDECREF(pyExcType);
    Py_XDECREF(pyExcValue);
    Py_XDECREF(pyExcTraceback);

    Py_XDECREF(str_exc_value);
    Py_XDECREF(pyExcValueStr);

/*    // TODO when to clean up python correctly
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
*/    
    return error;
}

