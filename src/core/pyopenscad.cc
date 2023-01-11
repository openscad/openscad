// Author: Sohler Guenther
// Date: 2023-01-01
// Purpose: Extend openscad with an python interpreter
#include <Python.h>
#include <structmember.h>
#include "pyopenscad.h"

// https://docs.python.it/html/ext/dnt-basics.html

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self)
{
    //Py_XDECREF(self->first);
    //Py_XDECREF(self->last);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject * PyOpenSCADObject_new(PyTypeObject *type,PyObject *args,  PyObject *kwds)
{
    PyOpenSCADObject *self;
    self = (PyOpenSCADObject *)  type->tp_alloc(type,0);
    return (PyObject *)self;
}

PyObject * PyOpenSCADObjectFromNode(PyTypeObject *type, std::shared_ptr<AbstractNode> node)
{
    PyOpenSCADObject *self;
    self = (PyOpenSCADObject *)  type->tp_alloc(type,0);
    if (self != NULL) {
	self->node=node;
    }
//    Py_XINCREF(self);
    return (PyObject *)self;
}


std::shared_ptr<AbstractNode> &PyOpenSCADObjectToNode(PyObject *obj)
{
//    Py_XDECREF(obj);
    return ((PyOpenSCADObject *) obj)->node;
}


static int PyOpenSCADInit(PyOpenSCADObject *self, PyObject *arfs,PyObject *kwds)
{
	return 0;
}


static PyMemberDef PyOpenSCADMembers[] = {
	{NULL}
};

static PyMethodDef PyOpenSCADMethods[] = {
    {"square", (PyCFunction) python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
    {"circle", (PyCFunction) python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
    {"polygon", (PyCFunction) python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},

    {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
    {"cylinder", (PyCFunction) python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
    {"sphere", (PyCFunction) python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
    {"polyhedron", (PyCFunction) python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},

    {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
    {"rotate", (PyCFunction) python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
    {"scale", (PyCFunction) python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
    {"mirror", (PyCFunction) python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
    {"multmatrix", (PyCFunction) python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},

    {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
    {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},

    {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
    {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
    {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},

    {"output", (PyCFunction) python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},
    {NULL, NULL, 0, NULL}
};


PyTypeObject PyOpenSCADType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "PyOpenSCAD",             /* tp_name */
    sizeof(PyOpenSCADObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) PyOpenSCADObject_dealloc ,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "PyOpenSCAD objects",           /* tp_doc */
    0,
    0,
    0,
    0,
    0,
    0,
    PyOpenSCADMethods,
    PyOpenSCADMembers,
    0,
    0,
    0,
    0,
    0,
    0,
    (initproc) PyOpenSCADInit,
    0,
    PyOpenSCADObject_new,
};


static PyModuleDef OpenSCADModule = {
    PyModuleDef_HEAD_INIT,
    "openscad",
    "Example module that creates an extension type.",
    -1,
    PyOpenSCADMethods,
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

std::shared_ptr<AbstractNode> result_node=NULL;

static PyObject* PyInit_openscad(void)
{
    return PyModule_Create(&OpenSCADModule);
}


std::string todo_fix_name;
AssignmentList todo_fix_asslist;
ModuleInstantiation todo_fix_inst(todo_fix_name,todo_fix_asslist,Location::NONE);

void evaluatePython(const char *code)
{
    result_node=NULL;
    wchar_t *program = Py_DecodeLocale("openscad", NULL);
    const char *prg="from time import time,ctime\n"
                       "print('Today is', ctime(time()))\n";
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
//    Py_SetProgramName(program);  /* optional but recommended /

    PyImport_AppendInittab("openscad", &PyInit_openscad);

    Py_Initialize();
    PyInit_PyOpenSCAD();
    PyRun_SimpleString("from openscad import *\n");
    PyRun_SimpleString(code);
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    PyMem_RawFree(program);
}

