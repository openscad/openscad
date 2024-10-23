#pragma once
#include <Python.h>
#include <memory>
#include "public.h"
#include "node.h"
#include "src/core/function.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

#define DECLARE_INSTANCE	std::string instance_name; \
	AssignmentList inst_asslist;\
	ModuleInstantiation *instance = new ModuleInstantiation(instance_name,inst_asslist, Location::NONE);


typedef struct {
  PyObject_HEAD
  std::shared_ptr<AbstractNode> node;
  PyObject *dict;
  /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

extern PyTypeObject PyOpenSCADType;

extern std::shared_ptr<AbstractNode> python_result_node;

void PyOpenSCADObject_dealloc(PyOpenSCADObject *self);

extern bool python_active;
extern std::string trusted_edit_document_name;
extern std::string untrusted_edit_document_name;
bool trust_python_file(const std::string &file, const std::string &content);
PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode> &node);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *object);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *object);
int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj);

int python_vectorval(PyObject *vec, double *x, double *y, double *z);
int python_numberval(PyObject *number, double *result);
void get_fnas(double& fn, double& fa, double& fs);

PyObject *python_oo_args(PyObject *self, PyObject *args);
PyObject *python_str(PyObject *self);

extern PyNumberMethods PyOpenSCADNumbers;
extern PyMappingMethods PyOpenSCADMapping;
extern PyMethodDef PyOpenSCADFunctions[];
extern PyMethodDef PyOpenSCADMethods[];

extern PyObject *pythonMainModule;
extern PyObject *pythonInitDict;
