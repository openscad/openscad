#include <Python.h>

#include <memory>
#include <string>

#include "core/ScopeContext.h"
#include "core/UserModule.h"
#include "core/function.h"
#include "core/node.h"
#include "geometry/Polygon2d.h"
#include "python_public.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

class CurveDiscretizer;

typedef struct {
  PyObject_HEAD std::shared_ptr<AbstractNode> node;
  PyObject *dict;
  /* Type-specific fields go here. */
} PyOpenSCADObject;

struct PyOpenSCADBoundMemberObject {
  PyObject_HEAD PyObject *scad_self;
  int index;
};

typedef struct {
  PyObject_HEAD double v[3];
} PyOpenSCADVectorObject;

void PyObjectDeleter(PyObject *pObject);
using PyObjectUniquePtr = std::unique_ptr<PyObject, decltype(&PyObjectDeleter)>;

// Encode a Python str object as a UTF-8 std::string and store it in
// ``out``. Returns ``true`` on success.
//
// On failure returns ``false`` with a Python exception set, and the
// caller is expected to propagate it (return ``nullptr`` / ``-1`` /
// etc., or use ``PyErr_ExceptionMatches`` to choose between
// best-effort skipping and propagation):
//
//   * If ``obj`` is not a ``str`` (or is null), raises a ``TypeError``
//     whose message includes ``context`` and the offending type
//     (e.g. ``"export(): expected str, got int"``).
//   * If ``PyUnicode_AsEncodedString`` raises a ``UnicodeError`` (the
//     helper uses the ``"strict"`` error handler so lone surrogates
//     and other unencodable code points reliably error rather than
//     silently substitute U+FFFD), the helper clears it and re-raises
//     as a ``TypeError`` for consistency with the non-str branch.
//   * If a custom utf-8 codec misbehaves and returns a non-``bytes``
//     object (which would make ``PyBytes_AS_STRING`` UB), raises a
//     ``TypeError`` with ``context`` mentioning the codec.
//   * Any other exception from ``PyUnicode_AsEncodedString``
//     (notably ``MemoryError`` or ``KeyboardInterrupt`` from a custom
//     codec hook) is propagated *verbatim* -- the helper does not
//     downgrade it to a ``TypeError``. Callers in ``void`` contexts
//     that cannot propagate must therefore call ``PyErr_Clear()``
//     themselves, or be restructured to return a status.
//
// On success ``out`` holds the UTF-8 bytes and the intermediate
// bytes object is released, so this is a leak-free replacement for
// the broken ``PyUnicode_AsEncodedString`` + ``PyBytes_AS_STRING`` +
// check-the-wrong-pointer idiom that used to be sprinkled across
// pyfunctions.cc (see issue #587).
bool python_pyobject_to_utf8(PyObject *obj, std::string& out, const char *context);

// Convenience factory: build a PyObjectUniquePtr without having to
// repeat `&PyObjectDeleter` at every call site. Mostly used to wrap
// `*dict` out-parameters from PyOpenSCADObjectToNode[Multi] (see the
// ownership note on those functions below).
inline PyObjectUniquePtr py_owned(PyObject *p = nullptr)
{
  return PyObjectUniquePtr(p, &PyObjectDeleter);
}

// Helper for the `child == nullptr` failure branch of
// PyOpenSCADObjectToNode[Multi] callers. Since that function may now
// return nullptr with a Python exception ALREADY SET (e.g.
// MemoryError from the dict-merge path -- see #596), unconditionally
// calling PyErr_SetString in the failure path would clobber the
// original exception. This helper preserves any pending exception
// and only synthesizes a fresh TypeError when none is set. Always
// returns NULL so callers can `return propagate_or_typeerror(msg);`.
inline PyObject *propagate_or_typeerror(const char *msg)
{
  if (!PyErr_Occurred()) PyErr_SetString(PyExc_TypeError, msg);
  return NULL;
}

// Sole init entry point for the `_openscad` extension module.  Used
// both when the module is embedded in the GUI/CLI (via
// `PyImport_AppendInittab("_openscad", ...)`) and when CPython loads the
// pip-built shared library (which dlsym's `PyInit__openscad`).
//
// Use `PyMODINIT_FUNC` (not bare `extern "C" PyObject *`) so that on
// Windows the symbol gets `__declspec(dllexport)` via Py_EXPORTED_SYMBOL;
// without it the loader cannot find `PyInit__openscad` in the `.pyd`.
// `PyMODINIT_FUNC` already implies `extern "C"`.
// NOLINTBEGIN(bugprone-reserved-identifier)
PyMODINIT_FUNC PyInit__openscad(void);
// NOLINTEND(bugprone-reserved-identifier)

extern PyTypeObject PyOpenSCADType;
extern PyTypeObject PyOpenSCADVectorType;
extern PyTypeObject PyOpenSCADBoundMemberType;
extern PyObject *python_result_obj;
extern std::vector<SelectedObject> python_result_handle;
extern void python_catch_error(std::string& errorstr);

extern bool python_active;
extern fs::path python_scriptpath;
extern std::string trusted_edit_document_name;
extern std::string untrusted_edit_document_name;
extern std::vector<std::shared_ptr<AbstractNode>> nodes_hold;
extern std::shared_ptr<AbstractNode> void_node, full_node;
extern std::vector<PyObject *> python_member_callables;
extern std::vector<std::string> python_member_names;
bool trust_python_file(const std::string& file, const std::string& content);
PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node);

// Both PyOpenSCADObjectToNode and PyOpenSCADObjectToNodeMulti set
// `*dict` to either nullptr or a NEW STRONG REFERENCE that the caller
// must Py_XDECREF (or hand to a PyObjectUniquePtr) when done with it.
// Callers that don't need the dict must still Py_XDECREF the
// out-parameter to avoid a per-call leak; using a PyObjectUniquePtr
// is the recommended pattern. See issue #596 for the historical
// borrow-vs-own asymmetry that this contract replaces.
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj, PyObject **dict);
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs, PyObject **dict);
PyTypeObject *PyOpenSCADObjectType(PyObject *objs);
int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj);
Outline2d python_getprofile(void *v_cbfunc, int fn, double arg);
double python_doublefunc(void *v_cbfunc, double arg);
std::shared_ptr<AbstractNode> python_modulefunc(const std::shared_ptr<const ModuleInstantiation>& module,
                                                const std::shared_ptr<const Context>& context,
                                                std::string& error);
std::vector<int> python_intlistval(PyObject *list);
// True for list/tuple/NumPy-array-like objects (anything supporting the
// sequence protocol) but not str/bytes/dict/PyOpenSCAD objects. Use in place
// of PyList_Check() when parsing user coordinate/index input so NumPy arrays
// and tuples are accepted alongside plain lists.
bool python_is_sequence(PyObject *o);

Value python_functionfunc(const FunctionCall *call, const std::shared_ptr<const Context>& context,
                          int& error);
int python_vectorval(PyObject *vec, int minarg, int maxarg, double *x, double *y, double *z,
                     double *w = NULL, int *flags = nullptr);
std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim, int *dragflags);
int python_numberval(PyObject *number, double *result, int *flags = nullptr, int flagor = 0);
void get_fnas(double& fn, double& fa, double& fs);
void python_retrieve_pyname(const std::shared_ptr<AbstractNode>& node);
// Walk ``node`` (and its subtree, up to a fixed recursion depth) and
// register triples in the global ``mapping_*`` arrays for every
// Python global that points at this node:
//   - ``mapping_name``  -- the ``__main__`` dict key (UTF-8)
//   - ``mapping_code``  -- a stringified node-tree dump (currently
//                          empty: the ``python_hierdump`` call is
//                          commented out)
//   - ``mapping_level`` -- the ``PyDict_Next`` iteration position at
//                          which the binding was found, *not* the
//                          recursion depth. The variable is
//                          historically misnamed; see issue tracker
//                          for the rename.
// Best-effort -- non-str ``__main__`` keys are silently skipped.
//
// Returns ``true`` on success. Returns ``false`` with a Python
// exception set if the helper raises something we cannot reasonably
// downgrade to "skip this entry" (notably ``MemoryError`` or
// ``KeyboardInterrupt``). Callers must check the return value and
// propagate if false.
bool python_build_hashmap(const std::shared_ptr<AbstractNode>& node, int level);
PyObject *python_fromopenscad(const Value& val);

extern SourceFile *osinclude_source;

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
extern int debug_num, debug_cnt;
