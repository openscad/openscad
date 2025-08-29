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
#include <filesystem>

#include "pyopenscad.h"
#include "pydata.h"
#include "core/CsgOpNode.h"
#include "Value.h"
#ifndef OPENSCAD_NOGUI
#include "executable.h"
#endif
#include "Expression.h"
#include "PlatformUtils.h"
#include <Context.h>
#include <Selection.h>
#include "platform/PlatformUtils.h"
#include "primitives.h"
namespace fs = std::filesystem;

// #define HAVE_PYTHON_YIELD
extern "C" PyObject *PyInit_openscad(void);
PyMODINIT_FUNC PyInit_PyOpenSCAD(void);

bool python_active;
bool python_trusted;
fs::path python_scriptpath;
// https://docs.python.org/3.10/extending/newtypes.html

void PyObjectDeleter(PyObject *pObject) { Py_XDECREF(pObject); };

PyObjectUniquePtr pythonInitDict(nullptr, PyObjectDeleter);
PyObjectUniquePtr pythonMainModule(nullptr, PyObjectDeleter);
std::list<std::string> pythonInventory;
AssignmentList customizer_parameters;
AssignmentList customizer_parameters_finished;
bool pythonDryRun = false;
PyObject *python_result_obj = nullptr;
std::vector<SelectedObject> python_result_handle;
bool python_runipython = false;
bool pythonMainModuleInitialized = false;
bool pythonRuntimeInitialized = false;

std::vector<std::shared_ptr<AbstractNode>> nodes_hold;  // make sure, that these nodes are not yet freed
std::shared_ptr<AbstractNode> void_node, full_node;

void PyOpenSCADObject_dealloc(PyOpenSCADObject *self)
{
  Py_XDECREF(self->dict);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/*
 *  allocates a new PyOpenSCAD Object including its internal dictionary
 */

static PyObject *PyOpenSCADObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)type->tp_alloc(type, 0);
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

/*
 *  allocates a new PyOpenSCAD to store an existing OpenSCAD Abstract Node
 */

PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)type->tp_new(type, nullptr, nullptr);
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

/*
 *  parses either a PyOpenSCAD Object or an List of PyOpenScad Object and adds it to the list of supplied
 * children, returns 1 on success
 */

int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj)
{
  int i, n;
  PyObject *obj;
  PyObject *dummy_dict;
  std::shared_ptr<AbstractNode> child;
  if (PyList_Check(more_obj)) {
    n = PyList_Size(more_obj);
    for (i = 0; i < n; i++) {
      obj = PyList_GetItem(more_obj, i);
      child = PyOpenSCADObjectToNode(obj, &dummy_dict);
      children.push_back(child);
    }
  } else if (PyObject_IsInstance(more_obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    child = PyOpenSCADObjectToNode(more_obj, &dummy_dict);
    children.push_back(child);
  } else return 1;
  return 0;
}

/*
 *  extracts Absrtract Node from PyOpenSCAD Object
 */

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj, PyObject **dict)
{
  std::shared_ptr<AbstractNode> result = ((PyOpenSCADObject *)obj)->node;
  if (result.use_count() > 2 && result != void_node && result != full_node) {
    result = result->clone();
  }
  *dict = ((PyOpenSCADObject *)obj)->dict;
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

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs, PyObject **dict)
{
  std::shared_ptr<AbstractNode> result = nullptr;
  if (PyObject_IsInstance(objs, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    result = ((PyOpenSCADObject *)objs)->node;
    if (result.use_count() > 2 && result != void_node && result != full_node) {
      result = result->clone();
    }
    *dict = ((PyOpenSCADObject *)objs)->dict;
  } else if (PyList_Check(objs)) {
    DECLARE_INSTANCE
    auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    int n = PyList_Size(objs);
    std::vector<PyObject *> child_dict;
    PyObject *subdict;
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      if (PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
        std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &subdict);
        node->children.push_back(child);
        child_dict.push_back(subdict);
      } else return nullptr;
    }
    result = node;

    *dict = PyDict_New();
    for (int i = child_dict.size() - 1; i >= 0; i--)  // merge from back  to give 1st child most priority
    {
      auto& subdict = child_dict[i];
      if (subdict == nullptr) continue;
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(subdict, &pos, &key, &value)) {
        PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
        const char *value_str = PyBytes_AS_STRING(value1);
        PyDict_SetItem(*dict, key, value);
      }
    }
  } else if (objs == Py_None || objs == Py_False) {
    result = void_node;
    *dict = nullptr;  // TODO improve
  } else if (objs == Py_True) {
    result = full_node;
    *dict = nullptr;  // TODO improve
  } else result = nullptr;
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
void python_build_hashmap(const std::shared_ptr<AbstractNode>& node, int level)
{
  PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  std::ostringstream stream;
  //  python_hierdump(stream, node);
  std::string code = stream.str();
  while (PyDict_Next(maindict, &pos, &key, &value)) {
    if (!PyObject_IsInstance(value, reinterpret_cast<PyObject *>(&PyOpenSCADType))) continue;
    std::shared_ptr<AbstractNode> testnode = ((PyOpenSCADObject *)value)->node;
    if (testnode != node) continue;
    PyObject *key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
    if (key1 == nullptr) continue;
    const char *key_str = PyBytes_AS_STRING(key1);
    if (key_str == nullptr) continue;
    mapping_name.push_back(key_str);
    mapping_code.push_back(code);
    mapping_level.push_back(pos);
  }
  if (level < 5) {  // no  many level are unclear and error prone(overwrites memory)
    for (const auto& child : node->getChildren()) {
      python_build_hashmap(child, level + 1);
    }
  }
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
/*
 * converts a python obejct into an integer by all means
 */

int python_numberval(PyObject *number, double *result, int *flags, int flagor)
{
  if (number == nullptr) return 1;
  if (number == Py_False) return 1;
  if (number == Py_True) return 1;
  if (number == Py_None) return 1;
  if (PyFloat_Check(number)) {
    *result = PyFloat_AsDouble(number);
    return 0;
  }
  if (PyLong_Check(number)) {
    *result = PyLong_AsLong(number);
    return 0;
  }
  if (number->ob_type == &PyDataType && flags != nullptr) {
    *flags |= flagor;
    *result = PyDataObjectToValue(number);
    return 0;
  }
  if (PyNumber_Check(number)) {
    // Handle other python number protocol objects (e.g. numpy.int64)
    PyObject *f = PyNumber_Float(number);
    *result = PyFloat_AsDouble(f);
    return 0;
  }
  if (PyUnicode_Check(number) && flags != nullptr) {
    PyObjectUniquePtr str(PyUnicode_AsEncodedString(number, "utf-8", "~"), PyObjectDeleter);
    char *str1 = PyBytes_AS_STRING(str.get());
    sscanf(str1, "%lf", result);
    if (flags != nullptr) *flags |= flagor;
    return 0;
  }
  return 1;
}

std::vector<int> python_intlistval(PyObject *list)
{
  std::vector<int> result;
  PyObject *item;
  if (PyLong_Check(list)) {
    result.push_back(PyLong_AsLong(list));
  }
  if (PyList_Check(list)) {
    for (int i = 0; i < PyList_Size(list); i++) {
      item = PyList_GetItem(list, i);
      if (PyLong_Check(item)) {
        result.push_back(PyLong_AsLong(item));
      }
    }
  }
  return result;
}
/*
 * Tries to extract an 3D vector out of a python list
 */

int python_vectorval(PyObject *vec, int minval, int maxval, double *x, double *y, double *z, double *w,
                     int *flags)
{
  if (vec == nullptr) return 1;
  if (flags != nullptr) *flags = 0;
  if (PyList_Check(vec)) {
    if (PyList_Size(vec) < minval || PyList_Size(vec) > maxval) return 1;

    if (PyList_Size(vec) >= 1) {
      if (python_numberval(PyList_GetItem(vec, 0), x, flags, 1)) return 1;
    }
    if (PyList_Size(vec) >= 2) {
      if (python_numberval(PyList_GetItem(vec, 1), y, flags, 2)) return 1;
    }
    if (PyList_Size(vec) >= 3) {
      if (python_numberval(PyList_GetItem(vec, 2), z, flags, 4)) return 1;
    }
    if (PyList_Size(vec) >= 4 && w != NULL) {
      if (python_numberval(PyList_GetItem(vec, 3), w, flags, 8)) return 1;
    }
    return 0;
  }
  if (!python_numberval(vec, x)) {
    *y = *x;
    *z = *x;
    if (w != NULL) *w = *x;
    return 0;
  }
  return 1;
}

std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim, int *dragflags)
{
  std::vector<Vector3d> results;
  if (PyList_Check(vec)) {
    // check if its a valid vec<Vector3d>
    int valid = 1;
    for (int i = 0; valid && i < PyList_Size(vec); i++) {
      PyObject *item = PyList_GetItem(vec, i);
      if (!PyList_Check(item)) valid = 0;
    }
    if (valid) {
      for (int j = 0; valid && j < PyList_Size(vec); j++) {
        Vector3d result(0, 0, 0);
        PyObject *item = PyList_GetItem(vec, j);
        if (PyList_Size(item) >= mindim && PyList_Size(item) <= maxdim) {
          for (int i = 0; i < PyList_Size(item); i++) {
            if (PyList_Size(item) > i) {
              if (python_numberval(PyList_GetItem(item, i), &result[i])) return results;  // Error
            }
          }
        }
        results.push_back(result);
      }
      return results;
    }
    Vector3d result(0, 0, 0);
    if (PyList_Size(vec) >= mindim && PyList_Size(vec) <= maxdim) {
      for (int i = 0; i < PyList_Size(vec); i++) {
        if (PyList_Size(vec) > i) {
          if (python_numberval(PyList_GetItem(vec, i), &result[i], dragflags, 1 << i))
            return results;  // Error
        }
      }
    }
    results.push_back(result);
  }
  Vector3d result(0, 0, 0);
  if (!python_numberval(vec, &result[0])) {
    result[1] = result[0];
    result[2] = result[1];
    results.push_back(result);
  }
  return results;  // Error
}

/*
 * Helper function to extract actual values for fn, fa and fs
 */

void get_fnas(double& fn, double& fa, double& fs)
{
  PyObject *mainModule = PyImport_AddModule("__main__");
  if (mainModule == nullptr) return;
  fn = 0;
  fa = 12;
  fs = 2;

  if (PyObject_HasAttrString(mainModule, "fn")) {
    PyObjectUniquePtr varFn(PyObject_GetAttrString(mainModule, "fn"), PyObjectDeleter);
    if (varFn.get() != nullptr) {
      fn = PyFloat_AsDouble(varFn.get());
    }
  }

  if (PyObject_HasAttrString(mainModule, "fa")) {
    PyObjectUniquePtr varFa(PyObject_GetAttrString(mainModule, "fa"), PyObjectDeleter);
    if (varFa.get() != nullptr) {
      fa = PyFloat_AsDouble(varFa.get());
    }
  }

  if (PyObject_HasAttrString(mainModule, "fs")) {
    PyObjectUniquePtr varFs(PyObject_GetAttrString(mainModule, "fs"), PyObjectDeleter);
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
  PyObject *cbfunc = (PyObject *)v_cbfunc;
  Outline2d result;
  if (pythonInitDict == NULL) initPython(PlatformUtils::applicationPath(), "", 0.0);
  PyObject *args = PyTuple_Pack(1, PyFloat_FromDouble(arg));
  PyObject *polygon = PyObject_CallObject(cbfunc, args);
  Py_XDECREF(args);
  if (polygon == NULL) {  // TODO fix
    for (unsigned int i = 0; i < (unsigned int)fn; i++) {
      double ang = 360.0 * (i / (double)fn);
      PyObject *args = PyTuple_Pack(2, PyFloat_FromDouble(arg), PyFloat_FromDouble(ang));
      Py_XINCREF(args);
      PyObject *pypt = PyObject_CallObject(cbfunc, args);
      double r = PyFloat_AsDouble(pypt);
      if (r < 0) r = -r;  // TODO who the hell knows, why this is needed
      double ang1 = ang * 3.1415 / 180.0;
      double x = r * cos(ang1);
      double y = r * sin(ang1);
      result.vertices.push_back(Vector2d(x, y));
    }
  } else if (polygon && PyList_Check(polygon)) {
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
  PyObject *cbfunc = (PyObject *)v_cbfunc;
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
  case Value::Type::UNDEFINED: return Py_None;
  case Value::Type::BOOL:      return val.toBool() ? Py_True : Py_False;
  case Value::Type::NUMBER:    return PyFloat_FromDouble(val.toDouble());
  case Value::Type::STRING:    return PyUnicode_FromString(val.toString().c_str());
  case Value::Type::VECTOR:    {
    const VectorType& vec = val.toVector();
    PyObject *result = PyList_New(vec.size());
    for (size_t j = 0; j < vec.size(); j++) PyList_SetItem(result, j, python_fromopenscad(vec[j]));
    return result;
  }
    // TODO  more types RANGE, OBJECT, FUNCTION
  default: return Py_None;
  }
  return Py_None;
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
    PyObjectUniquePtr str_exc_value(PyObject_Repr(pyExcValue), PyObjectDeleter);
    PyObjectUniquePtr pyExcValueStr(PyUnicode_AsEncodedString(str_exc_value.get(), "utf-8", "~"),
                                    PyObjectDeleter);
    char *suberror = PyBytes_AS_STRING(pyExcValueStr.get());
    if (suberror != nullptr) errorstr += suberror;
    Py_XDECREF(pyExcValue);
  }
  if (pyExcTraceback != nullptr) {
    auto *tb_o = (PyTracebackObject *)pyExcTraceback;
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
  if (dotpos >= 0) {
    std::string varname = name.substr(1, dotpos - 1);  // assume its always in paranthesis
    std::string methodname = name.substr(dotpos + 1, name.size() - dotpos - 2);
    Value var = cxt->lookup_variable(varname, Location::NONE).clone();
    if (var.type() == Value::Type::PYTHONCLASS) {
      const PythonClassType& python_class = var.toPythonClass();
      PyObject *methodobj = PyUnicode_FromString(methodname.c_str());
      pFunc = PyObject_GenericGetAttr((PyObject *)python_class.ptr, methodobj);
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
    Assignment *op_arg = op_args[i].get();

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
  PyObject *dummydict;
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

    if (PyObject_IsInstance(funcresult, reinterpret_cast<PyObject *>(&PyOpenSCADType)))
      result = PyOpenSCADObjectToNode(funcresult, &dummydict);
    Py_XDECREF(funcresult);
    errorstr = "";
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
void initPython(const std::string& binDir, const std::string& scriptpath, double time)
{
  static bool alreadyTried = false;
  if (alreadyTried) return;
  const auto name = PYTHON_EXECUTABLE_NAME;
  const auto exe = binDir + "/" + name;
  if (scriptpath.size() > 0) python_scriptpath = scriptpath;
  if (pythonInitDict) { /* If already initialized, undo to reinitialize after */
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
    while (PyDict_Next(maindict, &pos, &key, &value)) {
      PyObjectUniquePtr key_(PyUnicode_AsEncodedString(key, "utf-8", "~"), PyObjectDeleter);
      if (key_ == nullptr) continue;
      const char *key_str = PyBytes_AS_STRING(key_.get());
      if (key_str == nullptr) continue;
      if (std::find(std::begin(pythonInventory), std::end(pythonInventory), key_str) ==
          std::end(pythonInventory)) {
        if (strlen(key_str) < 4 || strncmp(key_str, "stat", 4) != 0) {
          PyDict_DelItemString(maindict, key_str);
        }
      }
      // bug in  PyDict_GetItemString, thus iterating
      if (strcmp(key_str, "sys") == 0) {
        PyObject *sysdict = PyModule_GetDict(value);
        if (sysdict == nullptr) continue;
        // get builtin_module_names
        PyObject *key1, *value1;
        Py_ssize_t pos1 = 0;
        while (PyDict_Next(sysdict, &pos1, &key1, &value1)) {
          PyObject *key1_ = PyUnicode_AsEncodedString(key1, "utf-8", "~");
          if (key1_ == nullptr) continue;
          const char *key1_str = PyBytes_AS_STRING(key1_);
          if (strcmp(key1_str, "modules") == 0) {
            PyObject *key2, *value2;
            Py_ssize_t pos2 = 0;
            while (PyDict_Next(value1, &pos2, &key2, &value2)) {
              PyObject *key2_ = PyUnicode_AsEncodedString(key2, "utf-8", "~");
              if (key2_ == nullptr) continue;
              const char *key2_str = PyBytes_AS_STRING(key2_);
              if (key2_str == nullptr) continue;
              if (!PyModule_Check(value2)) continue;

              PyObject *modrepr = PyObject_Repr(value2);
              PyObject *modreprobj = PyUnicode_AsEncodedString(modrepr, "utf-8", "~");
              const char *modreprstr = PyBytes_AS_STRING(modreprobj);
              if (modreprstr == nullptr) continue;
              if (strstr(modreprstr, "(frozen)") != nullptr) continue;
              if (strstr(modreprstr, "(built-in)") != nullptr) continue;
              if (strstr(modreprstr, "/encodings/") != nullptr) continue;
              if (strstr(modreprstr, "_frozen_") != nullptr) continue;
              if (strstr(modreprstr, "site-packages") != nullptr) continue;
              if (strstr(modreprstr, "usr/lib") != nullptr) continue;

              //  PyObject *mod_dict = PyModule_GetDict(value2);
              //  PyObject *loader = PyDict_GetItemString(mod_dict,"__loader__");
              //  PyObject *loaderrepr = PyObject_Repr(loader);
              //  PyObject* loaderreprobj = PyUnicode_AsEncodedString(loaderrepr, "utf-8", "~");
              //  const char *loaderreprstr = PyBytes_AS_STRING(loaderreprobj);
              //  if(strstr(loaderreprstr, "ExtensionFileLoader") != nullptr) continue; // dont delete
              //  extension files

              PyDict_DelItem(value1, key2);
            }
          }
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
    PyImport_AppendInittab("openscad", &PyInit_PyOpenSCAD);
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
    fs::path scriptfile(python_scriptpath);
    stream << sep << PlatformUtils::userLibraryPath();
    stream << sep << scriptfile.parent_path().string();
    stream << sepchar << ".";
    PyConfig_SetBytesString(&config, &config.pythonpath_env, stream.str().c_str());

    PyConfig_SetBytesString(&config, &config.program_name, name);
    PyConfig_SetBytesString(&config, &config.executable, exe.c_str());

    PyStatus status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
      alreadyTried = true;
      LOG(message_group::Error, "Python %1$lu.%2$lu.%3$lu not found. Is it installed ?",
          PY_MAJOR_VERSION, PY_MINOR_VERSION, PY_MICRO_VERSION);
      return;
    }
    PyConfig_Clear(&config);

    pythonMainModule.reset(PyImport_AddModule("__main__"));
    pythonMainModuleInitialized = pythonMainModule != nullptr;
    pythonInitDict.reset(PyModule_GetDict(pythonMainModule.get()));
    pythonRuntimeInitialized = pythonInitDict != nullptr;
    PyInit_PyData();
    PyRun_String("from builtins import *\n", Py_file_input, pythonInitDict.get(), pythonInitDict.get());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
    while (PyDict_Next(maindict, &pos, &key, &value)) {
      PyObjectUniquePtr key1(PyUnicode_AsEncodedString(key, "utf-8", "~"), PyObjectDeleter);
      const char *key_str = PyBytes_AsString(key1.get());
      if (key_str != NULL) pythonInventory.push_back(key_str);
    }
  }
  std::ostringstream stream;
  stream << "t=" << time << "\nphi=" << 2 * G_PI * time;
  PyRun_String(stream.str().c_str(), Py_file_input, pythonInitDict.get(), pythonInitDict.get());
  customizer_parameters_finished = customizer_parameters;
  customizer_parameters.clear();
  python_result_handle.clear();
  nodes_hold.clear();
  DECLARE_INSTANCE
  void_node = std::make_shared<CubeNode>(instance);  // just placeholders
  full_node = std::make_shared<CubeNode>(instance);  // just placeholders
}

void finishPython(void)
{
#ifdef HAVE_PYTHON_YIELD
  set_object_callback(NULL);
  if (python_result_node == nullptr) {
    if (python_orphan_objs.size() == 1) {
      python_result_node = PyOpenSCADObjectToNode(python_orphan_objs[0]);
    } else if (python_orphan_objs.size() > 1) {
      DECLARE_INSTANCE
      auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
      int n = python_orphan_objs.size();
      for (int i = 0; i < n; i++) {
        std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(python_orphan_objs[i]);
        node->children.push_back(child);
      }
      python_result_node = node;
    }
  }
#endif
  show_final();
}

std::string evaluatePython(const std::string& code, bool dry_run)
{
  std::string error;
  genlang_result_node = nullptr;
  python_result_handle.clear();
  PyObjectUniquePtr pyExcValue(nullptr, PyObjectDeleter);
  PyObjectUniquePtr pyExcTraceback(nullptr, PyObjectDeleter);
  /* special python code to catch errors from stdout and stderr and make them available in OpenSCAD
   * console */
  for (ModuleInstantiation *mi : modinsts_list) {
    delete mi;  // best time to delete it
  }
  modinsts_list.clear();
  pythonDryRun = dry_run;
  if (!pythonMainModuleInitialized) return "Python not initialized";
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

#ifndef OPENSCAD_NOGUI
  PyRun_SimpleString(python_init_code);
#endif
#ifdef HAVE_PYTHON_YIELD
  for (auto obj : python_orphan_objs) {
    Py_DECREF(obj);
  }
  python_orphan_objs.clear();
#endif
  PyObjectUniquePtr result(nullptr, PyObjectDeleter);
  result.reset(PyRun_String(code.c_str(), Py_file_input, pythonInitDict.get(),
                            pythonInitDict.get())); /* actual code is run here */

#ifndef OPENSCAD_NOGUI
  if (result == nullptr) {
    PyErr_Print();
    error = "";
    python_catch_error(error);
  }
  for (int i = 0; i < 2; i++) {
    PyObjectUniquePtr catcher(nullptr, PyObjectDeleter);
    catcher.reset(
      PyObject_GetAttrString(pythonMainModule.get(), i == 1 ? "catcher_err" : "catcher_out"));
    if (catcher == nullptr) continue;
    PyObjectUniquePtr command_output(nullptr, PyObjectDeleter);
    command_output.reset(PyObject_GetAttrString(catcher.get(), "data"));

    PyObjectUniquePtr command_output_value(nullptr, PyObjectDeleter);
    command_output_value.reset(PyUnicode_AsEncodedString(command_output.get(), "utf-8", "~"));
    const char *command_output_bytes = PyBytes_AS_STRING(command_output_value.get());
    if (command_output_bytes != nullptr && *command_output_bytes != '\0') {
      if (i == 1) error += command_output_bytes; /* output to console */
      else LOG(command_output_bytes);            /* error to LOG */
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

typedef struct {
  PyObject_HEAD PyObject *container;  // Referenz auf das Original-Objekt
  Py_ssize_t index;                   // Aktueller Index
} PyOpenSCADObjectIter;

extern PyTypeObject PyOpenSCADObjectIterType;

PyObject *PyOpenSCADType_iter(PyObject *self)
{
  PyOpenSCADObjectIter *iter =
    (PyOpenSCADObjectIter *)PyObject_New(PyOpenSCADObjectIter, &PyOpenSCADObjectIterType);
  if (iter == NULL) {
    return NULL;
  }
  Py_INCREF(iter);

  Py_INCREF(self);
  iter->container = self;
  iter->index = 0;

  return (PyObject *)iter;
}

PyObject *PyOpenSCADType_next(PyObject *self)
{
  PyOpenSCADObjectIter *iter = (PyOpenSCADObjectIter *)self;
  PyOpenSCADObject *container = (PyOpenSCADObject *)iter->container;

  // Prüfe ob noch Elemente vorhanden
  Py_ssize_t size = container->node->children.size();
  if (iter->index >= size) {
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
  }

  // Nächstes Element holen
  auto node = container->node->children[iter->index];
  iter->index++;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

// Iterator dealloc
void PyOpenSCADObjectIter_dealloc(PyOpenSCADObjectIter *self)
{
  PyOpenSCADObjectIter *iter = (PyOpenSCADObjectIter *)self;
  Py_XDECREF(iter->container);
  PyObject_Del(self);
}

// Iterator Type Definition
PyTypeObject PyOpenSCADObjectIterType = {
  PyVarObject_HEAD_INIT(NULL, 0).tp_name = "PyOpenSCADType.Iterator",
  .tp_basicsize = sizeof(PyOpenSCADObjectIter),
  .tp_dealloc = (destructor)PyOpenSCADObjectIter_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_iter = PyOpenSCADType_iter,
  .tp_iternext = PyOpenSCADType_next,  // <-- __next__ Implementierung
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
  0,                                                       /* tp_as_sequence */
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
  0,                                                       /* tp_alloc */
  PyOpenSCADObject_new,                                    /* tp_new */
};

static PyModuleDef OpenSCADModule = {PyModuleDef_HEAD_INIT,
                                     "openscad",
                                     "OpenSCAD Python Module",
                                     -1,
                                     PyOpenSCADFunctions,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL};

extern "C" PyObject *PyInit_openscad(void) { return PyModule_Create(&OpenSCADModule); }

PyMODINIT_FUNC PyInit_PyOpenSCAD(void)
{
  PyObject *m;

  if (PyType_Ready(&PyOpenSCADType) < 0) return NULL;

  m = PyInit_openscad();
  if (m == NULL) return NULL;

  Py_INCREF(&PyOpenSCADType);
  PyModule_AddObject(m, "Openscad", (PyObject *)&PyOpenSCADType);
  return m;
}

// ----------------------------------------------
// IPython Interpreter side
// ----------------------------------------------

static PyStatus pymain_init_ipython(void)
{
  PyStatus status;

  //    if (_PyStatus_EXCEPTION(status)) {
  //        return status;
  //    }

  PyPreConfig preconfig;
  PyPreConfig_InitPythonConfig(&preconfig);
  //    status = _Py_PreInitializeFromPyArgv(&preconfig, args);
  //    if (_PyStatus_EXCEPTION(status)) {
  //        return status;
  //    }

  PyConfig config;
  PyConfig_InitPythonConfig(&config);

  //    if (args->use_bytes_argv) {
  //        status = PyConfig_SetBytesArgv(&config, args->argc, args->bytes_argv);
  //    }
  //    else {
  //        status = PyConfig_SetArgv(&config, args->argc, args->wchar_argv);
  //    }
  //    if (_PyStatus_EXCEPTION(status)) {
  //        goto done;
  //    }

  status = Py_InitializeFromConfig(&config);
  //    if (_PyStatus_EXCEPTION(status)) {
  //        goto done;
  //    }
  //    status = 0; // PyStatus_Ok;

  PyConfig_Clear(&config);
  return status;
}

/* Write an exitcode into *exitcode and return 1 if we have to exit Python.
   Return 0 otherwise. */
static int pymain_run_interactive_hook_ipython(int *exitcode)
{
  PyObject *sys, *hook, *result;
  sys = PyImport_ImportModule("sys");
  if (sys == NULL) {
    goto error;
  }

  hook = PyObject_GetAttrString(sys, "__interactivehook__");
  Py_DECREF(sys);
  if (hook == NULL) {
    PyErr_Clear();
    return 0;
  }

  if (PySys_Audit("cpython.run_interactivehook", "O", hook) < 0) {
    goto error;
  }
  result = PyObject_CallNoArgs(hook);
  Py_DECREF(hook);
  if (result == NULL) {
    goto error;
  }
  Py_DECREF(result);
  return 0;

error:
  PySys_WriteStderr("Failed calling sys.__interactivehook__\n");
  //    return pymain_err_print(exitcode);
  return 0;
}

static void pymain_repl_ipython(int *exitcode)
{
  if (pymain_run_interactive_hook_ipython(exitcode)) {
    return;
  }
  PyCompilerFlags cf = _PyCompilerFlags_INIT;

  PyRun_AnyFileFlags(stdin, "<stdin>", &cf);
}

static void pymain_run_python_ipython(int *exitcode)
{
  PyObject *main_importer_path = NULL;
  //    PyInterpreterState *interp = PyInterpreterState_Get();

  pymain_repl_ipython(exitcode);
  goto done;

  //    *exitcode = pymain_exit_err_print();

done:
  //    _PyInterpreterState_SetNotRunningMain(interp);
  Py_XDECREF(main_importer_path);
}

int Py_RunMain_ipython(void)
{
  int exitcode = 0;

  pymain_run_python_ipython(&exitcode);

  if (Py_FinalizeEx() < 0) {
      exitcode = 120;
  }

  //    pymain_free();

  //    if (_PyRuntime.signals.unhandled_keyboard_interrupt) {
  //        exitcode = exit_sigint();
  //    }

  return exitcode;
}

void ipython(void)
{
  initPython(PlatformUtils::applicationPath(), "", 0.0);
  Py_RunMain_ipython();
  return;
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
