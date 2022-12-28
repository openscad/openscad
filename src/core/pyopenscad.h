#include <Python.h>
#include <memory>
#include "node.h"

typedef struct {
    PyObject_HEAD
    std::shared_ptr<AbstractPolyNode> node;
    /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self);
PyObject * PyOpenSCADObject_new( std::shared_ptr<AbstractPolyNode> node);

PyObject* openscad_cube(PyObject *self, PyObject *args);
PyObject* openscad_output(PyObject *self, PyObject *args);

