#include <Python.h>
#include <memory>
#include "python_public.h"
#include "geometry/Polygon2d.h"
#include "core/node.h"
#include "core/function.h"
#include "core/ScopeContext.h"
#include "core/UserModule.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

typedef struct {
  PyObject_HEAD std::shared_ptr<AbstractNode> node;
  PyObject *dict;
  /* Type-specific fields go here. */
} PyOpenSCADObject;

void PyObjectDeleter(PyObject *pObject);
using PyObjectUniquePtr = std::unique_ptr<PyObject, decltype(PyObjectDeleter)&>;

PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

extern PyTypeObject PyOpenSCADType;

extern PyObject *python_result_obj;
extern std::vector<SelectedObject> python_result_handle;
extern void python_catch_error(std::string& errorstr);

extern bool python_active;
extern fs::path python_scriptpath;
extern std::string trusted_edit_document_name;
extern std::string untrusted_edit_document_name;
extern std::vector<std::shared_ptr<AbstractNode>> nodes_hold;
extern std::shared_ptr<AbstractNode> void_node, full_node;
bool trust_python_file(const std::string& file, const std::string& content);
PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *object, PyObject **dict);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *object, PyObject **dict);
PyTypeObject *PyOpenSCADObjectType(PyObject *objs);
int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj);
Outline2d python_getprofile(void *cbfunc, int fn, double arg);
double python_doublefunc(void *cbfunc, double arg);
std::shared_ptr<AbstractNode> python_modulefunc(const ModuleInstantiation *module,
                                                const std::shared_ptr<const Context>& context,
                                                std::string& error);
std::vector<int> python_intlistval(PyObject *list);

Value python_functionfunc(const FunctionCall *call, const std::shared_ptr<const Context>& context,
                          int& error);
int python_vectorval(PyObject *vec, int minarg, int maxarg, double *x, double *y, double *z,
                     double *w = NULL, int *flags = nullptr);
std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim, int *dragflags);
int python_numberval(PyObject *number, double *result, int *flags = nullptr, int flagor = 0);
void get_fnas(double& fn, double& fa, double& fs);
void python_retrieve_pyname(const std::shared_ptr<AbstractNode>& node);
void python_build_hashmap(const std::shared_ptr<AbstractNode>& node, int level);
PyObject *python_fromopenscad(const Value& val);

extern SourceFile *osinclude_source;

PyObject *python_str(PyObject *self);

extern PyNumberMethods PyOpenSCADNumbers;
extern PyMappingMethods PyOpenSCADMapping;
extern PyMethodDef PyOpenSCADFunctions[];
extern PyMethodDef PyOpenSCADMethods[];

extern PyObjectUniquePtr pythonInitDict;
extern PyObjectUniquePtr pythonMainModule;
extern int debug_num, debug_cnt;
