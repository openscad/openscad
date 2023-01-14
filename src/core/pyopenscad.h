#include <Python.h>
#include <memory>
#include "node.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

typedef struct {
    PyObject_HEAD
    std::shared_ptr<AbstractNode> node;
    /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

extern PyTypeObject PyOpenSCADType;

extern std::shared_ptr<AbstractNode> python_result_node;

void PyOpenSCADObject_dealloc(PyOpenSCADObject * self);

PyObject * PyOpenSCADObjectFromNode( PyTypeObject *type,std::shared_ptr<AbstractNode> node);
std::shared_ptr<AbstractNode> & PyOpenSCADObjectToNode(PyObject *object);

PyObject* python_square(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_circle(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_polygon(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_text(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_textmetrics(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_sphere(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_text(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_translate_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_scale(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_scale_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_mirror(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_mirror_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_multmatrix_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_offset(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_offset_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_roof(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_roof_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_color(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_color_oo(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_linear_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_rotate_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_intersection(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_hull(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_fill(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_resize(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_render(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_group(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_projection(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_surface(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_import(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_version(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_version_num(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject* python_output(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject* python_output_oo(PyObject *self, PyObject *args, PyObject *kwargs);


PyObject *python_oo_args(PyObject *self, PyObject *args);

void get_fnas(double &fn, double &fa, double &fs);


extern  ModuleInstantiation todo_fix_inst;

char *evaluatePython(const char *code);

