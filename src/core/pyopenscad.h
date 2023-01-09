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

PyObject* openscad_square(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_circle(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_cylinder(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_sphere(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_rotate(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* openscad_intersection(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* openscad_output(PyObject *self, PyObject *args, PyObject *kwargs);

extern  ModuleInstantiation todo_fix_inst;

void evaluatePython(const char *code);

