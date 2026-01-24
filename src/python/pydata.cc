// Author: Sohler Guenther
// Date: 2023-01-01
// Purpose: Extend openscad with an python interpreter

#include <Python.h>
#include "pydata.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "Assignment.h"
#include "ModuleInstantiation.h"
#include "BuiltinContext.h"
#include "Expression.h"
#include "pyopenscad.h"
#include "genlang/genlang.h"
#include "core/Parameters.h"
#include "core/Arguments.h"
#include "core/function.h"

#include "../src/geometry/GeometryEvaluator.h"
#include "../src/core/primitives.h"
#include "core/Tree.h"
#include <PolySet.h>
#include <PolySetUtils.h>
#ifdef ENABLE_LIBFIVE
#include <libfive/tree/opcode.hpp>
#endif

// https://docs.python.it/html/ext/dnt-basics.html

void PyDataObject_dealloc(PyObject *self)
{
  //  Py_XDECREF(self->dict);
  //  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyDataObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyDataObject *self;
  self = (PyDataObject *)type->tp_alloc(type, 0);
  self->data_type = DATA_TYPE_UNKNOWN;
  self->data = 0;
  Py_XINCREF(self);
  return (PyObject *)self;
}

PyObject *PyDataObjectFromModule(PyTypeObject *type, std::string modulepath, std::string modulename)
{
  std::string result = modulepath + "|" + modulename;
  PyDataObject *res;
  res = (PyDataObject *)type->tp_alloc(type, 0);
  if (res != NULL) {
    res->data_type = DATA_TYPE_SCADMODULE;
    res->data = (void *)strdup(result.c_str());  // TODO memory leak!
    Py_XINCREF(res);
    return (PyObject *)res;
  }
  return Py_None;
}

void PyDataObjectToModule(PyObject *obj, std::string& modulepath, std::string& modulename)
{
  if (obj != NULL && obj->ob_type == &PyDataType) {
    PyDataObject *dataobj = (PyDataObject *)obj;
    if (dataobj->data_type == DATA_TYPE_SCADMODULE) {
      std::string result((char *)dataobj->data);
      int sep = result.find("|");
      if (sep >= 0) {
        modulepath = result.substr(0, sep);
        modulename = result.substr(sep + 1);
      }
    }
  }
}

PyObject *PyDataObjectFromFunction(PyTypeObject *type, std::string functionpath,
                                   std::string functionname)
{
  std::string result = functionpath + "|" + functionname;
  PyDataObject *res;
  res = (PyDataObject *)type->tp_alloc(type, 0);
  if (res != NULL) {
    res->data_type = DATA_TYPE_SCADFUNCTION;
    res->data = (void *)strdup(result.c_str());  // TODO memory leak!
    Py_XINCREF(res);
    return (PyObject *)res;
  }
  return Py_None;
}

void PyDataObjectToFunction(PyObject *obj, std::string& functionpath, std::string& functionname)
{
  if (obj != NULL && obj->ob_type == &PyDataType) {
    PyDataObject *dataobj = (PyDataObject *)obj;
    if (dataobj->data_type == DATA_TYPE_SCADFUNCTION) {
      std::string result((char *)dataobj->data);
      int sep = result.find("|");
      if (sep >= 0) {
        functionpath = result.substr(0, sep);
        functionname = result.substr(sep + 1);
      }
    }
  }
}

PyObject *PyDataObjectFromValue(PyTypeObject *type, double value)
{
  PyDataObject *res;
  double *ptr = (double *)malloc(sizeof(value));  // TODO memory leak
  *ptr = value;
  res = (PyDataObject *)type->tp_alloc(type, 0);
  if (res != NULL) {
    res->data_type = DATA_TYPE_MARKEDVALUE;
    res->data = (void *)ptr;
    Py_XINCREF(res);
    return (PyObject *)res;
  }
  return Py_None;
}

double PyDataObjectToValue(PyObject *obj)
{
  if (obj != NULL && obj->ob_type == &PyDataType) {
    PyDataObject *dataobj = (PyDataObject *)obj;
    if (dataobj->data_type == DATA_TYPE_MARKEDVALUE) {
      double *val = (double *)dataobj->data;
      return *val;
    }
  }
  return 0;
}

PyObject *python_data_str(PyObject *self)
{
  std::ostringstream stream;
  PyDataObject *data = (PyDataObject *)self;
  switch (data->data_type) {
  case DATA_TYPE_LIBFIVE:      stream << "Libfive Tree"; break;
  case DATA_TYPE_SCADMODULE:   stream << "SCAD Class Module"; break;
  case DATA_TYPE_SCADFUNCTION: stream << "SCAD Class Function"; break;
  case DATA_TYPE_MARKEDVALUE:  stream << "Marked Value (" << PyDataObjectToValue(self) << ")"; break;
  }
  return PyUnicode_FromStringAndSize(stream.str().c_str(), stream.str().size());
}

#ifdef ENABLE_LIBFIVE

PyObject *PyDataObjectFromTree(PyTypeObject *type, const std::vector<libfive::Tree *>& tree)
{
  if (tree.size() == 0) {
    return Py_None;
  } else if (tree.size() == 1) {
    PyDataObject *res;
    res = (PyDataObject *)type->tp_alloc(type, 0);
    if (res != NULL) {
      res->data = (void *)tree[0];
      res->data_type = DATA_TYPE_LIBFIVE;
      Py_XINCREF(res);
      return (PyObject *)res;
    }
  } else {
    PyObject *res = PyTuple_New(tree.size());
    for (int i = 0; i < tree.size(); i++) {
      PyDataObject *sub;
      sub = (PyDataObject *)type->tp_alloc(type, 0);
      if (sub != NULL) {
        sub->data = tree[i];
        sub->data_type = DATA_TYPE_LIBFIVE;
        Py_XINCREF(sub);
        PyTuple_SetItem(res, i, (PyObject *)sub);
      }
    }
    return res;
  }

  return Py_None;
}

std::vector<libfive::Tree *> PyDataObjectToTree(PyObject *obj)
{
  std::vector<libfive::Tree *> result;
  if (obj != NULL && obj->ob_type == &PyDataType) {
    PyDataObject *dataobj = (PyDataObject *)obj;
    if (dataobj->data_type == DATA_TYPE_LIBFIVE) result.push_back((libfive::Tree *)dataobj->data);
  } else if (PyLong_Check(obj)) {
    result.push_back(new libfive::Tree(libfive::Tree(PyLong_AsLong(obj))));
  } else if (PyFloat_Check(obj)) {
    result.push_back(new libfive::Tree(libfive::Tree(PyFloat_AsDouble(obj))));
  } else if (PyTuple_Check(obj)) {
    for (int i = 0; i < PyTuple_Size(obj); i++) {
      PyObject *x = PyTuple_GetItem(obj, i);
      std::vector<libfive::Tree *> sub = PyDataObjectToTree(x);
      result.insert(result.end(), sub.begin(), sub.end());
    }
  } else if (PyList_Check(obj)) {
    for (int i = 0; i < PyList_Size(obj); i++) {
      PyObject *x = PyList_GetItem(obj, i);
      std::vector<libfive::Tree *> sub = PyDataObjectToTree(x);
      result.insert(result.end(), sub.begin(), sub.end());
    }
  } else {
    printf("Unknown type! %p %p\n", obj->ob_type, &PyFloat_Type);
  }
  //  Py_XDECREF(obj); TODO cannot activate
  return result;
}

#endif
static int PyDataInit(PyDataObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

#ifdef ENABLE_LIBFIVE
PyObject *python_lv_void_int(PyObject *self, PyObject *args, PyObject *kwargs, libfive::Tree *t)
{
  std::vector<libfive::Tree *> vec;
  vec.push_back(t);
  return PyDataObjectFromTree(&PyDataType, vec);
}

PyObject *python_lv_un_int(PyObject *self, PyObject *args, PyObject *kwargs, libfive::Opcode::Opcode op)
{
  char *kwlist[] = {"arg", NULL};
  PyObject *arg = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &arg)) return NULL;

  std::vector<libfive::Tree *> tv = PyDataObjectToTree(arg);
  std::vector<libfive::Tree *> res;
  for (int i = 0; i < tv.size(); i++) {
    libfive::Tree *sub = new libfive::Tree(libfive::Tree::unary(op, *(tv[i])));
    res.push_back(sub);
  }
  return PyDataObjectFromTree(&PyDataType, res);
}

PyObject *python_lv_bin_int(PyObject *self, PyObject *args, PyObject *kwargs, libfive::Opcode::Opcode op)
{
#if 1
  char *kwlist[] = {"arg1", "arg2", NULL};
  PyObject *arg1 = NULL;
  PyObject *arg2 = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &arg1, &arg2)) return NULL;

  std::vector<libfive::Tree *> a1 = PyDataObjectToTree(arg1);
  std::vector<libfive::Tree *> a2 = PyDataObjectToTree(arg2);
  std::vector<libfive::Tree *> res;
  if (a1.size() == a2.size()) {
    for (int i = 0; i < a1.size(); i++) {
      res.push_back(new libfive::Tree(libfive::Tree::binary(op, *a1[i], *a2[i])));
    }
  } else if (a2.size() == 1) {
    for (int i = 0; i < a1.size(); i++) {
      res.push_back(new libfive::Tree(libfive::Tree::binary(op, *a1[i], *a2[0])));
    }
  } else {
    printf("Cannot handle  bin %lu binop %lu \n", a1.size(), a2.size());
    return Py_None;
  }
#else
  int i;
  PyObject *obj = NULL;
  if (args == NULL) return Py_None;
  if (PyTuple_Size(args) == 0) return Py_None;
  obj = PyTuple_GetItem(args, 0);
  //  Py_INCREF(obj);
  Tree res = PyDataObjectToTree(obj);
  for (i = 1; i < PyTuple_Size(args); i++) {
    obj = PyTuple_GetItem(args, i);
    // Py_INCREF(obj);
    Tree tmp = PyDataObjectToTree(obj);
    Tree res = libfive_tree_binary(op, res, tmp);
  }
#endif
  return PyDataObjectFromTree(&PyDataType, res);
}

PyObject *python_lv_unop_int(PyObject *arg, libfive::Opcode::Opcode op)
{
  std::vector<libfive::Tree *> t = PyDataObjectToTree(arg);
  std::vector<libfive::Tree *> res;
  for (int i = 0; i < t.size(); i++) {
    res.push_back(new libfive::Tree(libfive::Tree::unary(op, *(t[i]))));
  }
  return PyDataObjectFromTree(&PyDataType, res);
}

PyObject *python_lv_binop_int(PyObject *arg1, PyObject *arg2, libfive::Opcode::Opcode op)
{
  std::vector<libfive::Tree *> t1 = PyDataObjectToTree(arg1);
  std::vector<libfive::Tree *> t2 = PyDataObjectToTree(arg2);

  std::vector<libfive::Tree *> res;
  if (t1.size() == t2.size()) {
    for (int i = 0; i < t1.size(); i++) {
      res.push_back(new libfive::Tree(libfive::Tree::binary(op, *(t1[i]), *(t2[i]))));
    }
  } else if (t2.size() == 1) {
    for (int i = 0; i < t1.size(); i++) {
      res.push_back(new libfive::Tree(libfive::Tree::binary(op, *(t1[i]), *(t2[0]))));
    }
  } else {
    printf("Cannot handle bin %lu binop %lu \n", t1.size(), t2.size());
    return Py_None;
  }

  return PyDataObjectFromTree(&PyDataType, res);
}

libfive::Tree lv_x = libfive::Tree::X();
libfive::Tree lv_y = libfive::Tree::Y();
libfive::Tree lv_z = libfive::Tree::Z();
PyObject *python_lv_x(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_void_int(self, args, kwargs, &lv_x);
}
PyObject *python_lv_y(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_void_int(self, args, kwargs, &lv_y);
}
PyObject *python_lv_z(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_void_int(self, args, kwargs, &lv_z);
}
PyObject *python_lv_sqrt(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_SQRT);
}
PyObject *python_lv_square(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_SQUARE);
}
PyObject *python_lv_abs(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_ABS);
}
PyObject *python_lv_max(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_bin_int(self, args, kwargs, libfive::Opcode::OP_MAX);
}
PyObject *python_lv_min(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_bin_int(self, args, kwargs, libfive::Opcode::OP_MIN);
}
PyObject *python_lv_pow(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_bin_int(self, args, kwargs, libfive::Opcode::OP_POW);
}
PyObject *python_lv_comp(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_bin_int(self, args, kwargs, libfive::Opcode::OP_COMPARE);
}
PyObject *python_lv_atan2(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_bin_int(self, args, kwargs, libfive::Opcode::OP_ATAN2);
}

PyObject *python_lv_sin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_SIN);
}
PyObject *python_lv_cos(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_COS);
}
PyObject *python_lv_tan(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_TAN);
}
PyObject *python_lv_asin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_ASIN);
}
PyObject *python_lv_acos(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_ACOS);
}
PyObject *python_lv_atan(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_ATAN);
}
PyObject *python_lv_exp(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_EXP);
}
PyObject *python_lv_log(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_lv_un_int(self, args, kwargs, libfive::Opcode::OP_LOG);
}

PyObject *python_lv_print(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"arg", NULL};
  PyObject *arg = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyDataType, &arg)) return NULL;
  std::vector<libfive::Tree *> tv = PyDataObjectToTree(arg);
  for (int i = 0; i < tv.size(); i++) {
    //    printf("tree %d: %s\n",i,libfive_tree_print(tv[i]));
  }
  return Py_None;
}

#endif

static PyMethodDef PyDataFunctions[] = {
#ifdef ENABLE_LIBFIVE
  {"x", (PyCFunction)python_lv_x, METH_VARARGS | METH_KEYWORDS, "Get X."},
  {"y", (PyCFunction)python_lv_y, METH_VARARGS | METH_KEYWORDS, "Get Y."},
  {"z", (PyCFunction)python_lv_z, METH_VARARGS | METH_KEYWORDS, "Get Z."},
  {"sqrt", (PyCFunction)python_lv_sqrt, METH_VARARGS | METH_KEYWORDS, "Square Root"},
  {"square", (PyCFunction)python_lv_square, METH_VARARGS | METH_KEYWORDS, "Square"},
  {"abs", (PyCFunction)python_lv_abs, METH_VARARGS | METH_KEYWORDS, "Absolute"},
  {"max", (PyCFunction)python_lv_max, METH_VARARGS | METH_KEYWORDS, "Maximal"},
  {"min", (PyCFunction)python_lv_min, METH_VARARGS | METH_KEYWORDS, "Minimal"},

  {"sin", (PyCFunction)python_lv_sin, METH_VARARGS | METH_KEYWORDS, "Sin"},
  {"cos", (PyCFunction)python_lv_cos, METH_VARARGS | METH_KEYWORDS, "Cos"},
  {"tan", (PyCFunction)python_lv_tan, METH_VARARGS | METH_KEYWORDS, "Tan"},
  {"asin", (PyCFunction)python_lv_asin, METH_VARARGS | METH_KEYWORDS, "Asin"},
  {"acos", (PyCFunction)python_lv_acos, METH_VARARGS | METH_KEYWORDS, "Acos"},
  {"atan", (PyCFunction)python_lv_atan, METH_VARARGS | METH_KEYWORDS, "Atan"},
  {"exp", (PyCFunction)python_lv_exp, METH_VARARGS | METH_KEYWORDS, "Exp"},
  {"log", (PyCFunction)python_lv_log, METH_VARARGS | METH_KEYWORDS, "Log"},
  {"pow", (PyCFunction)python_lv_pow, METH_VARARGS | METH_KEYWORDS, "Power"},
  {"comp", (PyCFunction)python_lv_comp, METH_VARARGS | METH_KEYWORDS, "Compare"},
  {"atan2", (PyCFunction)python_lv_atan2, METH_VARARGS | METH_KEYWORDS, "Atan2"},
  {"print", (PyCFunction)python_lv_print, METH_VARARGS | METH_KEYWORDS, "Print"},
#endif
  {NULL, NULL, 0, NULL}};

#ifdef ENABLE_LIBFIVE
PyObject *python_lv_add(PyObject *arg1, PyObject *arg2)
{
  return python_lv_binop_int(arg1, arg2, libfive::Opcode::OP_ADD);
}
PyObject *python_lv_substract(PyObject *arg1, PyObject *arg2)
{
  return python_lv_binop_int(arg1, arg2, libfive::Opcode::OP_SUB);
}
PyObject *python_lv_multiply(PyObject *arg1, PyObject *arg2)
{
  return python_lv_binop_int(arg1, arg2, libfive::Opcode::OP_MUL);
}
PyObject *python_lv_remainder(PyObject *arg1, PyObject *arg2)
{
  return python_lv_binop_int(arg1, arg2, libfive::Opcode::OP_MOD);
}
PyObject *python_lv_divide(PyObject *arg1, PyObject *arg2)
{
  return python_lv_binop_int(arg1, arg2, libfive::Opcode::OP_DIV);
}
PyObject *python_lv_negate(PyObject *arg)
{
  return python_lv_unop_int(arg, libfive::Opcode::OP_NEG);
}
#else
PyObject *python_lv_add(PyObject *arg1, PyObject *arg2)
{
  return Py_None;
}
PyObject *python_lv_substract(PyObject *arg1, PyObject *arg2)
{
  return Py_None;
}
PyObject *python_lv_multiply(PyObject *arg1, PyObject *arg2)
{
  return Py_None;
}
PyObject *python_lv_remainder(PyObject *arg1, PyObject *arg2)
{
  return Py_None;
}
PyObject *python_lv_divide(PyObject *arg1, PyObject *arg2)
{
  return Py_None;
}
PyObject *python_lv_negate(PyObject *arg)
{
  return Py_None;
}
#endif

Value python_convertresult(PyObject *arg, int& error);

extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename,
                  const std::string& mainFile, int debug);

PyObject *PyDataObject_call_module(PyObject *self, PyObject *args, PyObject *kwargs)
{
  if (pythonDryRun) {
    DECLARE_INSTANCE();
    auto empty = std::make_shared<CubeNode>(instance);
    return PyOpenSCADObjectFromNode(&PyOpenSCADType, empty);
  }
  std::string modulepath, modulename;
  PyDataObjectToModule(self, modulepath, modulename);
  AssignmentList pargs;
  std::vector<std::shared_ptr<ModuleInstantiation>> modinsts;
  int error;
  for (int i = 0; i < PyTuple_Size(args); i++) {
    PyObject *arg = PyTuple_GetItem(args, i);
    if (PyObject_IsInstance(arg, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
      std::shared_ptr<AbstractNode> child = ((PyOpenSCADObject *)arg)->node;
      Tree tree(child, "");
      GeometryEvaluator geomevaluator(tree);
      std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
      std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
      if (ps != nullptr) {
        // prepare vertices
        auto vecs3d = std::make_shared<Vector>(Location::NONE);
        for (auto vertex : ps->vertices) {
          auto vec3d = new Vector(Location::NONE);
          vec3d->emplace_back(new Literal(vertex[0], Location::NONE));
          vec3d->emplace_back(new Literal(vertex[1], Location::NONE));
          vec3d->emplace_back(new Literal(vertex[2], Location::NONE));

          vecs3d->emplace_back(vec3d);
        }
        std::shared_ptr<Assignment> ass_pts =
          std::make_shared<Assignment>(std::string("points"), vecs3d);

        // prepare indices
        auto inds3d = std::make_shared<Vector>(Location::NONE);
        for (auto face : ps->indices) {
          auto ind3d = new Vector(Location::NONE);
          for (auto it = face.crbegin(); it != face.crend(); ++it) {
            ind3d->emplace_back(new Literal(*it, Location::NONE));
          }
          inds3d->emplace_back(ind3d);
        }
        std::shared_ptr<Assignment> ass_faces =
          std::make_shared<Assignment>(std::string("faces"), inds3d);

        AssignmentList poly_asslist;
        poly_asslist.push_back(ass_pts);
        poly_asslist.push_back(ass_faces);

        auto poly_modinst =
          std::make_shared<ModuleInstantiation>("polyhedron", poly_asslist, Location::NONE);
        modinsts.push_back(poly_modinst);
      }
    } else {
      Value val = python_convertresult(arg, error);
      std::shared_ptr<Literal> lit = std::make_shared<Literal>(std::move(val), Location::NONE);
      std::shared_ptr<Assignment> ass = std::make_shared<Assignment>(std::string(""), lit);
      pargs.push_back(ass);
    }
  }
  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      std::string value_str = PyBytes_AS_STRING(value1);
      if (value_str == "fn") value_str = "$fn";
      if (value_str == "fa") value_str = "$fa";
      if (value_str == "fs") value_str = "$fs";
      Value val = python_convertresult(value, error);
      std::shared_ptr<Literal> lit = std::make_shared<Literal>(std::move(val), Location::NONE);
      std::shared_ptr<Assignment> ass = std::make_shared<Assignment>(value_str, lit);
      pargs.push_back(ass);
    }
  }

  std::shared_ptr<ModuleInstantiation> modinst =
    std::make_shared<ModuleInstantiation>(modulename, pargs, Location::NONE);
  modinst->scope->moduleInstantiations = modinsts;

  std::ostringstream stream;
  stream << "include <" << modulepath << ">";

  SourceFile *source;
  if (!parse(source, stream.str(), "python", "python", false)) {
    PyErr_SetString(PyExc_TypeError, "Error in SCAD code");
    return Py_None;
  }
  source->handleDependencies(true);

  EvaluationSession session{"python"};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  std::shared_ptr<const FileContext> dummy_context;
  source->scope->moduleInstantiations.clear();
  source->scope->moduleInstantiations.push_back(modinst);
  std::shared_ptr<AbstractNode> resultnode = source->instantiate(*builtin_context, &dummy_context);
  nodes_hold.push_back(resultnode);  // dirty hacks so resultnode does not go out of context
  resultnode = resultnode->clone();  // use own ModuleInstatiation
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, resultnode);
}

PyObject *PyDataObject_call_function(PyObject *self, PyObject *args, PyObject *kwargs)
{
  if (pythonDryRun) {
    return Py_None;
  }

  std::string functionpath, functionname;
  PyDataObjectToFunction(self, functionpath, functionname);

  // Convert Python args/kwargs to OpenSCAD AssignmentList
  AssignmentList pargs;
  int error;

  // Handle positional args
  for (int i = 0; i < PyTuple_Size(args); i++) {
    PyObject *arg = PyTuple_GetItem(args, i);
    Value val = python_convertresult(arg, error);
    std::shared_ptr<Literal> lit = std::make_shared<Literal>(std::move(val), Location::NONE);
    std::shared_ptr<Assignment> ass = std::make_shared<Assignment>(std::string(""), lit);
    pargs.push_back(ass);
  }

  // Handle keyword args
  if (kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      std::string value_str = PyBytes_AS_STRING(value1);
      Value val = python_convertresult(value, error);
      std::shared_ptr<Literal> lit = std::make_shared<Literal>(std::move(val), Location::NONE);
      std::shared_ptr<Assignment> ass = std::make_shared<Assignment>(value_str, lit);
      pargs.push_back(ass);
    }
  }

  // Parse the file to get function definition
  std::ostringstream stream;
  stream << "include <" << functionpath << ">";

  // Use the function's file path as the source file for proper Location tracking
  fs::path funcFilePath(functionpath);
  std::string parentPath = funcFilePath.parent_path().string();

  SourceFile *source;
  if (!parse(source, stream.str(), parentPath, parentPath, false)) {
    PyErr_SetString(PyExc_TypeError, "Error parsing SCAD file for function");
    return Py_None;
  }
  source->handleDependencies(true);

  // Create evaluation context with proper document path
  EvaluationSession session{parentPath};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  std::shared_ptr<const FileContext> file_context;
  source->instantiate(*builtin_context, &file_context);

  // Find the function in the scope
  auto func_it = source->scope->functions.find(functionname);
  if (func_it == source->scope->functions.end()) {
    PyErr_SetString(PyExc_TypeError, "Function not found in SCAD file");
    return Py_None;
  }

  std::shared_ptr<UserFunction> userfunc = func_it->second;

  // Create body context from file context
  ContextHandle<Context> body_context{Context::create<Context>(file_context)};
  body_context->apply_config_variables(**builtin_context);

  // Parse arguments against function parameters
  Arguments arguments{pargs, *builtin_context};
  Parameters parameters =
    Parameters::parse(std::move(arguments), Location::NONE, userfunc->parameters, file_context);
  body_context->apply_variables(std::move(parameters).to_context_frame());

  // Evaluate function expression
  Value result_val = userfunc->expr->evaluate(*body_context);

  // Convert OpenSCAD Value to Python
  PyObject *result = python_fromopenscad(result_val);

  return result;
}

PyObject *PyDataObject_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyDataObject *dataobj = (PyDataObject *)self;

  if (dataobj->data_type == DATA_TYPE_SCADMODULE) {
    return PyDataObject_call_module(self, args, kwargs);
  } else if (dataobj->data_type == DATA_TYPE_SCADFUNCTION) {
    return PyDataObject_call_function(self, args, kwargs);
  }

  PyErr_SetString(PyExc_TypeError, "PyDataObject is not callable");
  return Py_None;
}

PyNumberMethods PyDataNumbers = {
  &python_lv_add,
  &python_lv_substract,
  &python_lv_multiply,
  &python_lv_remainder,
  0, /* binaryfunc nb_divmod; */
  0, /* ternaryfunc nb_power; */
  &python_lv_negate,
  0, /* unaryfunc nb_positive; */
  0, /* unaryfunc nb_absolute; */
  0, /* inquiry nb_bool; */
  0, /* unaryfunc nb_invert; */
  0, /* binaryfunc nb_lshift; */
  0, /* binaryfunc nb_rshift; */
  0, /* binaryfunc nb_and; */
  0, /* binaryfunc nb_xor; */
  0, /* binaryfunc nb_or; */
  0, /* unaryfunc nb_int; */
  0, /* void *nb_reserved; */
  0, /* unaryfunc nb_float; */

  0, /* binaryfunc nb_inplace_add; */
  0, /* binaryfunc nb_inplace_subtract; */
  0, /* binaryfunc nb_inplace_multiply; */
  0, /* binaryfunc nb_inplace_remainder; */
  0, /* ternaryfunc nb_inplace_power; */
  0, /* binaryfunc nb_inplace_lshift; */
  0, /* binaryfunc nb_inplace_rshift; */
  0, /* binaryfunc nb_inplace_and; */
  0, /* binaryfunc nb_inplace_xor; */
  0, /* binaryfunc nb_inplace_or; */

  0,                 /* binaryfunc nb_floor_divide; */
  &python_lv_divide, /* binaryfunc nb_true_divide; */
  0,                 /* binaryfunc nb_inplace_floor_divide; */
  0,                 /* binaryfunc nb_inplace_true_divide; */

  0, /* unaryfunc nb_index; */

  0, /* binaryfunc nb_matrix_multiply; */
  0, /* binaryfunc nb_inplace_matrix_multiply; */
};

PyTypeObject PyDataType = {
  PyVarObject_HEAD_INIT(NULL, 0) "PyData",                                /* tp_name */
  sizeof(PyDataObject),                                                   /* tp_basicsize */
  0,                                                                      /* tp_itemsize */
  PyDataObject_dealloc,                                                   /* tp_dealloc */
  0,                                                                      /* tp_print */
  0,                                                                      /* tp_getattr */
  0,                                                                      /* tp_setattr */
  0,                                                                      /* tp_reserved */
  0,                                                                      /* tp_repr */
  &PyDataNumbers,                                                         /* tp_as_number */
  0,                                                                      /* tp_as_sequence */
  0,                                                                      /* tp_as_mapping */
  0,                                                                      /* tp_hash  */
  PyDataObject_call,                                                      /* tp_call */
  python_data_str,                                                        /* tp_str */
  0,                                                                      /* tp_getattro */
  0,                                                                      /* tp_setattro */
  0,                                                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VERSION_TAG | Py_TPFLAGS_BASETYPE, /* tp_flags */
  0,                                                                      /* tp_doc */
  0,                                                                      /* tp_traverse */
  0,                                                                      /* tp_clear */
  0,                                                                      /* tp_richcompare */
  0,                                                                      /* tp_weaklistoffset */
  0,                                                                      /* tp_iter */
  0,                                                                      /* tp_iternext */
  0,                                                                      /* tp_methods */
  0,                                                                      /* tp_members */
  0,                                                                      /* tp_getset */
  0,                                                                      /* tp_base */
  0,                                                                      /* tp_dict */
  0,                                                                      /* tp_descr_get */
  0,                                                                      /* tp_descr_set */
  0,                                                                      /* tp_dictoffset */
  (initproc)PyDataInit,                                                   /* tp_init */
  0,                                                                      /* tp_alloc */
  PyDataObject_new,                                                       /* tp_new */
};

static PyModuleDef DataModule = {PyModuleDef_HEAD_INIT,
                                 "Data",
                                 "Example module that creates an extension type.",
                                 -1,
                                 PyDataFunctions,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL};

PyMODINIT_FUNC PyInit_PyData(void)
{
  PyObject *m;
  if (PyType_Ready(&PyDataType) < 0) return NULL;
  m = PyModule_Create(&DataModule);
  if (m == NULL) return NULL;

  Py_INCREF(&PyDataType);
  PyModule_AddObject(m, "openscad", (PyObject *)&PyDataType);
  return m;
}

PyObject *PyInit_data(void)
{
  return PyModule_Create(&DataModule);
}
