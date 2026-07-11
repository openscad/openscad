#include <Python.h>

#ifdef _WIN32

#include "python_runtime.h"

#include <cstring>

void python_configure_windows_sys_compat()
{
  PyObject *sys = PyImport_ImportModule("sys");
  if (sys == nullptr) {
    PyErr_Clear();
    return;
  }

  PyObject *sysdict = PyModule_GetDict(sys);
  PyObject *isMingw = sysdict == nullptr ? nullptr : PyDict_GetItemString(sysdict, "_is_mingw");
  if (isMingw == nullptr && PyErr_Occurred()) {
    PyErr_Clear();
  } else if (sysdict != nullptr && isMingw == nullptr) {
    const char *compiler = Py_GetCompiler();
    const bool runtimeIsMingw = compiler != nullptr && (strstr(compiler, "MINGW") != nullptr ||
                                                        strstr(compiler, "GCC") != nullptr);
    PyObject *value = runtimeIsMingw ? Py_True : Py_False;
    if (PyDict_SetItemString(sysdict, "_is_mingw", value) != 0) {
      PyErr_Clear();
    }
  }
  PyObject *abiflags = sysdict == nullptr ? nullptr : PyDict_GetItemString(sysdict, "abiflags");
  if (abiflags == nullptr && PyErr_Occurred()) {
    PyErr_Clear();
  } else if (sysdict != nullptr && abiflags == nullptr) {
    PyObject *value = PyUnicode_FromString("");
    if (value == nullptr) {
      PyErr_Clear();
    } else if (PyDict_SetItemString(sysdict, "abiflags", value) != 0) {
      PyErr_Clear();
    }
    Py_XDECREF(value);
  }
  Py_DECREF(sys);
}

#endif
