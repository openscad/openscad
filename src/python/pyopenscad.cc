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
#include <filesystem>

#include "pyopenscad.h"
#include "core/CsgOpNode.h"
#include "core/CurveDiscretizer.h"
#include "platform/PlatformUtils.h"

namespace fs = std::filesystem;

extern "C" PyObject *PyInit_openscad(void);

bool python_active;
bool python_trusted;

void PyObjectDeleter(PyObject *pObject) { Py_XDECREF(pObject); };

PyObjectUniquePtr pythonInitDict(nullptr, PyObjectDeleter);
PyObjectUniquePtr pythonMainModule(nullptr, PyObjectDeleter);
std::list<std::string> pythonInventory;
bool pythonDryRun = false;
std::shared_ptr<AbstractNode> python_result_node =
  nullptr; /* global result veriable containing the python created result */
PyObject *python_result_obj = nullptr;
bool pythonMainModuleInitialized = false;

void PyOpenSCADObject_dealloc(PyOpenSCADObject *self)
{
  Py_XDECREF(self->dict);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PyOpenSCADObject_alloc(PyTypeObject *cls, Py_ssize_t nitems)
{
  PyObject *self = PyType_GenericAlloc(cls, nitems);
  ((PyOpenSCADObject *)self)->dict = PyDict_New();
  PyObject *origin = PyList_New(4);
  for (int i = 0; i < 4; i++) {
    PyObject *row = PyList_New(4);
    for (int j = 0; j < 4; j++) PyList_SetItem(row, j, PyFloat_FromDouble(i == j ? 1.0 : 0.0));
    PyList_SetItem(origin, i, row);
  }
  PyDict_SetItemString(((PyOpenSCADObject *)self)->dict, "origin", origin);
  Py_XDECREF(origin);
  return self;
}

/*
 *  allocates a new PyOpenSCAD Object including its internal dictionary
 */

static PyObject *PyOpenSCADObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  return PyOpenSCADObject_alloc(&PyOpenSCADType, 0);
}

/*
 *  allocates a new PyOpenSCAD to store an existing OpenSCAD Abstract Node
 */

PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, const std::shared_ptr<AbstractNode>& node)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)type->tp_alloc(type, 0);
  if (self != nullptr) {
    Py_XINCREF(self);
    self->node = node;
    return (PyObject *)self;
  }
  return nullptr;
}

PyThreadState *tstate = nullptr;

void python_lock(void)
{
  if (tstate != nullptr && pythonInitDict != nullptr) PyEval_RestoreThread(tstate);
}

void python_unlock(void)
{
  if (pythonInitDict != nullptr) tstate = PyEval_SaveThread();
}
/*
 *  extracts Absrtract Node from PyOpenSCAD Object
 */

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj, PyObject **dict)
{
  std::shared_ptr<AbstractNode> result = ((PyOpenSCADObject *)obj)->node;
  if (result.use_count() > 2) {
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
 * same as  python_more_obj but always returns only one AbstractNode by creating an UNION operation
 */

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs, PyObject **dict)
{
  std::shared_ptr<AbstractNode> result;
  if (Py_TYPE(objs) == &PyOpenSCADType) {
    result = ((PyOpenSCADObject *)objs)->node;
    if (result.use_count() > 2) {
      result = result->clone();
    }
    *dict = ((PyOpenSCADObject *)objs)->dict;
  } else if (PyList_Check(objs)) {
    DECLARE_INSTANCE();
    auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    int n = PyList_Size(objs);
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      if (Py_TYPE(obj) == &PyOpenSCADType) {
        std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, dict);
        node->children.push_back(child);
      } else return nullptr;
    }
    result = node;
  } else result = nullptr;
  return result;
}

int python_numberval(PyObject *number, double *result)
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
  return 1;
}

/*
 * Tries to extract an 3D vector out of a python list
 */

int python_vectorval(PyObject *vec, int minval, int maxval, double *x, double *y, double *z, double *w)
{
  if (w != NULL) *w = 0;
  if (PyList_Check(vec)) {
    if (PyList_Size(vec) < minval || PyList_Size(vec) > maxval) return 1;

    if (PyList_Size(vec) >= 1) {
      if (python_numberval(PyList_GetItem(vec, 0), x)) return 1;
    }
    if (PyList_Size(vec) >= 2) {
      if (python_numberval(PyList_GetItem(vec, 1), y)) return 1;
    }
    if (PyList_Size(vec) >= 3) {
      if (python_numberval(PyList_GetItem(vec, 2), z)) return 1;
    }
    if (PyList_Size(vec) >= 4 && w != NULL) {
      if (python_numberval(PyList_GetItem(vec, 3), w)) return 1;
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

std::vector<Vector3d> python_vectors(PyObject *vec, int mindim, int maxdim)
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
          if (python_numberval(PyList_GetItem(vec, i), &result[i])) return results;  // Error
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

/**
 * Create a CurveDiscretizer by extracting parameters from __main__ and kwargs
 */

CurveDiscretizer CreateCurveDiscretizer(PyObject *kwargs)
{
  PyObject *mainModule = pythonMainModule.get();
  return CurveDiscretizer([kwargs, mainModule](const char *key) -> std::optional<double> {
    double result;
    if (kwargs != nullptr && PyDict_Check(kwargs)) {  // kwargs can be nullptr
      PyObject *value = PyDict_GetItemString(kwargs, key);
      if (!(python_numberval(value, &result))) return result;  // value an be Integer, Number, ...
    }
    if (mainModule != nullptr) {
      if (PyObject_HasAttrString(mainModule, key)) {
        PyObjectUniquePtr var(PyObject_GetAttrString(mainModule, key), PyObjectDeleter);
        if (var.get() != nullptr) {
          if (!(python_numberval(var.get(), &result))) return result;
        }
      }
    }
    return {};
  });
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

void initPython(const std::string& binDir, double time)
{
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
          PyObjectUniquePtr key1_(PyUnicode_AsEncodedString(key1, "utf-8", "~"), PyObjectDeleter);
          if (key1_ == nullptr) continue;
          const char *key1_str = PyBytes_AS_STRING(key1_.get());
          if (strcmp(key1_str, "modules") == 0) {
            PyObject *key2, *value2;
            Py_ssize_t pos2 = 0;
            while (PyDict_Next(value1, &pos2, &key2, &value2)) {
              PyObjectUniquePtr key2_(PyUnicode_AsEncodedString(key2, "utf-8", "~"), PyObjectDeleter);
              if (key2_ == nullptr) continue;
              const char *key2_str = PyBytes_AS_STRING(key2_.get());
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

    PyImport_AppendInittab("openscad", &PyInit_openscad);
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    std::string sep = "";
    std::ostringstream stream;
#ifdef _WIN32
    char sepchar = ':';
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
    stream << sep << PlatformUtils::userLibraryPath();
    stream << sepchar << ".";

    if (!binDir.empty()) {
      PyConfig_SetBytesString(&config, &config.executable, (binDir + "/python").c_str());
    }

    PyStatus status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
      LOG(message_group::Error, "Python not found. Is it installed ?");
      return;
    }
    PyConfig_Clear(&config);

    pythonMainModule.reset(PyImport_AddModule("__main__"));
    pythonMainModuleInitialized = pythonMainModule != nullptr;
    pythonInitDict.reset(PyModule_GetDict(pythonMainModule.get()));
    PyInit_PyOpenSCAD();
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
  stream << "t=" << time;
  PyRun_String(stream.str().c_str(), Py_file_input, pythonInitDict.get(), pythonInitDict.get());
}

void finishPython(void) {}

std::string evaluatePython(const std::string& code, bool dry_run)
{
  std::string error;
  python_result_node = nullptr;
  PyObjectUniquePtr pyExcValue(nullptr, PyObjectDeleter);
  PyObjectUniquePtr pyExcTraceback(nullptr, PyObjectDeleter);
  /* special python code to catch errors from stdout and stderr and make them available in OpenSCAD
   * console */
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
";

  PyRun_SimpleString(python_init_code);
  PyObjectUniquePtr result(nullptr, PyObjectDeleter);
  result.reset(PyRun_String(code.c_str(), Py_file_input, pythonInitDict.get(),
                            pythonInitDict.get())); /* actual code is run here */

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

PyTypeObject PyOpenSCADType = {
  PyVarObject_HEAD_INIT(nullptr, 0) "PyOpenSCAD", /* tp_name */
  sizeof(PyOpenSCADObject),                       /* tp_basicsize */
  0,                                              /* tp_itemsize */
  (destructor)PyOpenSCADObject_dealloc,           /* tp_dealloc */
  0,                                              /* vectorcall_offset */
  0,                                              /* tp_getattr */
  0,                                              /* tp_setattr */
  0,                                              /* tp_as_async */
  python_str,                                     /* tp_repr */
  &PyOpenSCADNumbers,                             /* tp_as_number */
  0,                                              /* tp_as_sequence */
  &PyOpenSCADMapping,                             /* tp_as_mapping */
  0,                                              /* tp_hash  */
  0,                                              /* tp_call */
  python_str,                                     /* tp_str */
  python__getattro__,                             /* tp_getattro */
  python__setattro__,                             /* tp_setattro */
  0,                                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
  "PyOpenSCAD Object",                            /* tp_doc */
  0,                                              /* tp_traverse */
  0,                                              /* tp_clear */
  0,                                              /* tp_richcompare */
  0,                                              /* tp_weaklistoffset */
  0,                                              /* tp_iter */
  0,                                              /* tp_iternext */
  PyOpenSCADMethods,                              /* tp_methods */
  0,                                              /* tp_members */
  0,                                              /* tp_getset */
  0,                                              /* tp_base */
  0,                                              /* tp_dict */
  0,                                              /* tp_descr_get */
  0,                                              /* tp_descr_set */
  0,                                              /* tp_dictoffset */
  (initproc)PyOpenSCADInit,                       /* tp_init */
  PyOpenSCADObject_alloc,                         /* tp_alloc */
  PyOpenSCADObject_new,                           /* tp_new */
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
  PyModule_AddObject(m, "openscad", (PyObject *)&PyOpenSCADType);
  return m;
}
