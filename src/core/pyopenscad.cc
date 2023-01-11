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

static PyMethodDef PyOpenSCADFunctions[] = {
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

PyObject *python_oo_args(PyObject *self, PyObject *args)
{
	int i;
	PyObject *new_args = PyTuple_New(PyTuple_Size(args)+1);
//	Py_INCREF(&new_args);
	PyTuple_SetItem(new_args,0,self);
	for(i=0;i<PyTuple_Size(args);i++)
		PyTuple_SetItem(new_args,i+1,PyTuple_GetItem(args,i));
	return new_args;
}


static PyMethodDef PyOpenSCADMethods[] = {
    {"translate", (PyCFunction) python_translate_oo, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
    {"rotate", (PyCFunction) python_rotate_oo, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
    {"scale", (PyCFunction) python_scale_oo, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
    {"mirror", (PyCFunction) python_mirror_oo, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
    {"multmatrix", (PyCFunction) python_multmatrix_oo, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
    {"linear_extrude", (PyCFunction) python_linear_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
    {"rotate_extrude", (PyCFunction) python_rotate_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},
    {"output", (PyCFunction) python_output_oo, METH_VARARGS | METH_KEYWORDS, "Output the result."},
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
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    0,				/* tp_iter */
    0,				/* tp_iternext */
    PyOpenSCADMethods,		/* tp_methods */
    PyOpenSCADMembers,		/* tp_members */
    0,				/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set*/
    0,				/* tp_dictoffset */
    (initproc) PyOpenSCADInit,	/* tp_init */
    0,				/* tp_alloc*/
    PyOpenSCADObject_new,	/* tp_new */
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

std::shared_ptr<AbstractNode> result_node=NULL;

static PyObject* PyInit_openscad(void)
{
    return PyModule_Create(&OpenSCADModule);
}


std::string todo_fix_name;
AssignmentList todo_fix_asslist;
ModuleInstantiation todo_fix_inst(todo_fix_name,todo_fix_asslist,Location::NONE);

char *evaluatePython(const char *code)
{
    char *error;
    result_node=NULL;
    PyObject *pyExcType;
    PyObject *pyExcValue;
    PyObject *pyExcTraceback;

    PyImport_AppendInittab("openscad", &PyInit_openscad);
    Py_Initialize();

    PyObject *py_main = PyImport_AddModule("__main__");
    PyObject *py_dict = PyModule_GetDict(py_main);
    PyInit_PyOpenSCAD();
    PyRun_String("from openscad import *\n", Py_file_input, py_dict, py_dict);
    PyObject *result = PyRun_String(code, Py_file_input, py_dict, py_dict);

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

    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    return error;
}

