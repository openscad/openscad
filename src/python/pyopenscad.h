#include <Python.h>
#include <memory>
#include "python_public.h"
#include "geometry/Polygon2d.h"
#include "core/node.h"
#include "core/function.h"
#include "core/ScopeContext.h"
#include "core/UserModule.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

class CurveDiscretizer;

#define DECLARE_INSTANCE()     \
  std::string instance_name;   \
  AssignmentList inst_asslist; \
  ModuleInstantiation *instance = new ModuleInstantiation(instance_name, inst_asslist, Location::NONE)

typedef struct {
  PyObject_HEAD std::shared_ptr<AbstractNode> node;
  PyObject *dict;
  /* Type-specific fields go here. */
} PyOpenSCADObject;

void PyObjectDeleter(PyObject *pObject);
using PyObjectUniquePtr = std::unique_ptr<PyObject, decltype(PyObjectDeleter)&>;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);
int python_vectorval(PyObject *vec, int minarg, int maxarg, double *x, double *y, double *z,
                     double *w = NULL);
PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *object);

extern PyTypeObject PyOpenSCADType;
extern std::shared_ptr<AbstractNode> python_result_node;
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *object, PyObject **dict);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *object, PyObject **dict);
extern std::string trusted_edit_document_name;
extern std::string untrusted_edit_document_name;
std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim);
int python_numberval(PyObject *number, double *result);
CurveDiscretizer CreateCurveDiscretizer(PyObject *kwargs);
PyObject *python_str(PyObject *self);

extern PyNumberMethods PyOpenSCADNumbers;
extern PyMappingMethods PyOpenSCADMapping;
extern PyMethodDef PyOpenSCADFunctions[];
extern PyMethodDef PyOpenSCADMethods[];

extern PyObjectUniquePtr pythonInitDict;
extern PyObjectUniquePtr pythonMainModule;
