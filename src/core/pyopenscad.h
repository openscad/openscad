#include <Python.h>
#include <memory>
#include "node.h"

typedef struct {
    PyObject_HEAD
    std::shared_ptr<AbstractPolyNode> node;
    /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

extern std::shared_ptr<AbstractPolyNode> result_node;
extern std::vector<std::shared_ptr<AbstractPolyNode>> node_stack;

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self);
PyObject * PyOpenSCADObject_new( std::shared_ptr<AbstractPolyNode> node);

PyObject* openscad_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_output(PyObject *self, PyObject *args, PyObject *kwargs);

