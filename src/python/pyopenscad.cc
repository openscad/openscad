/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <Python.h>
#include "genlang/genlang.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#ifdef _WIN32
// AttachConsole / GetStdHandle / _wfreopen for the --repl/--ipython
// console reattach dance (see windows_reattach_console_for_repl below).
// NOGDI suppresses windows.h's GDI Polygon() function declaration, which
// would otherwise collide with the project's `using Polygon = std::vector<...>`
// alias from src/geometry/GeometryUtils.h. (OpenSCADLibInternal sets NOGDI
// at the target level; OpenSCADPy doesn't, so we define it locally here.)
// WIN32_LEAN_AND_MEAN trims a large chunk of unrelated win32 surface area.
#ifndef NOGDI
#define NOGDI
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "pyopenscad.h"
#include "pydata.h"
#include "core/CsgOpNode.h"
#include "Value.h"
#ifndef PYTHON_EXECUTABLE_NAME
#include "executable.h"
#endif
#include "Expression.h"
#include "PlatformUtils.h"
#include <Context.h>
#include <Selection.h>
#include "core/CurveDiscretizer.h"
#include "core/enums.h"
#include "core/node.h"
#include "platform/PlatformUtils.h"
#include "primitives.h"
namespace fs = std::filesystem;

// #define HAVE_PYTHON_YIELD
// CPython requires the init function for the `_openscad` extension module to
// be named `PyInit__openscad` (double underscore). The reserved-identifier
// warning is therefore unavoidable here. `PyMODINIT_FUNC` (rather than a
// bare `extern "C" PyObject *`) is required so that on Windows the symbol
// is decorated with `__declspec(dllexport)` via Py_EXPORTED_SYMBOL and
// CPython's loader can find it inside the `.pyd`.
// NOLINTBEGIN(bugprone-reserved-identifier)
PyMODINIT_FUNC PyInit__openscad(void);
// NOLINTEND(bugprone-reserved-identifier)

bool python_active;
bool python_trusted;
fs::path python_scriptpath;
// https://docs.python.org/3.10/extending/newtypes.html

void PyObjectDeleter(PyObject *pObject)
{
  if (pObject == nullptr) {
    return;
  }
  // C++ static destructors can run after Py_FinalizeEx; DECREF is undefined then (Python 3.12+ aborts).
  if (!Py_IsInitialized()) {
    return;
  }
  Py_DECREF(pObject);
};

PyObjectUniquePtr pythonInitDict(nullptr, &PyObjectDeleter);
PyObjectUniquePtr pythonMainModule(nullptr, &PyObjectDeleter);

bool python_pyobject_to_utf8(PyObject *obj, std::string& out, const char *context)
{
  if (obj == nullptr || !PyUnicode_Check(obj)) {
    /* PyUnicode_Check is exception-free, so don't pre-clear: a stale
     * exception (if any) is a caller bug we shouldn't mask. ``%S`` on
     * the type would print ``<class 'int'>``; ``tp_name`` gives the
     * cleaner ``int`` form. ``PyErr_Format`` overwrites whatever the
     * current exception was, so we don't have to chain manually. */
    PyObject *type = obj == nullptr ? nullptr : (PyObject *)Py_TYPE(obj);
    PyErr_Format(PyExc_TypeError, "%s: expected str, got %s", context,
                 type == nullptr ? "NULL" : ((PyTypeObject *)type)->tp_name);
    return false;
  }
  /* Use ``"strict"`` so unencodable code points (e.g. lone surrogates
   * in a "str" produced by surrogateescape decoding) reliably raise a
   * UnicodeEncodeError that we can convert into the documented
   * TypeError. The ``"replace"`` handler would silently substitute
   * U+FFFD instead, which is dangerous for dict keys: two distinct
   * surrogate-bearing keys would both become "?...?" and collide on
   * lookup, and the user-visible part name in a 3MF or the OBJ
   * attribute would be a corrupted approximation of what they passed.
   * The pre-existing call sites this helper replaces all used ``"~"``,
   * which is not a registered error handler at all -- so on surrogate
   * input they were already raising ``LookupError`` by accident, just
   * by luck never on a dict key in the wild. */
  PyObjectUniquePtr bytes(PyUnicode_AsEncodedString(obj, "utf-8", "strict"), &PyObjectDeleter);
  if (bytes.get() == nullptr) {
    if (PyErr_Occurred() != nullptr && !PyErr_ExceptionMatches(PyExc_UnicodeError)) {
      /* MemoryError, KeyboardInterrupt, ImportError from a custom
       * codec hook, ... -- propagate verbatim. */
      return false;
    }
    PyErr_Clear();
    PyErr_Format(PyExc_TypeError, "%s: str cannot be UTF-8 encoded", context);
    return false;
  }
  /* ``PyBytes_AS_STRING`` and ``PyBytes_GET_SIZE`` are unchecked
   * macros that assume a real ``bytes`` instance; if a custom utf-8
   * codec misbehaves and returns a non-``bytes`` (or ``bytearray``,
   * or some other buffer type), the macros would interpret the
   * object's memory as ``PyBytesObject`` and we'd be reading garbage
   * (or segfaulting). Guard explicitly. */
  if (!PyBytes_Check(bytes.get())) {
    PyErr_Format(PyExc_TypeError, "%s: utf-8 codec returned non-bytes", context);
    return false;
  }
  out.assign(PyBytes_AS_STRING(bytes.get()), PyBytes_GET_SIZE(bytes.get()));
  return true;
}

static std::atomic<bool> openscad_py_atexit_registered{false};

static void openscad_release_static_py_refs(void)
{
  if (!Py_IsInitialized()) {
    return;
  }
  const PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *const d = pythonInitDict.release();
  PyObject *const m = pythonMainModule.release();
  Py_XDECREF(d);
  Py_XDECREF(m);
  PyGILState_Release(gstate);
}

static void register_openscad_py_atexit(void)
{
  if (openscad_py_atexit_registered.exchange(true)) {
    return;
  }
  if (Py_AtExit(openscad_release_static_py_refs) != 0) {
    openscad_py_atexit_registered = false;
  }
}
std::list<std::string> pythonInventory;
AssignmentList customizer_parameters;
AssignmentList customizer_parameters_finished;
bool pythonDryRun = false;
PyObject *python_result_obj = nullptr;
std::vector<SelectedObject> python_result_handle;
bool python_runipython = false;
bool python_runrepl = false;
std::vector<std::string> python_replargs;
bool pythonMainModuleInitialized = false;
bool pythonRuntimeInitialized = false;

std::vector<std::shared_ptr<AbstractNode>> nodes_hold;  // make sure, that these nodes are not yet freed
std::shared_ptr<AbstractNode> void_node, full_node;

void PyOpenSCADObject_dealloc(PyOpenSCADObject *self)
{
  Py_XDECREF(self->dict);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject *>(self));
}

/*
 *  allocates a new PyOpenSCAD Object including its internal dictionary
 */

PyObject *PyOpenSCADObject_alloc(PyTypeObject *cls, Py_ssize_t nitems)
{
  PyOpenSCADObject *self = reinterpret_cast<PyOpenSCADObject *>(PyType_GenericAlloc(cls, nitems));
  self->dict = PyDict_New();
  PyObject *origin = PyList_New(4);
  for (int i = 0; i < 4; i++) {
    PyObject *row = PyList_New(4);
    for (int j = 0; j < 4; j++) PyList_SetItem(row, j, PyFloat_FromDouble(i == j ? 1.0 : 0.0));
    PyList_SetItem(origin, i, row);
  }
  PyDict_SetItemString(self->dict, "origin", origin);
  Py_XDECREF(origin);
  return (PyObject *)self;
}

static PyObject *PyOpenSCADObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  return PyOpenSCADObject_alloc(type, 0);
}

/*
 *  allocates a new PyOpenSCAD to store an existing OpenSCAD Abstract Node
 */

PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node)
{
  PyOpenSCADObject *self;
  self = reinterpret_cast<PyOpenSCADObject *>(type->tp_new(type, nullptr, nullptr));
  if (self != nullptr) {
    Py_XINCREF(self);
    self->node = node;
    return (PyObject *)self;
  }
  return nullptr;
}

// PyGILState_STATE gstate=PyGILState_LOCKED;
PyThreadState *tstate = nullptr;

void python_lock(void)
{
  // #ifndef _WIN32
  if (tstate != nullptr && pythonInitDict != nullptr) PyEval_RestoreThread(tstate);
  // #endif
}

void python_unlock(void)
{
  // #ifndef _WIN32
  if (pythonInitDict != nullptr) tstate = PyEval_SaveThread();
  // #endif
}

/*
 *  extracts Absrtract Node from PyOpenSCAD Object
 */

// `*dict` ownership contract (post-fix):
//
// On every return path (success or failure, every input shape) `*dict`
// is set to either `nullptr` or a NEW STRONG REFERENCE that the caller
// owns and MUST `Py_XDECREF` (or hand to a `PyObjectUniquePtr`) when
// done. The borrow-vs-own asymmetry that historically existed -- a
// borrowed ref to `obj->dict` for instance inputs but a freshly-
// allocated dict for list inputs -- silently leaked one dict per
// list-input call (see issue #596). Standardising on "always strong"
// makes the contract uniform; the per-input-shape leak goes away
// automatically once every call site decrefs.
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj, PyObject **dict)
{
  *dict = nullptr;

  if (obj == Py_None || obj == Py_False) {
    return void_node;
  }
  if (obj == Py_True) {
    return full_node;
  }

  if (!PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    return void_node;
  }

  std::shared_ptr<AbstractNode> result = (reinterpret_cast<PyOpenSCADObject *>(obj))->node;
  if (result != nullptr) {
    if (result.use_count() > 2 && result != void_node && result != full_node) {
      result = result->clone();
    }
    PyObject *instance_dict = (reinterpret_cast<PyOpenSCADObject *>(obj))->dict;
    Py_XINCREF(instance_dict);
    *dict = instance_dict;
  } else {
    result = nullptr;
  }
  return result;
}

std::string python_version(void)
{
  std::ostringstream stream;
  stream << "Python " << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << "." << PY_MICRO_VERSION;
  return stream.str();
}

/*
 * Lookup a PyOpenSCADType compatible type from a source object
 */

PyTypeObject *PyOpenSCADObjectType(PyObject *objs)
{
  if (PyObject_IsInstance(objs, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    return Py_TYPE(objs);
  } else if (PyList_Check(objs)) {
    int n = PyList_Size(objs);
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      if (PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
        return Py_TYPE(obj);
      }
    }
  }
  return &PyOpenSCADType;
}

/*
 * same as  python_more_obj but always returns only one AbstractNode by creating an UNION operation
 */

// `*dict` ownership contract: same as PyOpenSCADObjectToNode -- always
// `nullptr` or a NEW STRONG REFERENCE the caller must `Py_XDECREF`.
// See the comment on PyOpenSCADObjectToNode above for the rationale
// and the pre-fix leak behavior (issue #596).
std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs, PyObject **dict)
{
  *dict = nullptr;
  std::shared_ptr<AbstractNode> result = nullptr;
  if (PyObject_IsInstance(objs, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    result = (reinterpret_cast<PyOpenSCADObject *>(objs))->node;
    if (result.use_count() > 2 && result != void_node && result != full_node) {
      result = result->clone();
    }
    PyObject *instance_dict = (reinterpret_cast<PyOpenSCADObject *>(objs))->dict;
    Py_XINCREF(instance_dict);
    *dict = instance_dict;
  } else if (PyList_Check(objs)) {
    DECLARE_INSTANCE();
    auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    int n = PyList_Size(objs);
    // Each entry is a STRONG ref returned by PyOpenSCADObjectToNode;
    // we must Py_XDECREF every entry on every exit path. Wrapping in
    // PyObjectUniquePtr so this is impossible to forget regardless of
    // which return path is taken (early `return nullptr` on a
    // non-PyOpenSCAD list element, or the merge fall-through below).
    std::vector<PyObjectUniquePtr> child_dict;
    child_dict.reserve(n);
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      if (PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
        PyObject *subdict = nullptr;
        std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &subdict);
        auto owned_subdict = py_owned(subdict);
        if (child == nullptr) continue;
        node->children.push_back(child);
        child_dict.push_back(std::move(owned_subdict));
      } else {
        // Early return: child_dict's destructor decrements every
        // strong ref accumulated so far, so no leak on this path.
        return nullptr;
      }
    }
    result = node;

    PyObject *merged = PyDict_New();
    if (merged == nullptr) {
      // PyDict_New sets a Python exception (typically MemoryError) on
      // failure. Returning a valid node here would leave the exception
      // pending and surface as a SystemError later. Returning nullptr
      // -- with *dict already nullptr -- propagates the exception
      // cleanly; child_dict cleans up its accumulated strong refs on
      // scope exit.
      return nullptr;
    }
    // Reverse-iterate: 1st child wins (later writes overwrite earlier
    // ones). Use the `i-- > 0` pattern with size_t to avoid the
    // implementation-defined size_t-to-int conversion that the prior
    // `int i = child_dict.size() - 1` formulation triggered when
    // child_dict was empty.
    for (size_t i = child_dict.size(); i-- > 0;) {
      PyObject *subsubdict = child_dict[i].get();
      if (subsubdict == nullptr) continue;
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(subsubdict, &pos, &key, &value)) {
        if (PyDict_SetItem(merged, key, value) < 0) {
          // Propagate the pending exception (typically MemoryError, or
          // the rare hashing-side error). PyDict_New's strong ref is
          // dropped via py_owned() so we don't leak `merged`, and
          // *dict stays nullptr so the caller never sees a partially
          // merged dict.
          auto owned_merged = py_owned(merged);
          return nullptr;
        }
      }
    }
    *dict = merged;
  } else if (objs == Py_None || objs == Py_False) {
    result = void_node;
  } else if (objs == Py_True) {
    result = full_node;
  } else {
    result = nullptr;
  }
  return result;
}

void python_hierdump(std::ostringstream& stream, const std::shared_ptr<AbstractNode>& node)
{
  if (node == nullptr) return;
  stream << node->toString();
  auto children = node->getChildren();
  if (children.size() < 1) stream << ";";
  else if (children.size() < 2) python_hierdump(stream, children[0]);
  else {
    stream << "{ ";
    for (unsigned int i = 0; i < children.size(); i++) python_hierdump(stream, children[i]);
    stream << " }";
  }
}
bool python_build_hashmap(const std::shared_ptr<AbstractNode>& node, int level)
{
  PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  std::ostringstream stream;
  //  python_hierdump(stream, node);
  std::string code = stream.str();
  while (PyDict_Next(maindict, &pos, &key, &value)) {
    if (!PyObject_IsInstance(value, reinterpret_cast<PyObject *>(&PyOpenSCADType))) continue;
    std::shared_ptr<AbstractNode> testnode = (reinterpret_cast<PyOpenSCADObject *>(value))->node;
    if (testnode != node) continue;
    std::string key_str;
    if (!python_pyobject_to_utf8(key, key_str, "__main__ key")) {
      /* The hashmap is a best-effort lookup table for selection
       * handles; non-str keys in __main__ (rare but legal) just
       * shouldn't appear in it. Skip those (clear the helper's
       * TypeError). Anything else (MemoryError, KeyboardInterrupt
       * from a custom codec hook, ...) we propagate by leaving the
       * exception set and returning ``false`` -- the caller
       * (python_show_core et al.) is expected to bubble out via
       * ``return nullptr``. */
      if (!PyErr_ExceptionMatches(PyExc_TypeError)) return false;
      PyErr_Clear();
      continue;
    }
    mapping_name.push_back(key_str);
    mapping_code.push_back(code);
    mapping_level.push_back(pos);
  }
  if (level < 5) {  // no  many level are unclear and error prone(overwrites memory)
    for (const auto& child : node->getChildren()) {
      if (!python_build_hashmap(child, level + 1)) return false;
    }
  }
  return true;
}

void python_retrieve_pyname(const std::shared_ptr<AbstractNode>& node)
{
  /*
  std::string name;
  int level=-1;
  std::ostringstream stream;
  python_hierdump(stream, node);
  std::string my_code = stream.str();
  for(unsigned int i=0;i<mapping_code.size();i++) {
    if(mapping_code[i] == my_code) {
      if(level == -1 || level > mapping_level[i]) {
        name=mapping_name[i];
        level=mapping_level[i];
      }
    }
  }
  node->setPyName(name);
  */
}
/**
 * Create a CurveDiscretizer by extracting parameters from __main__ and kwargs
 * @param kwargs *Remove* any control parameter arguments found.
 */

CurveDiscretizer CreateCurveDiscretizer(PyObject *kwargs)
{
  PyObject *mainModule = pythonMainModule.get();
  return CurveDiscretizer([kwargs, mainModule](const char *key) -> std::optional<double> {
    double result;
    if (kwargs != nullptr && PyDict_Check(kwargs)) {  // kwargs can be nullptr
      if (PyObject *value = PyDict_GetItemString(kwargs, key); value != nullptr) {
        // PyArg_ParseTupleAndKeywords does not allow unspecified keyword args.
        PyDict_DelItemString(kwargs, key);
        if (!(python_numberval(value, &result, nullptr, 0)))
          return result;  // value can be Integer, Number, ...
      }
    }
    if (mainModule != nullptr) {
      if (PyObject_HasAttrString(mainModule, key)) {
        PyObjectUniquePtr var(PyObject_GetAttrString(mainModule, key), &PyObjectDeleter);
        if (var.get() != nullptr) {
          if (!(python_numberval(var.get(), &result, nullptr, 0))) return result;
        }
      }
    }
    return {};
  });
}

void get_fnas(double& fn, double& fa, double& fs)
{
  PyObject *mainModule = PyImport_AddModule("__main__");
  if (mainModule == nullptr) return;
  fn = 0;
  fa = 12;
  fs = 2;

  if (PyObject_HasAttrString(mainModule, "fn")) {
    PyObjectUniquePtr varFn(PyObject_GetAttrString(mainModule, "fn"), &PyObjectDeleter);
    if (varFn.get() != nullptr) {
      fn = PyFloat_AsDouble(varFn.get());
    }
  }

  if (PyObject_HasAttrString(mainModule, "fa")) {
    PyObjectUniquePtr varFa(PyObject_GetAttrString(mainModule, "fa"), &PyObjectDeleter);
    if (varFa.get() != nullptr) {
      fa = PyFloat_AsDouble(varFa.get());
    }
  }

  if (PyObject_HasAttrString(mainModule, "fs")) {
    PyObjectUniquePtr varFs(PyObject_GetAttrString(mainModule, "fs"), &PyObjectDeleter);
    if (varFs.get() != nullptr) {
      fs = PyFloat_AsDouble(varFs.get());
    }
  }
}

/*
 * Type specific init function. nothing special here
 */

static int PyOpenSCADInit(PyOpenSCADObject *self, PyObject *args, PyObject *kwds)
{
  (void)self;
  (void)args;
  (void)kwds;
  return 0;
}
Outline2d python_getprofile(void *v_cbfunc, int fn, double arg)
{
  PyObject *cbfunc = reinterpret_cast<PyObject *>(v_cbfunc);
  Outline2d result;
  if (pythonInitDict == nullptr) initPython(PlatformUtils::applicationPath(), "", nullptr);
  PyObject *args = PyTuple_Pack(1, PyFloat_FromDouble(arg));
  PyObject *polygon = PyObject_CallObject(cbfunc, args);
  Py_XDECREF(args);
  if (polygon == nullptr) {  // TODO fix
    for (unsigned int i = 0; i < (unsigned int)fn; i++) {
      double ang = 360.0 * (i / (double)fn);
      PyObject *args1 = PyTuple_Pack(2, PyFloat_FromDouble(arg), PyFloat_FromDouble(ang));
      Py_XINCREF(args1);
      PyObject *pypt = PyObject_CallObject(cbfunc, args1);
      double r = PyFloat_AsDouble(pypt);
      if (r < 0) r = -r;  // TODO who the hell knows, why this is needed
      double ang1 = ang * 3.1415 / 180.0;
      double x = r * cos(ang1);
      double y = r * sin(ang1);
      result.vertices.push_back(Vector2d(x, y));
    }
  } else if (PyList_Check(polygon)) {
    unsigned int n = PyList_Size(polygon);
    for (unsigned int i = 0; i < n; i++) {
      PyObject *pypt = PyList_GetItem(polygon, i);
      if (PyList_Check(pypt) && PyList_Size(pypt) == 2) {
        double x = PyFloat_AsDouble(PyList_GetItem(pypt, 0));
        double y = PyFloat_AsDouble(PyList_GetItem(pypt, 1));
        result.vertices.push_back(Vector2d(x, y));
      }
    }
  }
  if (result.vertices.size() < 3) {
    Outline2d err;
    err.vertices.push_back(Vector2d(0, 0));
    err.vertices.push_back(Vector2d(10, 0));
    err.vertices.push_back(Vector2d(10, 10));
    return err;
  }
  return result;
}

double python_doublefunc(void *v_cbfunc, double arg)
{
  PyObject *cbfunc = reinterpret_cast<PyObject *>(v_cbfunc);
  double result = 0;
  PyObject *args = PyTuple_Pack(1, PyFloat_FromDouble(arg));
  PyObject *funcresult = PyObject_CallObject(cbfunc, args);
  Py_XDECREF(args);
  if (funcresult) result = PyFloat_AsDouble(funcresult);
  return result;
}

/*
 * Try to call a python function by name using OpenSCAD module childs and OpenSCAD function arguments:
 * argument order is childs, arguments
 */
PyObject *python_fromopenscad(const Value& val)
{
  switch (val.type()) {
  case Value::Type::UNDEFINED: Py_RETURN_NONE;
  case Value::Type::BOOL:
    if (val.toBool()) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
  case Value::Type::NUMBER: return PyFloat_FromDouble(val.toDouble());
  case Value::Type::STRING: return PyUnicode_FromString(val.toString().c_str());
  case Value::Type::VECTOR: {
    const VectorType& vec = val.toVector();
    PyObject *result = PyList_New(vec.size());
    for (size_t j = 0; j < vec.size(); j++) PyList_SetItem(result, j, python_fromopenscad(vec[j]));
    return result;
  }
    // TODO  more types RANGE, OBJECT, FUNCTION
  default: Py_RETURN_NONE;
  }
  Py_RETURN_NONE;
}
void python_catch_error(std::string& errorstr)
{
  PyObject *pyExcType;
  PyObject *pyExcValue;
  PyObject *pyExcTraceback;
  PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
  PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);
  if (pyExcType != nullptr) Py_XDECREF(pyExcType);

  if (pyExcValue != nullptr) {
    PyObjectUniquePtr str_exc_value(PyObject_Repr(pyExcValue), &PyObjectDeleter);
    /* Best-effort: this is a void error-formatter, so we cannot
     * propagate a helper failure to a caller. Clear any exception
     * the helper sets (e.g. TypeError from a repr() containing lone
     * surrogates) and degrade silently to no-append.
     *
     * Same treatment for a NULL repr -- PyObject_Repr() also leaves
     * an exception set on failure, and we don't want to poison the
     * next unrelated C-API call from the GUI. */
    if (str_exc_value.get() != nullptr) {
      std::string suberror;
      if (python_pyobject_to_utf8(str_exc_value.get(), suberror, "python_catch_error()")) {
        errorstr += suberror;
      } else {
        PyErr_Clear();
      }
    } else {
      PyErr_Clear();
    }
    Py_XDECREF(pyExcValue);
  }
  if (pyExcTraceback != nullptr) {
    const auto *tb_o = reinterpret_cast<PyTracebackObject *>(pyExcTraceback);
    int line_num = tb_o->tb_lineno;
    errorstr += " in line ";
    errorstr += std::to_string(line_num);
    Py_XDECREF(pyExcTraceback);
  }
}

PyObject *python_callfunction(const std::shared_ptr<const Context>& cxt, const std::string& name,
                              const std::vector<std::shared_ptr<Assignment>>& op_args,
                              std::string& errorstr)
{
  PyObject *pFunc = nullptr;
  if (!pythonMainModule) {
    return nullptr;
  }

  int dotpos = name.find(".");
  if (dotpos > 0) {
    std::string varname = name.substr(1, dotpos - 1);  // assume its always in paranthesis
    std::string methodname = name.substr(dotpos + 1, name.size() - dotpos - 2);
    Value var = cxt->lookup_variable(varname, Location::NONE).clone();
    if (var.type() == Value::Type::PYTHONCLASS) {
      const PythonClassType& python_class = var.toPythonClass();
      PyObject *methodobj = PyUnicode_FromString(methodname.c_str());
      pFunc = PyObject_GenericGetAttr(reinterpret_cast<PyObject *>(python_class.ptr), methodobj);
    }
  }
  if (!pFunc) {
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());

    // search the function in all modules
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(maindict, &pos, &key, &value)) {
      PyObject *module = PyObject_GetAttrString(pythonMainModule.get(), PyUnicode_AsUTF8(key));
      if (module != nullptr) {
        PyObject *moduledict = PyModule_GetDict(module);
        Py_DECREF(module);
        if (moduledict != nullptr) {
          pFunc = PyDict_GetItemString(moduledict, name.c_str());
          if (pFunc != nullptr) break;
        }
      }
    }
  }
  if (!pFunc) {
    errorstr = "Function not found";
    return nullptr;
  }
  if (!PyCallable_Check(pFunc)) {
    errorstr = "Function not callable";
    return nullptr;
  }

  PyObject *args = PyTuple_New(op_args.size());
  for (unsigned int i = 0; i < op_args.size(); i++) {
    const Assignment *op_arg = op_args[i].get();

    std::shared_ptr<Expression> expr = op_arg->getExpr();
    PyObject *value = python_fromopenscad(expr.get()->evaluate(cxt));
    if (value != nullptr) {
      PyTuple_SetItem(args, i, value);
    }
  }
  PyObject *funcresult = PyObject_CallObject(pFunc, args);
  Py_XDECREF(args);

  errorstr = "";
  if (funcresult == nullptr) {
    python_catch_error(errorstr);
    PyErr_SetString(PyExc_TypeError, errorstr.c_str());

    return nullptr;
  }
  return funcresult;
}

/*
 * Actually trying use python to evaluate a OpenSCAD Module
 */

std::shared_ptr<AbstractNode> python_modulefunc(const ModuleInstantiation *op_module,
                                                const std::shared_ptr<const Context>& cxt,
                                                std::string& error)  // null & error: error, else: None
{
  std::shared_ptr<AbstractNode> result = nullptr;
  std::string errorstr = "";
  {
    PyObject *funcresult = python_callfunction(cxt, op_module->name(), op_module->arguments, errorstr);
    if (errorstr.size() > 0) {
      error = errorstr;
      return nullptr;
    }
    if (funcresult == nullptr) {
      error = "function not executed";
      return nullptr;
    }

    if (PyObject_IsInstance(funcresult, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
      PyObject *raw_dummydict = nullptr;
      result = PyOpenSCADObjectToNode(funcresult, &raw_dummydict);
      auto dummydict = py_owned(raw_dummydict);
    }
    Py_XDECREF(funcresult);
  }
  return result;
}

/*
 * Converting a python result to an openscad result. extra function required as it might call itself
 * hierarchically
 */

Value python_convertresult(PyObject *arg, int& error)
{
  error = 0;
  if (arg == nullptr) return Value::undefined.clone();
  if (PyList_Check(arg)) {
    VectorType vec(nullptr);
    for (int i = 0; i < PyList_Size(arg); i++) {
      PyObject *item = PyList_GetItem(arg, i);
      int suberror;
      vec.emplace_back(python_convertresult(item, suberror));
      error |= suberror;
    }
    return std::move(vec);
  } else if (PyFloat_Check(arg)) {
    return {PyFloat_AsDouble(arg)};
  } else if (arg == Py_False) {
    return false;
  } else if (arg == Py_True) {
    return true;
  } else if (PyLong_Check(arg)) {
    return {(double)PyLong_AsLong(arg)};
  } else if (PyUnicode_Check(arg)) {
    auto str = std::string(PyUnicode_AsUTF8(arg));
    return {str};
  } else if (arg == Py_None) {
    return Value::undefined.clone();
  } else if (arg->ob_type->tp_base == &PyBaseObject_Type) {
    Py_INCREF(arg);
    return PythonClassType(arg);
  } else {
    printf("unsupported type %s\n", arg->ob_type->tp_base->tp_name);
    PyErr_SetString(PyExc_TypeError, "Unsupported function result\n");
    error = 1;
  }
  return Value::undefined.clone();
}

/*
 * Actually trying use python to evaluate a OpenSCAD Function
 */

Value python_functionfunc(const FunctionCall *call, const std::shared_ptr<const Context>& cxt,
                          int& error)
{
  std::string errorstr = "";
  PyObject *funcresult = python_callfunction(cxt, call->name, call->arguments, errorstr);
  if (errorstr.size() > 0) {
    PyErr_SetString(PyExc_TypeError, errorstr.c_str());
    return Value::undefined.clone();
  }
  if (funcresult == nullptr) return Value::undefined.clone();

  Value res = python_convertresult(funcresult, error);
  Py_XDECREF(funcresult);
  return res;
}

extern PyObject *PyInit_data(void);
PyMODINIT_FUNC PyInit_PyData(void);
/*
 * Main python evaluation entry
 */
#ifdef HAVE_PYTHON_YIELD
std::vector<PyObject *> python_orphan_objs;
extern "C" {
void set_object_callback(void (*object_capture_callback)(PyObject *));
}
void openscad_object_callback(PyObject *obj)
{
  if (PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    Py_INCREF(obj);
    python_orphan_objs.push_back(obj);
  }
}
#endif
void initPython(const std::string& binDir, const std::string& scriptpath, const RenderVariables *r)
{
  static bool alreadyTried = false;
  if (alreadyTried) return;
  const auto name = PYTHON_EXECUTABLE_NAME;
  const auto exe = binDir + "/" + name;
  if (scriptpath.size() > 0) python_scriptpath = scriptpath;
  if (pythonInitDict) {
    /* Re-init path: remove user-added globals. Never call PyDict_DelItem* while iterating the same
     * dict with PyDict_Next — that invalidates the iteration and can crash (e.g. second initPython
     * during GUI startup). Snapshot keys first, then delete. */
    struct CleanupGil {
      PyGILState_STATE s;
      CleanupGil() : s(PyGILState_Ensure()) {}
      ~CleanupGil() { PyGILState_Release(s); }
    } cleanupGilScope;
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
    PyObject *mainKeyList = PyDict_Keys(maindict);
    if (mainKeyList != nullptr) {
      std::vector<std::string> mainKeysToDelete;
      std::vector<std::string> moduleKeysToDelete;
      PyObject *modulesMap = nullptr;

      const Py_ssize_t nMain = PyList_GET_SIZE(mainKeyList);
      for (Py_ssize_t i = 0; i < nMain; ++i) {
        PyObject *key = PyList_GET_ITEM(mainKeyList, i);
        if (!PyUnicode_Check(key)) {
          continue;
        }
        /* Best-effort cleanup walk: a key we can't UTF-8 encode is
         * a key we can't compare against the inventory anyway, so
         * skip it (and clear the helper's exception). */
        std::string keyStrStr;
        if (!python_pyobject_to_utf8(key, keyStrStr, "initPython() __main__ key")) {
          PyErr_Clear();
          continue;
        }
        const char *keyStr = keyStrStr.c_str();
        if (std::find(std::begin(pythonInventory), std::end(pythonInventory), keyStr) ==
            std::end(pythonInventory)) {
          if (keyStrStr.size() < 4 || strncmp(keyStr, "stat", 4) != 0) {
            mainKeysToDelete.emplace_back(keyStr);
          }
        }
        if (strcmp(keyStr, "sys") != 0) {
          continue;
        }
        PyObject *sysMod = PyDict_GetItem(maindict, key);
        if (sysMod == nullptr || !PyModule_Check(sysMod)) {
          continue;
        }
        PyObject *sysdict = PyModule_GetDict(sysMod);
        if (sysdict == nullptr) {
          continue;
        }
        PyObject *sysKeyList = PyDict_Keys(sysdict);
        if (sysKeyList == nullptr) {
          continue;
        }
        const Py_ssize_t nSys = PyList_GET_SIZE(sysKeyList);
        for (Py_ssize_t j = 0; j < nSys; ++j) {
          PyObject *k1 = PyList_GET_ITEM(sysKeyList, j);
          if (!PyUnicode_Check(k1)) {
            continue;
          }
          std::string k1StrStr;
          if (!python_pyobject_to_utf8(k1, k1StrStr, "initPython() sys.<attr> key")) {
            PyErr_Clear();
            continue;
          }
          if (k1StrStr != "modules") {
            continue;
          }
          PyObject *mm = PyDict_GetItem(sysdict, k1);
          if (mm == nullptr || !PyDict_Check(mm)) {
            continue;
          }
          modulesMap = mm;
          PyObject *modKeyList = PyDict_Keys(mm);
          if (modKeyList == nullptr) {
            continue;
          }
          const Py_ssize_t nMod = PyList_GET_SIZE(modKeyList);
          for (Py_ssize_t k = 0; k < nMod; ++k) {
            PyObject *k2 = PyList_GET_ITEM(modKeyList, k);
            if (!PyUnicode_Check(k2)) {
              continue;
            }
            std::string k2StrStr;
            if (!python_pyobject_to_utf8(k2, k2StrStr, "initPython() sys.modules key")) {
              PyErr_Clear();
              continue;
            }
            const char *k2Str = k2StrStr.c_str();
            PyObject *value2 = PyDict_GetItem(mm, k2);
            if (value2 == nullptr || !PyModule_Check(value2)) {
              continue;
            }
            PyObject *modrepr = PyObject_Repr(value2);
            if (modrepr == nullptr) {
              /* Same exception-hygiene treatment as the catcher /
               * catch_error sites: PyObject_Repr() leaves a
               * TypeError / MemoryError / etc. set on failure;
               * clear it so we do not poison the next iteration's
               * C-API calls. */
              PyErr_Clear();
              continue;
            }
            PyObjectUniquePtr modreprOwned(modrepr, &PyObjectDeleter);
            std::string modReprStr;
            if (!python_pyobject_to_utf8(modrepr, modReprStr, "initPython() repr(module)")) {
              PyErr_Clear();
              continue;
            }
            const char *modreprstr = modReprStr.c_str();
            if (strstr(modreprstr, "(frozen)") != nullptr) continue;
            if (strstr(modreprstr, "(built-in)") != nullptr) continue;
            if (strstr(modreprstr, "/encodings/") != nullptr) continue;
            if (strstr(modreprstr, "_frozen_") != nullptr) continue;
            if (strstr(modreprstr, "site-packages") != nullptr) continue;
            if (strstr(modreprstr, "usr/lib") != nullptr) continue;
            moduleKeysToDelete.emplace_back(k2Str);
          }
          Py_DECREF(modKeyList);
        }
        Py_DECREF(sysKeyList);
      }
      Py_DECREF(mainKeyList);

      // Some entries may have been removed between collection and
      // deletion (e.g. modules that drop themselves on import failure,
      // or the overlay names below that may never have been imported in
      // this run).  Probe with `PyMapping_HasKeyString` first so the
      // delete only happens when the key is actually present; this
      // matches the documented intent ("missing keys are expected") and
      // means no Python exception is ever raised in the embedded
      // interpreter, preventing it from poisoning later C-API calls.
      // `PyDict_DelItemString` cannot fail on a key that was just
      // observed by `HasKeyString`, so its return value is safe to
      // ignore.
      auto safe_del = [](PyObject *dict, const char *name) {
        if (PyMapping_HasKeyString(dict, name) > 0) {
          PyDict_DelItemString(dict, name);
        }
      };
      for (const auto& s : mainKeysToDelete) {
        safe_del(maindict, s.c_str());
      }
      if (modulesMap != nullptr) {
        // Defensive cleanup of the PythonSCAD module hierarchy:
        //
        //   _openscad  -> registered via PyImport_AppendInittab, repr contains
        //                 "(built-in)" and is therefore filtered out above.
        //                 We *intentionally* keep it cached because the C
        //                 module is a singleton (its types, methods and
        //                 internal state must not be torn down between scripts).
        //
        //   openscad   -> file-based pure-Python overlay
        //                 (libraries/python/openscad/__init__.py). The filter
        //                 above already enqueues it for deletion if it was
        //                 imported, but we list it explicitly so the behaviour
        //                 survives unrelated changes to the filter.  If the
        //                 user script never imported it, safe_del's
        //                 `PyMapping_HasKeyString` probe simply returns 0
        //                 and the delete is skipped.
        //
        //   pythonscad -> file-based pure-Python overlay; same rationale.
        const char *overlay_modules[] = {"openscad", "pythonscad"};
        for (const char *mod_name : overlay_modules) {
          if (std::find(moduleKeysToDelete.begin(), moduleKeysToDelete.end(), mod_name) ==
              moduleKeysToDelete.end()) {
            moduleKeysToDelete.emplace_back(mod_name);
          }
        }
        for (const auto& s : moduleKeysToDelete) {
          safe_del(modulesMap, s.c_str());
        }
      }
    }
  } else {
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);
    Py_PreInitialize(&preconfig);
    //    PyEval_InitThreads(); //
    //    https://stackoverflow.com/questions/47167251/pygilstate-ensure-causing-deadlock

#ifdef HAVE_PYTHON_YIELD
    set_object_callback(openscad_object_callback);
#endif
    PyImport_AppendInittab("_openscad", &PyInit__openscad);
    PyImport_AppendInittab("libfive", &PyInit_data);
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    std::string sep = "";
    std::ostringstream stream;
#ifdef _WIN32
    char sepchar = ';';
    sep = sepchar;
    stream << PlatformUtils::applicationPath() << "\\..\\libraries\\python";
#else
    char sepchar = ':';
    const auto pythonXY =
      "python" + std::to_string(PY_MAJOR_VERSION) + "." + std::to_string(PY_MINOR_VERSION);
    const std::array<std::string, 5> paths = {
      "../libraries/python",
      "../lib/" + pythonXY,
      "../python/lib/" + pythonXY,
      "../Frameworks/" + pythonXY,
      "../Frameworks/" + pythonXY + "/site-packages",
    };
    for (const auto& path : paths) {
      const auto p = fs::path(PlatformUtils::applicationPath() + fs::path::preferred_separator + path);
      if (fs::is_directory(p)) {
        stream << sep << fs::absolute(p).generic_string();
        sep = sepchar;
      }
    }
#endif
    // Add resource-based libraries directory (works for development, installed, and packaged builds)
    const auto resourceLibPath = fs::path(PlatformUtils::resourceBasePath()) / "libraries" / "python";
    if (fs::is_directory(resourceLibPath)) {
      stream << sepchar << fs::absolute(resourceLibPath).generic_string();
    }

    fs::path scriptfile(python_scriptpath);
    stream << sepchar << PlatformUtils::userPythonLibraryPath();
    stream << sepchar << PlatformUtils::userLibraryPath();
    stream << sepchar << scriptfile.parent_path().string();
    stream << sepchar << ".";
    PyConfig_SetBytesString(&config, &config.pythonpath_env, stream.str().c_str());

    if (!binDir.empty()) {
      PyConfig_SetBytesString(&config, &config.executable, (binDir + "/python").c_str());
    }

    PyStatus status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
      alreadyTried = true;
      LOG(message_group::Error, "Python %1$lu.%2$lu.%3$lu not found. Is it installed ?",
          PY_MAJOR_VERSION, PY_MINOR_VERSION, PY_MICRO_VERSION);
      return;
    }
    PyConfig_Clear(&config);

    PyObject *mainMod = PyImport_AddModule("__main__");
    if (mainMod == nullptr) {
      alreadyTried = true;
      return;
    }
    Py_INCREF(mainMod);
    pythonMainModule.reset(mainMod);
    PyObject *mainDict = PyModule_GetDict(pythonMainModule.get());
    if (mainDict == nullptr) {
      pythonMainModule.reset();
      alreadyTried = true;
      return;
    }
    Py_INCREF(mainDict);
    pythonInitDict.reset(mainDict);
    pythonMainModuleInitialized = true;
    pythonRuntimeInitialized = true;
    register_openscad_py_atexit();
    PyInit_PyData();
    // Append (NOT prepend) bundle-supplied Python packages to sys.path so a
    // user-installed copy (e.g. `pip install --user ipython`) always wins
    // over the bundled fallback that ships inside an AppImage / .app /
    // Windows installer. The directories are intentionally outside
    // `libraries/python/` (which holds PythonSCAD-owned overlays that
    // *should* take priority).
    {
      // The bundled-fallback search path is platform-specific because
      // the binary distributions stage the IPython wheels at different
      // locations relative to the executable:
      //
      //   * AppImage / source `make install` (Linux): pythonscad lives
      //     in <prefix>/bin, so `../lib/pythonscad-bundled-py` resolves
      //     to <prefix>/lib/pythonscad-bundled-py.
      //   * macOS .app: pythonscad lives in <App>.app/Contents/MacOS,
      //     so `../Resources/...` resolves to
      //     <App>.app/Contents/Resources/pythonscad-bundled-py.
      //     (Note: NOT `../Frameworks/...`. macOS `codesign` treats
      //     `Contents/Frameworks/` as a place where every subdirectory
      //     is a sub-bundle, and chokes on pip's `*.dist-info`
      //     directories with `bundle format unrecognized`. `Resources/`
      //     is the conventional home for arbitrary data assets.)
      //   * Windows native installer: CMake's `WIN32` branch sets
      //     `OPENSCAD_BINDIR = "."`, i.e. pythonscad.exe lives at the
      //     install prefix root. `../lib/...` would point OUTSIDE the
      //     installed tree (a sibling directory of the prefix), so we
      //     also try the no-prefix `lib/pythonscad-bundled-py` form.
      //     Listing both keeps the Linux entry working without a
      //     #ifdef _WIN32 split: `fs::is_directory(p)` returns false
      //     for whichever entry doesn't match on a given OS.
      const std::array<const char *, 3> bundledFallbackPaths = {
        "../lib/pythonscad-bundled-py",        // AppImage / Linux source install
        "../Resources/pythonscad-bundled-py",  // macOS .app
        "lib/pythonscad-bundled-py",           // Windows native installer
      };
      // Defensive: this runs during initPython, so any pending Python
      // exception left here would poison every subsequent C-API call.
      // PyErr_Clear() on every error path keeps the sys-path append
      // strictly best-effort: a missing `sys.path` entry never aborts
      // initialisation, but it never bleeds an exception either.
      PyObject *sysModule = PyImport_ImportModule("sys");
      if (sysModule == nullptr) {
        PyErr_Clear();
      } else {
        PyObject *sysPath = PyObject_GetAttrString(sysModule, "path");
        if (sysPath == nullptr) {
          PyErr_Clear();
        } else {
          if (PyList_Check(sysPath)) {
            const fs::path appPath = fs::path(PlatformUtils::applicationPath());
            for (const auto *relPath : bundledFallbackPaths) {
              // Use fs::path's operator/ rather than string concatenation:
              // `fs::path::preferred_separator` is `wchar_t` on Windows, which
              // does not compose with `std::string` and breaks the MSYS2 build.
              const fs::path p = appPath / relPath;
              if (!fs::is_directory(p)) continue;
              const fs::path absPath = fs::absolute(p);

              // Cross-platform Python-string construction:
              // * Windows native paths are wide (UTF-16) and may contain
              //   characters that have no valid UTF-8 round-trip via
              //   `PyUnicode_FromString` on a build where the runtime
              //   encoding isn't UTF-8. `PyUnicode_FromWideChar` is the
              //   canonical CPython entry point for that case.
              // * POSIX paths are bytes whose interpretation depends on
              //   the filesystem locale; `PyUnicode_DecodeFSDefaultAndSize`
              //   uses CPython's filesystem decoder (matches what
              //   `os.fsdecode` does in Python land), which is the right
              //   thing for a `sys.path` entry that user-side `import`
              //   logic will resolve through `os.path.*` operations.
              PyObject *pathStr = nullptr;
#ifdef _WIN32
              const std::wstring abs = absPath.wstring();
              pathStr = PyUnicode_FromWideChar(abs.c_str(), static_cast<Py_ssize_t>(abs.size()));
#else
              const std::string abs = absPath.native();
              pathStr =
                PyUnicode_DecodeFSDefaultAndSize(abs.c_str(), static_cast<Py_ssize_t>(abs.size()));
#endif
              if (pathStr == nullptr) {
                PyErr_Clear();
                continue;
              }
              if (PyList_Append(sysPath, pathStr) != 0) {
                PyErr_Clear();
              }
              Py_DECREF(pathStr);
            }
          }
          // Else: hostile sys.path replacement (user code replaced it
          // with a non-list); skip silently rather than risk exception.
          Py_DECREF(sysPath);
        }
        Py_DECREF(sysModule);
      }
    }
    PyRun_String("from builtins import *\n", Py_file_input, pythonInitDict.get(), pythonInitDict.get());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
    while (PyDict_Next(maindict, &pos, &key, &value)) {
      if (!PyUnicode_Check(key)) {
        continue;
      }
      /* Best-effort inventory snapshot of __main__ at initPython() time.
       * Skip (and clear) keys we can't UTF-8 encode -- they couldn't be
       * compared against on the cleanup walk above either. */
      std::string key_str;
      if (!python_pyobject_to_utf8(key, key_str, "initPython() inventory key")) {
        PyErr_Clear();
        continue;
      }
      pythonInventory.push_back(key_str);
    }
  }
  std::ostringstream stream;
  if (r != nullptr) {
    stream << "preview=" << (r->preview ? "True" : "False") << "\n";

    stream << "t=" << r->time << "\n";
    stream << "phi=" << 2 * G_PI * r->time << "\n";

    const auto vpr = r->camera.getVpr();
    stream << "vpr=[" << vpr.x() << "," << vpr.y() << "," << vpr.z() << "]\n";

    const auto vpt = r->camera.getVpt();
    stream << "vpt=[" << vpt.x() << "," << vpt.y() << "," << vpt.z() << "]\n";

    const auto vpd = r->camera.zoomValue();
    stream << "vpd=" << vpd << "\n";

    const auto vpf = r->camera.fovValue();
    stream << "vpf=" << vpf << "\n";
  }
  stream << commandline_commands << "\n";
  {
    PyGILState_STATE runGil = PyGILState_Ensure();
    PyRun_String(stream.str().c_str(), Py_file_input, pythonInitDict.get(), pythonInitDict.get());
    PyGILState_Release(runGil);
  }
  customizer_parameters_finished = customizer_parameters;
  customizer_parameters.clear();
  python_result_handle.clear();
  nodes_hold.clear();
  DECLARE_INSTANCE();
  void_node = std::make_shared<CubeNode>(instance);  // just placeholders
  full_node = std::make_shared<CubeNode>(instance);  // just placeholders
}

void finishPython(void)
{
  if (!pythonDryRun) {
    show_final();
  }
  pythonDryRun = false;
}

int debug_num, debug_cnt;  // Hidden debug aid

std::string evaluatePython(const std::string& code, bool dry_run)
{
  std::string error;
  genlang_result_node = nullptr;
  python_result_handle.clear();
  PyObjectUniquePtr pyExcValue(nullptr, &PyObjectDeleter);
  PyObjectUniquePtr pyExcTraceback(nullptr, &PyObjectDeleter);
  /* special python code to catch errors from stdout and stderr and make them available in OpenSCAD
   * console */
  for (ModuleInstantiation *mi : modinsts_list) {
    delete mi;  // best time to delete it
  }
  modinsts_list.clear();
  pythonDryRun = dry_run;
  if (!pythonMainModuleInitialized) return "Python not initialized";
  struct EvaluatePythonGil {
    bool on = false;
    PyGILState_STATE st{};
    EvaluatePythonGil()
    {
      if (Py_IsInitialized()) {
        st = PyGILState_Ensure();
        on = true;
      }
    }
    ~EvaluatePythonGil()
    {
      if (on) PyGILState_Release(st);
    }
  } gil;
#ifndef OPENSCAD_NOGUI
  const char *python_init_code =
    "\
import sys\n\
class InputCatcher:\n\
   def __init__(self):\n\
      self.data = \"modules\"\n\
   def read(self):\n\
      return self.data\n\
   def readline(self):\n\
      return self.data\n\
   def isatty(self):\n\
      return False\n\
class OutputCatcher:\n\
   def __init__(self):\n\
      self.data = ''\n\
   def write(self, stuff):\n\
      self.data = self.data + stuff\n\
   def flush(self):\n\
      pass\n\
catcher_in = InputCatcher()\n\
catcher_out = OutputCatcher()\n\
catcher_err = OutputCatcher()\n\
stdin_bak=sys.stdin\n\
stdout_bak=sys.stdout\n\
stderr_bak=sys.stderr\n\
sys.stdin = catcher_in\n\
sys.stdout = catcher_out\n\
sys.stderr = catcher_err\n\
";
  const char *python_exit_code =
    "\
sys.stdin = stdin_bak\n\
sys.stdout = stdout_bak\n\
sys.stderr = stderr_bak\n\
stdin_bak = None\n\
stdout_bak = None\n\
stderr_bak = None\n\
";

  PyRun_SimpleString(python_init_code);
#endif
#ifdef HAVE_PYTHON_YIELD
  for (auto obj : python_orphan_objs) {
    Py_DECREF(obj);
  }
  python_orphan_objs.clear();
#endif
  PyObjectUniquePtr result(nullptr, &PyObjectDeleter);
  result.reset(PyRun_String(code.c_str(), Py_file_input, pythonInitDict.get(),
                            pythonInitDict.get())); /* actual code is run here */

#ifndef OPENSCAD_NOGUI
  if (result == nullptr) {
    PyErr_Print();
    error = "";
    python_catch_error(error);
  }
  for (int i = 0; i < 2; i++) {
    PyObjectUniquePtr catcher(nullptr, &PyObjectDeleter);
    catcher.reset(
      PyObject_GetAttrString(pythonMainModule.get(), i == 1 ? "catcher_err" : "catcher_out"));
    if (catcher == nullptr) {
      /* The catcher_out / catcher_err globals are installed by
       * python_init_code above, but a user script could `del` them
       * (or replace them with something that raises on attribute
       * access). PyObject_GetAttrString sets AttributeError /
       * MemoryError / etc. on failure -- clear it so we don't
       * poison the next unrelated C-API call. */
      PyErr_Clear();
      continue;
    }
    PyObjectUniquePtr command_output(nullptr, &PyObjectDeleter);
    command_output.reset(PyObject_GetAttrString(catcher.get(), "data"));
    if (command_output.get() == nullptr) {
      PyErr_Clear();
      continue;
    }

    /* Best-effort capture of the catcher's accumulated stdout/stderr.
     * The helper can fail if `data` is somehow not a str (a
     * misbehaving catcher reimplementation), or if the str contains
     * lone surrogates that the strict utf-8 handler refuses to
     * encode. Drop the chunk in either case rather than crashing or
     * appending garbage to the user-visible error string. */
    std::string command_output_str;
    if (!python_pyobject_to_utf8(command_output.get(), command_output_str, "captured Python output")) {
      PyErr_Clear();
      continue;
    }
    if (!command_output_str.empty()) {
      if (i == 1) error += command_output_str; /* output to console */
      else LOG(command_output_str.c_str());    /* error to LOG */
    }
  }
  PyRun_SimpleString(python_exit_code);
#endif
  return error;
}
/*
 * the magical Python Type descriptor for an OpenSCAD Object. Adding more fields makes the type more
 * powerful
 */

int python__setitem__(PyObject *dict, PyObject *key, PyObject *v);
PyObject *python__getitem__(PyObject *dict, PyObject *key);

PyObject *python__getattro__(PyObject *dict, PyObject *key)
{
  PyObject *result = python__getitem__(dict, key);
  if (result == Py_None || result == nullptr) result = PyObject_GenericGetAttr(dict, key);
  return result;
}

int python__setattro__(PyObject *dict, PyObject *key, PyObject *v)
{
  return python__setitem__(dict, key, v);
}

// -------
// ItemRef
// -------

typedef struct {
  PyObject_HEAD PyOpenSCADObject *parent;
  size_t index;
} PyOpenSCADItemRef;

static void PyOpenSCADItemRef_dealloc(PyOpenSCADItemRef *self)
{
  Py_XDECREF(self->parent);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PyOpenSCADItemRef_get_value(PyOpenSCADItemRef *self, void *closure)
{
  PyObject *dummydict_raw = nullptr;
  std::shared_ptr<AbstractNode> parnode =
    PyOpenSCADObjectToNode(reinterpret_cast<PyObject *>(self->parent), &dummydict_raw);
  auto dummydict = py_owned(dummydict_raw);
  if (self->index >= parnode->children.size()) {
    PyErr_SetString(PyExc_IndexError, "child index out of range");
    return NULL;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, parnode->children[self->index]);
}

int PyOpenSCADItemRef_set_value(PyOpenSCADItemRef *self, PyObject *value, void *closure)
{
  PyObject *parent_dict_raw = nullptr;
  std::shared_ptr<AbstractNode> parnode =
    PyOpenSCADObjectToNode(reinterpret_cast<PyObject *>(self->parent), &parent_dict_raw);
  auto parent_dict = py_owned(parent_dict_raw);
  if (self->index >= parnode->children.size()) {
    PyErr_SetString(PyExc_IndexError, "child index out of range");
    return -1;
  }
  PyObject *value_dict_raw = nullptr;
  std::shared_ptr<AbstractNode> childnode = PyOpenSCADObjectToNode(value, &value_dict_raw);
  auto value_dict = py_owned(value_dict_raw);
  if (!childnode) {
    PyErr_SetString(PyExc_TypeError, "invalid OpenSCAD object");
    return -1;
  }
  parnode->children[self->index] = childnode;
  return 0;
}

static PyObject *PyOpenSCADItemRef_getattro(PyObject *self_obj, PyObject *attr_name)
{
  PyOpenSCADItemRef *self = (PyOpenSCADItemRef *)self_obj;

  // ZUERST: normale Attribute versuchen (z.B. "value")
  PyObject *result = PyObject_GenericGetAttr(self_obj, attr_name);
  if (result) return result;

  // Wenn nicht gefunden → Fehler löschen
  PyErr_Clear();

  // Jetzt: echtes Kind holen
  PyObject *value = PyOpenSCADItemRef_get_value(self, NULL);
  if (!value) return NULL;

  // Attribut auf dem echten Objekt suchen
  PyObject *forwarded = PyObject_GetAttr(value, attr_name);
  Py_DECREF(value);
  return forwarded;
}

static PyGetSetDef PyOpenSCADItemRef_getset[] = {
  {"value", (getter)PyOpenSCADItemRef_get_value, (setter)PyOpenSCADItemRef_set_value, "child object",
   NULL},
  {NULL}};

static PyTypeObject PyOpenSCADItemRefType = {
  PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pyopenscad.ChildRef",
  .tp_basicsize = sizeof(PyOpenSCADItemRef),
  .tp_dealloc = (destructor)PyOpenSCADItemRef_dealloc,
  .tp_getattro = PyOpenSCADItemRef_getattro,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_getset = PyOpenSCADItemRef_getset,
  .tp_new = PyType_GenericNew};

// ---------------------
// PyOpenSCADObjectIter
// ---------------------

typedef struct {
  PyObject_HEAD PyObject *container;  // Referenz auf das Original-Objekt
  Py_ssize_t index;                   // Aktueller Index
} PyOpenSCADObjectIter;

extern PyTypeObject PyOpenSCADObjectIterType;

PyObject *PyOpenSCADType_iter(PyObject *self)
{
  PyOpenSCADObjectIter *iter = reinterpret_cast<PyOpenSCADObjectIter *>(
    PyObject_New(PyOpenSCADObjectIter, &PyOpenSCADObjectIterType));
  if (iter == nullptr) {
    return nullptr;
  }
  Py_INCREF(iter);

  Py_INCREF(self);
  iter->container = self;
  iter->index = 0;

  return (PyObject *)iter;
}

PyObject *PyOpenSCADType_iternext(PyObject *self)
{
  PyOpenSCADObjectIter *iter = reinterpret_cast<PyOpenSCADObjectIter *>(self);
  PyOpenSCADObject *container = reinterpret_cast<PyOpenSCADObject *>(iter->container);

  // Prüfe ob noch Elemente vorhanden
  Py_ssize_t size = container->node->children.size();
  if (iter->index >= size) {
    PyErr_SetNone(PyExc_StopIteration);
    return nullptr;
  }

  // Nächstes Element holen

  PyOpenSCADItemRef *ref = PyObject_New(PyOpenSCADItemRef, &PyOpenSCADItemRefType);
  ref->parent = container;
  Py_INCREF(container);
  ref->index = iter->index++;
  return (PyObject *)ref;
}

// Iterator dealloc
void PyOpenSCADObjectIter_dealloc(PyOpenSCADObjectIter *self)
{
  PyOpenSCADObjectIter *iter = reinterpret_cast<PyOpenSCADObjectIter *>(self);
  Py_XDECREF(iter->container);
  PyObject_Del(self);
}

// Iterator Type Definition
PyTypeObject PyOpenSCADObjectIterType = {
  PyVarObject_HEAD_INIT(nullptr, 0).tp_name = "PyOpenSCADType.Iterator",
  .tp_basicsize = sizeof(PyOpenSCADObjectIter),
  .tp_dealloc = (destructor)PyOpenSCADObjectIter_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_iter = PyOpenSCADType_iter,
  .tp_iternext = PyOpenSCADType_iternext,  // <-- __next__ Implementierung
};

// ---------------------------
// PythonSCAD Sequence methods
// ---------------------------

PyObject *PyOpenSCAD_sq_item(PyOpenSCADObject *self, Py_ssize_t i)
{
  PyObject *dummydict_raw = nullptr;
  std::shared_ptr<AbstractNode> node =
    PyOpenSCADObjectToNode(reinterpret_cast<PyObject *>(self), &dummydict_raw);
  auto dummydict = py_owned(dummydict_raw);
  /* `PyOpenSCADObjectToNode` returns nullptr for a user-constructed
   * `PyOpenSCADType` instance whose `tp_init` did not populate
   * `self->node` (e.g. ``Openscad()`` then ``obj[0]``). Dereferencing
   * `node->children` in that case would segfault. Surface a clean
   * `TypeError` instead. `propagate_or_typeerror` preserves any
   * pre-existing pending exception (e.g. MemoryError from the
   * dict-merge path) verbatim. */
  if (node == nullptr) return propagate_or_typeerror("PyOpenSCAD object has no node");
  if (i < 0 || i >= (Py_ssize_t)node->children.size()) {
    PyErr_SetString(PyExc_IndexError, "index out of range");
    return nullptr;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node->children[i]);
}

Py_ssize_t PyOpenSCAD_sq_length(PyOpenSCADObject *self)
{
  PyObject *dummydict_raw = nullptr;
  std::shared_ptr<AbstractNode> node =
    PyOpenSCADObjectToNode(reinterpret_cast<PyObject *>(self), &dummydict_raw);
  auto dummydict = py_owned(dummydict_raw);
  /* See PyOpenSCAD_sq_item above for the null-`node` motivation.
   * `sq_length` returns Py_ssize_t -- the C-API convention for an
   * exceptional case is to return -1 with a Python exception set,
   * which CPython will then surface to the caller of `len(obj)`. */
  if (node == nullptr) {
    propagate_or_typeerror("PyOpenSCAD object has no node");
    return -1;
  }
  return node->children.size();
}

static PySequenceMethods PyOpenSCAD_sequence_methods = {
  (lenfunc)PyOpenSCAD_sq_length,
  0,
  0,
  (ssizeargfunc)PyOpenSCAD_sq_item,
};

PyTypeObject PyOpenSCADType = {
  PyVarObject_HEAD_INIT(nullptr, 0) "openscad.PyOpenSCAD", /* tp_name */
  sizeof(PyOpenSCADObject),                                /* tp_basicsize */
  0,                                                       /* tp_itemsize */
  (destructor)PyOpenSCADObject_dealloc,                    /* tp_dealloc */
  0,                                                       /* vectorcall_offset */
  0,                                                       /* tp_getattr */
  0,                                                       /* tp_setattr */
  0,                                                       /* tp_as_async */
  python_str,                                              /* tp_repr */
  &PyOpenSCADNumbers,                                      /* tp_as_number */
  &PyOpenSCAD_sequence_methods,                            /* tp_as_sequence */
  &PyOpenSCADMapping,                                      /* tp_as_mapping */
  0,                                                       /* tp_hash  */
  0,                                                       /* tp_call */
  python_str,                                              /* tp_str */
  python__getattro__,                                      /* tp_getattro */
  python__setattro__,                                      /* tp_setattro */
  0,                                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                /* tp_flags */
  "PyOpenSCAD Object",                                     /* tp_doc */
  0,                                                       /* tp_traverse */
  0,                                                       /* tp_clear */
  0,                                                       /* tp_richcompare */
  0,                                                       /* tp_weaklistoffset */
  PyOpenSCADType_iter,                                     /* tp_iter */
  0,                                                       /* tp_iternext */
  PyOpenSCADMethods,                                       /* tp_methods */
  0,                                                       /* tp_members */
  0,                                                       /* tp_getset */
  0,                                                       /* tp_base */
  0,                                                       /* tp_dict */
  0,                                                       /* tp_descr_get */
  0,                                                       /* tp_descr_set */
  0,                                                       /* tp_dictoffset */
  (initproc)PyOpenSCADInit,                                /* tp_init */
  PyOpenSCADObject_alloc,                                  /* tp_alloc */
  PyOpenSCADObject_new,                                    /* tp_new */
};

static PyModuleDef OpenSCADModule = {PyModuleDef_HEAD_INIT,
                                     "_openscad",
                                     "OpenSCAD Python Module (low-level C extension)",
                                     -1,
                                     PyOpenSCADFunctions,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr};

#if PY_VERSION_HEX < 0x030A0000
// Polyfill for `PyModule_AddObjectRef`, which CPython only added in 3.10.
// Debian bullseye and RHEL/EL 9 ship Python 3.9, so we have to emulate it
// there. This is a near-verbatim port of CPython's upstream implementation
// in `Python/modsupport.c` (the body of the function below matches
// the 3.10+ source line for line, only reformatted to project style),
// so the validation surface (TypeError on a non-module first arg,
// SystemError on a NULL value with no pending exception, SystemError
// on a module with no `__dict__`) and the no-steal reference semantics
// match the 3.10+ symbol exactly.
static int PyModule_AddObjectRef(PyObject *mod, const char *name, PyObject *value)
{
  if (!PyModule_Check(mod)) {
    PyErr_SetString(PyExc_TypeError,
                    "PyModule_AddObjectRef() first argument "
                    "must be a module");
    return -1;
  }
  if (!value) {
    if (!PyErr_Occurred()) {
      PyErr_SetString(PyExc_SystemError,
                      "PyModule_AddObjectRef() must be called "
                      "with an exception raised if value is NULL");
    }
    return -1;
  }

  PyObject *dict = PyModule_GetDict(mod);
  if (dict == nullptr) {
    PyErr_Format(PyExc_SystemError, "module '%s' has no __dict__", PyModule_GetName(mod));
    return -1;
  }
  return PyDict_SetItemString(dict, name, value);
}
#endif

// Sole init entry point for the `_openscad` extension module.  This
// function is called both for the embedded interpreter (registered via
// `PyImport_AppendInittab("_openscad", ...)` in `initPython`) and for
// the pip-built shared library (dlsym'd by CPython as `PyInit__openscad`
// matching the extension name).  It must therefore do everything needed
// to leave `_openscad` usable from a bare `from _openscad import *`,
// including registering the `Openscad`, `ChildRef` and `ChildIterator`
// type objects.
// NOLINTNEXTLINE(bugprone-reserved-identifier)
PyMODINIT_FUNC PyInit__openscad(void)
{
  if (PyType_Ready(&PyOpenSCADType) < 0) return nullptr;
  if (PyType_Ready(&PyOpenSCADVectorType) < 0) return nullptr;
  if (PyType_Ready(&PyOpenSCADItemRefType) < 0) return nullptr;
  if (PyType_Ready(&PyOpenSCADObjectIterType) < 0) return nullptr;
  if (PyType_Ready(&PyOpenSCADBoundMemberType) < 0) return nullptr;

  PyObject *m = PyModule_Create(&OpenSCADModule);
  if (m == nullptr) return nullptr;

  // Use `PyModule_AddObjectRef` (CPython >=3.10, polyfilled above for
  // older Pythons) instead of the legacy `Py_INCREF` + `PyModule_AddObject`
  // pair: the legacy API only steals the reference on success, so a
  // failure left the manual `Py_INCREF` leaking and the module
  // half-initialised. `PyModule_AddObjectRef` never steals, making the
  // ref counting symmetric on both paths.
  if (PyModule_AddObjectRef(m, "Openscad", reinterpret_cast<PyObject *>(&PyOpenSCADType)) < 0) {
    Py_DECREF(m);
    return nullptr;
  }
  if (PyModule_AddObjectRef(m, "ChildRef", reinterpret_cast<PyObject *>(&PyOpenSCADItemRefType)) < 0) {
    Py_DECREF(m);
    return nullptr;
  }
  if (PyModule_AddObjectRef(m, "ChildIterator",
                            reinterpret_cast<PyObject *>(&PyOpenSCADObjectIterType)) < 0) {
    Py_DECREF(m);
    return nullptr;
  }

  // When loaded as a pip module the full runtime (initPython) is never
  // called, so the globals it normally sets up are still null.  Initialise
  // them here so that show(), export(), etc. work from a plain
  // "from _openscad import *" (or via the `openscad`/`pythonscad` overlay packages) session.
  if (!pythonMainModule) {
    PyObject *mainMod = PyImport_AddModule("__main__");
    if (mainMod != nullptr) {
      Py_INCREF(mainMod);
      pythonMainModule.reset(mainMod);
      PyObject *d = PyModule_GetDict(pythonMainModule.get());
      if (d != nullptr) {
        Py_INCREF(d);
        pythonInitDict.reset(d);
        pythonMainModuleInitialized = true;
        pythonRuntimeInitialized = true;
      } else {
        pythonMainModule.reset();
      }
    }
    // Placeholder nodes used as sentinels (void / full space)
    if (!void_node) {
      DECLARE_INSTANCE();
      void_node = std::make_shared<CubeNode>(instance);
      full_node = std::make_shared<CubeNode>(instance);
    }
  }

  register_openscad_py_atexit();
  return m;
}

// Run sys.__interactivehook__ to enable readline / tab completion / history
// in the basic embedded REPL. Returns 0 on success (i.e. carry on running
// the REPL) and is best-effort -- a missing hook is not an error. Each
// error path explicitly clears the Python exception state so subsequent
// C-API calls (PyRun_AnyFileFlags etc.) cannot inherit a stale exception.
static int pymain_run_interactive_hook(void)
{
  PyObject *sys = PyImport_ImportModule("sys");
  if (sys == nullptr) {
    PyErr_Clear();
    PySys_WriteStderr("Failed importing sys for sys.__interactivehook__\n");
    return 0;
  }

  PyObject *hook = PyObject_GetAttrString(sys, "__interactivehook__");
  Py_DECREF(sys);
  if (hook == nullptr) {
    PyErr_Clear();
    return 0;
  }

  if (PySys_Audit("cpython.run_interactivehook", "O", hook) < 0) {
    PyErr_Clear();
    PySys_WriteStderr("Failed auditing sys.__interactivehook__\n");
    Py_DECREF(hook);
    return 0;
  }
  PyObject *result = PyObject_CallNoArgs(hook);
  Py_DECREF(hook);
  if (result == nullptr) {
    PyErr_Clear();
    PySys_WriteStderr("Failed calling sys.__interactivehook__\n");
    return 0;
  }
  Py_DECREF(result);
  return 0;
}

#ifdef _WIN32
// Wire stdin/stdout/stderr up to the launching console.
//
// pythonscad.exe is built as IMAGE_SUBSYSTEM_WINDOWS_GUI so it can pop a
// graphical window when launched without arguments. A side effect is that
// when the *parent* shell is PowerShell (and stdio has not been explicitly
// redirected by the parent), the child inherits NULL handles for
// STD_INPUT_HANDLE / STD_OUTPUT_HANDLE / STD_ERROR_HANDLE -- PowerShell
// does not share its console with GUI children the way cmd.exe does. The
// embedded Python interpreter therefore sees `stdin == NULL` (fd == -2),
// PyRun_AnyFileFlags hits immediate EOF, and `--repl` exits silently
// without ever drawing a `>>>` prompt. IPython's prompt_toolkit dies the
// same way one step earlier. This is the symptom reported in pythonscad
// #620 and #621.
//
// Fix: AttachConsole(ATTACH_PARENT_PROCESS) borrows the parent shell's
// console, then _wfreopen(CONIN$ / CONOUT$) gives the C runtime real
// terminal-backed FILE* streams that fd-mapping and isatty() recognise.
// After this Python's interactive loop runs the way it does on any
// console-subsystem build.
//
// Best-effort: missing parent console (genuinely detached process,
// already attached, redirected stdin via `<`) is fine -- we leave the
// existing streams alone in those cases.
static void windows_reattach_console_for_repl(void)
{
  // Best-effort attach to the parent's console. Returns 0 with
  // ERROR_ACCESS_DENIED if a console is already attached (the typical
  // path when launched via pythonscad.com, which IS console-subsystem and
  // thus already has its console inherited by the GUI child) -- that's
  // exactly the state we want, so the failure is benign.
  AttachConsole(ATTACH_PARENT_PROCESS);

  // Reopen any standard stream whose CRT file descriptor is unbound
  // (fd == -2). For GUI-subsystem children the MSVC runtime skips lazy
  // fd binding for stdin even when STD_INPUT_HANDLE is a perfectly good
  // console handle; PyRun_AnyFileFlags then sees fd=-2, hits immediate
  // EOF and exits without ever drawing a `>>>` prompt. _wfreopen on
  // CONIN$ / CONOUT$ goes through the CRT's open-by-name path, which
  // *does* allocate an fd, and the resulting streams are isatty-true
  // and line-buffered -- the contract Python's interactive loop expects.
  //
  // The fd==-2 guard means we never clobber an explicit redirect. If
  // the user ran `pythonscad --repl < script.py`, the parent shell set
  // up stdin as a pipe/file with a real fd; Python will then take the
  // PyRun_SimpleFile path on that fd, which is what we want.
  if (_fileno(stdin) == -2) (void)_wfreopen(L"CONIN$", L"r", stdin);
  if (_fileno(stdout) == -2) (void)_wfreopen(L"CONOUT$", L"w", stdout);
  if (_fileno(stderr) == -2) (void)_wfreopen(L"CONOUT$", L"w", stderr);
}
#endif  // _WIN32

// Drop into the basic embedded CPython REPL on stdin. Used by the
// explicit `--repl` flag and as the fallback when `--ipython` cannot
// launch IPython.
//
// The user namespace (`__main__`) starts empty -- we deliberately do
// NOT auto-import any PythonSCAD names. Users opt in with whichever of
// `from pythonscad import *`, `from openscad import *`, or
// `import pythonscad` matches their preferred style; auto-importing
// would silently break the third form (`pythonscad.cube(10)`) and
// pollute `dir()` with a couple hundred names regardless of what the
// user actually intended to use.
//
// This helper assumes Py_IsInitialized() is already true; callers must
// gate it behind that check.
static void run_basic_python_repl(void)
{
  pymain_run_interactive_hook();
  PyCompilerFlags cf = _PyCompilerFlags_INIT;
  PyRun_AnyFileFlags(stdin, "<stdin>", &cf);
}

// Open the basic embedded CPython REPL on stdin. Reachable explicitly
// via `--repl` and as the fallback path of `ipython()` when IPython is
// not installed. The user namespace is empty on entry; the user is
// responsible for any imports they need (e.g. `from pythonscad import
// *`, `from openscad import *`, or `import pythonscad`). Returns 0 on
// clean exit, 1 on init failure.
int repl(void)
{
#ifdef _WIN32
  // Borrow the parent shell's console BEFORE Py_Initialize so the embedded
  // interpreter wraps real terminal-backed FILE* streams instead of the
  // CRT-orphaned ones GUI-subsystem children get on Windows.
  windows_reattach_console_for_repl();
#endif
  initPython(PlatformUtils::applicationPath(), "", nullptr);
  if (!Py_IsInitialized()) {
    fprintf(stderr,
            "PythonSCAD: embedded Python interpreter could not be initialised; "
            "--repl is unavailable.\n");
    return 1;
  }
  run_basic_python_repl();
  return 0;
}

// Distinguish "IPython is not installed" from "IPython is broken". Both
// surface as `PyImport_ImportModule("IPython") == NULL`, but for the
// "not installed" case we want a friendly diagnostic, while for the
// broken case we want the actual Python traceback so the user can see
// what dependency is missing or what the syntax error was.
//
// The active Python exception is fetched and either printed via
// PyErr_Print() (broken IPython) or cleared (genuinely missing). The
// `out_truly_missing` parameter is set to true for the missing-package
// case.
static void diagnose_failed_ipython_import(bool *out_truly_missing)
{
  *out_truly_missing = false;
  PyObject *exc_type = nullptr;
  PyObject *exc_value = nullptr;
  PyObject *exc_tb = nullptr;
  PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
  PyErr_NormalizeException(&exc_type, &exc_value, &exc_tb);

  if (exc_type != nullptr && PyErr_GivenExceptionMatches(exc_type, PyExc_ModuleNotFoundError)) {
    PyObject *missing_name = exc_value != nullptr ? PyObject_GetAttrString(exc_value, "name") : nullptr;
    if (missing_name != nullptr && PyUnicode_Check(missing_name)) {
      const char *missing_name_utf8 = PyUnicode_AsUTF8(missing_name);
      if (missing_name_utf8 != nullptr && std::string(missing_name_utf8) == "IPython") {
        *out_truly_missing = true;
      }
    }
    Py_XDECREF(missing_name);
    // Probing exc_value.name (PyObject_GetAttrString / PyUnicode_AsUTF8)
    // can set a new exception on failure. We're about to either restore the
    // original ImportError for PyErr_Print() or drop it entirely, so any
    // leaked probing exception must be cleared first; otherwise it would
    // poison subsequent C-API calls in the fallback REPL.
    PyErr_Clear();
  }

  if (*out_truly_missing) {
    // The user just doesn't have IPython installed -- no traceback
    // needed, the friendly hint is enough.
    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value);
    Py_XDECREF(exc_tb);
  } else {
    // IPython is technically importable but raised something we don't
    // recognise as "not installed" (broken transitive dependency,
    // SyntaxError on bad install, ...). Print the actual exception so
    // the user can fix it.
    PyErr_Restore(exc_type, exc_value, exc_tb);
    PyErr_Print();
  }
}

// Launch the real IPython interactive shell. Forwards `args` as
// IPython's argv (so `pythonscad --ipython script.py arg1` runs the
// script under the IPython kernel). The user namespace starts empty;
// the user is responsible for any imports they need (`from pythonscad
// import *`, `from openscad import *`, or `import pythonscad`). If
// IPython is not importable, prints a diagnostic to stderr and falls
// back to the basic REPL so the user still gets an interactive prompt
// instead of a hard error. Returns 0 on clean exit, 1 on init failure.
int ipython(const std::vector<std::string>& args)
{
#ifdef _WIN32
  // Same console-reattach dance as repl() -- needed so IPython's
  // prompt_toolkit sees real terminal-backed stdio, and so the basic-REPL
  // fallback below inherits a working console when IPython is missing.
  windows_reattach_console_for_repl();
#endif
  initPython(PlatformUtils::applicationPath(), "", nullptr);
  if (!Py_IsInitialized()) {
    fprintf(stderr,
            "PythonSCAD: embedded Python interpreter could not be initialised; "
            "--ipython is unavailable.\n");
    return 1;
  }

  // Detect IPython availability up-front in C++ so the fallback to the
  // basic REPL happens cleanly without a half-initialised IPython on
  // sys.modules.
  PyObject *ipython_mod = PyImport_ImportModule("IPython");
  if (ipython_mod == nullptr) {
    bool truly_missing = false;
    diagnose_failed_ipython_import(&truly_missing);
    if (truly_missing) {
      fprintf(stderr,
              "PythonSCAD: IPython is not installed; falling back to the basic Python prompt.\n"
              "            Run `pip install ipython` for a richer REPL, or pass --repl to "
              "skip this notice.\n");
    } else {
      fprintf(stderr,
              "PythonSCAD: IPython could not be imported (see traceback above); falling "
              "back to the basic Python prompt.\n");
    }
    run_basic_python_repl();
    return 0;
  }
  Py_DECREF(ipython_mod);

  // Build the IPython argv list. PyUnicode_DecodeFSDefaultAndSize is the
  // right primitive for argv strings since they may carry non-UTF-8
  // bytes on Linux/macOS or non-ASCII filenames on Windows. Any failure
  // (allocation, decoding, dict insertion) cleanly tears down what we
  // built so far and falls back to the basic REPL instead of crashing.
  PyObject *argv_list = PyList_New(static_cast<Py_ssize_t>(args.size()));
  if (argv_list == nullptr) {
    PyErr_Clear();
    fprintf(stderr,
            "PythonSCAD: could not allocate IPython argv; falling back to the basic "
            "Python prompt.\n");
    run_basic_python_repl();
    return 0;
  }
  for (Py_ssize_t i = 0; i < static_cast<Py_ssize_t>(args.size()); ++i) {
    PyObject *arg =
      PyUnicode_DecodeFSDefaultAndSize(args[i].data(), static_cast<Py_ssize_t>(args[i].size()));
    if (arg == nullptr) {
      PyErr_Clear();
      Py_DECREF(argv_list);
      fprintf(stderr,
              "PythonSCAD: could not decode argv[%zd]; falling back to the basic Python prompt.\n", i);
      run_basic_python_repl();
      return 0;
    }
    if (PyList_SetItem(argv_list, i, arg) != 0) {
      // PyList_SetItem only steals the reference on success.
      PyErr_Clear();
      Py_DECREF(arg);
      Py_DECREF(argv_list);
      fprintf(stderr,
              "PythonSCAD: could not assemble IPython argv; falling back to the basic "
              "Python prompt.\n");
      run_basic_python_repl();
      return 0;
    }
  }
  PyObject *main_module = PyImport_AddModule("__main__");
  PyObject *main_dict = main_module != nullptr ? PyModule_GetDict(main_module) : nullptr;
  if (main_dict == nullptr ||
      PyDict_SetItemString(main_dict, "__pythonscad_ipython_argv__", argv_list) != 0) {
    PyErr_Clear();
    Py_DECREF(argv_list);
    fprintf(stderr,
            "PythonSCAD: could not install IPython argv into __main__; falling back to "
            "the basic Python prompt.\n");
    run_basic_python_repl();
    return 0;
  }
  Py_DECREF(argv_list);

  // IPython's `start_ipython(user_ns=globals())` exposes the bootstrap's
  // private helper names to the user namespace. Some IPython exit paths
  // (e.g. EOF after the "Do you really want to exit" confirmation) clear
  // entries from that dict before returning, so the cleanup loop must
  // tolerate missing names.
  static const char *boot =
    "from IPython import start_ipython as _pythonscad_start_ipython\n"
    "_pythonscad_start_ipython(argv=__pythonscad_ipython_argv__, user_ns=globals())\n"
    "for _name in ('__pythonscad_ipython_argv__', '_pythonscad_start_ipython',\n"
    "              '_name'):\n"
    "    globals().pop(_name, None)\n";

  if (PyRun_SimpleString(boot) != 0) {
    // Surface the actual Python exception so failures are diagnosable
    // instead of silently dropping the user into the fallback REPL.
    PyErr_Print();
    // Bootstrap may have failed BEFORE reaching its own cleanup loop
    // (e.g. `from IPython import start_ipython as ...` raises) so the
    // helper names we (or the bootstrap) injected into `__main__` would
    // otherwise leak into the fallback REPL session, where the user is
    // already busy debugging a startup failure. Scrub them defensively
    // here. `PyDict_DelItemString` reports a KeyError-equivalent for
    // missing keys, which is fine -- we just clear and move on so the
    // post-cleanup state is the same regardless of which step failed.
    if (main_dict != nullptr) {
      static const char *const helper_names[] = {
        "__pythonscad_ipython_argv__",
        "_pythonscad_start_ipython",
        "_name",
      };
      for (const char *helper_name : helper_names) {
        if (PyDict_DelItemString(main_dict, helper_name) != 0) {
          PyErr_Clear();
        }
      }
    }
    fprintf(stderr, "PythonSCAD: IPython startup failed; falling back to the basic Python prompt.\n");
    run_basic_python_repl();
  }
  return 0;
}
// -------------------------

#ifdef ENABLE_JUPYTER
void python_startjupyter(void)
{
  const char *python_init_code =
    "\
import sys\n\
class OutputCatcher:\n\
   def __init__(self):\n\
      self.data = ''\n\
   def write(self, stuff):\n\
      self.data = self.data + stuff\n\
   def flush(self):\n\
      pass\n\
catcher_out = OutputCatcher()\n\
catcher_err = OutputCatcher()\n\
stdout_bak=sys.stdout\n\
stderr_bak=sys.stderr\n\
sys.stdout = catcher_out\n\
sys.stderr = catcher_err\n\
";
  initPython(0.0);
  PyRun_SimpleString(python_init_code);

  auto logger = xeus::make_console_logger(
    xeus::xlogger::msg_type, xeus::make_file_logger(xeus::xlogger::full, "my_log_file.log"));
  try {
    xeus::xconfiguration config = xeus::load_configuration(python_jupyterconfig);
    std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();

    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<openscad_jupyter::interpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new openscad_jupyter::interpreter());

    // Create kernel instance and start it
    xeus::xkernel kernel(config, xeus::get_user_name(), std::move(context), std::move(interpreter),
                         xeus::make_xserver_shell_main, xeus::make_in_memory_history_manager(),
                         std::move(logger));

    kernel.start();
  } catch (std::exception& e) {
    printf("Exception %s during startup of jupyter\n", e.what());
  }
}
#endif
