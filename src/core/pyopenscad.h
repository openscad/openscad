#include <Python.h>
#include <memory>
#include "node.h"

typedef struct {
    PyObject_HEAD
    std::shared_ptr<AbstractNode> node;
    /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

extern PyTypeObject PyOpenSCADType;

extern std::shared_ptr<AbstractNode> result_node;
extern std::vector<std::shared_ptr<AbstractNode>> node_stack;

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self);
PyObject * PyOpenSCADObject_new2( PyTypeObject *type,std::shared_ptr<AbstractNode> node);

PyObject* openscad_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_cylinder(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_sphere(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_rotate(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_intersection(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_output(PyObject *self, PyObject *args, PyObject *kwargs);

void evaluatePython(const char *code);

