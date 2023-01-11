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

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self);

PyObject * PyOpenSCADObjectFromNode( PyTypeObject *type,std::shared_ptr<AbstractNode> node);
std::shared_ptr<AbstractNode> & PyOpenSCADObjectToNode(PyObject *object);

PyObject* python_square(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_circle(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_polygon(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_sphere(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_scale(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_mirror(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_intersection(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_output(PyObject *self, PyObject *args, PyObject *kwargs);

extern  ModuleInstantiation todo_fix_inst;

void evaluatePython(const char *code);

