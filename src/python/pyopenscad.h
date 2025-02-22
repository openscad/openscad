#include <Python.h>
#include <memory>
#include "python_public.h"
#include "src/core/node.h"
#include <geometry/Polygon2d.h>
#include "src/core/function.h"
#include "src/core/ScopeContext.h"
#include "src/core/UserModule.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

#define DECLARE_INSTANCE	std::string instance_name; \
	AssignmentList inst_asslist;\
	ModuleInstantiation *instance = new ModuleInstantiation(instance_name,inst_asslist, Location::NONE); 


typedef struct {
  PyObject_HEAD
  std::shared_ptr<AbstractNode> node;
  /* Type-specific fields go here. */
} PyOpenSCADObject;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);
int python_vectorval(PyObject *vec, int minarg, int maxarg, double *x, double *y, double *z, double *w=NULL);
PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode> &node);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *object);


extern PyTypeObject PyOpenSCADType;
extern std::shared_ptr<AbstractNode> python_result_node;
extern std::string trusted_edit_document_name;
extern std::string untrusted_edit_document_name;
extern PyMethodDef PyOpenSCADFunctions[];
extern PyObject *pythonInitDict;
extern PyObject *pythonMainModule;
