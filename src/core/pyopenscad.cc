#include <Python.h>
#include "pyopenscad.h"

// https://docs.python.it/html/ext/dnt-basics.html

std::shared_ptr<AbstractPolyNode> result_node;
std::vector<std::shared_ptr<AbstractPolyNode>> node_stack;

static PyTypeObject PyOpenSCADType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "PyOpenSCAD",             /* tp_name */
    sizeof(PyOpenSCADObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "PyOpenSCAD objects",           /* tp_doc */
};

static PyModuleDef OpenScadModule = {
    PyModuleDef_HEAD_INIT,
    "PyOpenScadType",
    "Example module that creates an extension type.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_PyOpenSCAD(void)
{
    PyObject* m;

    PyOpenSCADType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyOpenSCADType) < 0)
        return NULL;

    m = PyModule_Create(&OpenScadModule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyOpenSCADType);
    PyModule_AddObject(m, "PyOpenSCADObject", (PyObject *)&PyOpenSCADType);
    return m;
}

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self)
{
    //Py_XDECREF(self->first);
    //Py_XDECREF(self->last);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject * PyOpenSCADObject_new( std::shared_ptr<AbstractPolyNode> node)
{
    PyOpenSCADObject *self;
	printf("a\n");
    self = (PyOpenSCADObject *)malloc(sizeof(PyOpenSCADObject));
    printf("b self=%p %d\n",self,sizeof(PyOpenSCADObject));
    if (self != NULL) {
	self->node=node; // node;
    }
	printf("C\n");
    printf("Self=%p\n",self);
    printf("b\n");
    return (PyObject *)self;
}


