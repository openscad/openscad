#include "core/TessellationControl.h"

#ifdef ENABLE_PYTHON
#include "python/pyopenscad.h"
#endif
#include <algorithm>

#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "geometry/Grid.h"
#include "utils/printutils.h"

#define F_MINIMUM 0.01

TessellationControl::TessellationControl(const Parameters& parameters, const ModuleInstantiation *inst)
{
  fn = parameters["$fn"].toDouble();
  fs = parameters["$fs"].toDouble();
  fa = parameters["$fa"].toDouble();

  if (fn < 0.0) {
    if (inst) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "$fn negative - setting to 0");
    }
    fn = 0.0;
  }
  if (fs < F_MINIMUM) {
    if (inst) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "$fs too small - clamping to %1$f", F_MINIMUM);
    }
    fs = F_MINIMUM;
  }
  if (fa < F_MINIMUM) {
    if (inst) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "$fa too small - clamping to %1$f", F_MINIMUM);
    }
    fa = F_MINIMUM;
  }
}

#ifdef ENABLE_PYTHON
/**
 * Attempts to set a variable from a Python dictionary, handling
 * double* (direct assignment) and int* (rounding) targets.
 * On any error, target unchanged.
 * @param kwargs The Python dict (PyObject*) to look up the value in.
 * @param string_input The key (const char*) to search for.
 * @param var_ptr A pointer to the variable to set on success.
 */
void trySetVariable(PyObject *kwargs, const char *string_input, double *var_ptr)
{
  if (!kwargs || !string_input || !var_ptr) {
    return;
  }

  PyObject *value = nullptr;

  // PyDict_GetItemStringRef returns 1 (found), 0 (missing), or -1 (error).
  int res = PyDict_GetItemStringRef(kwargs, string_input, &value);

  if (res == 1) {
    double result = PyFloat_AsDouble(value);
    Py_DECREF(value);

    if (PyErr_Occurred()) {
      PyErr_Clear();
    } else {
      *var_ptr = result;
    }
  } else if (res == -1) {
    PyErr_Clear();
  }
}

TessellationControl::TessellationControl(PyObject *kwargs)
{
  setValuesFromPyMain();
  if (kwargs == nullptr || !PyDict_Check(kwargs)) {
    return;
  }

  trySetVariable(kwargs, "fn", &this->fn);
  trySetVariable(kwargs, "fs", &this->fs);
  trySetVariable(kwargs, "fa", &this->fa);
}

void TessellationControl::setValuesFromPyMain()
{
  PyObject *mainModule = PyImport_AddModule("__main__");
  if (mainModule == nullptr) {
    fn = fa = fs = 0;
    return;
  }
  fn = 0;
  fa = 12;
  fs = 2;

  if (PyObject_HasAttrString(mainModule, "fn")) {
    PyObjectUniquePtr varFn(PyObject_GetAttrString(mainModule, "fn"), PyObjectDeleter);
    if (varFn.get() != nullptr) {
      fn = PyFloat_AsDouble(varFn.get());
      if (isnan(fn)) {
        fn = 0;
      }
    }
  }

  if (PyObject_HasAttrString(mainModule, "fa")) {
    PyObjectUniquePtr varFa(PyObject_GetAttrString(mainModule, "fa"), PyObjectDeleter);
    if (varFa.get() != nullptr) {
      fa = PyFloat_AsDouble(varFa.get());
      if (isnan(fa)) {
        fa = 0;
      }
    }
  }

  if (PyObject_HasAttrString(mainModule, "fs")) {
    PyObjectUniquePtr varFs(PyObject_GetAttrString(mainModule, "fs"), PyObjectDeleter);
    if (varFs.get() != nullptr) {
      fs = PyFloat_AsDouble(varFs.get());
      if (isnan(fs)) {
        fs = 0;
      }
    }
  }
}
#endif

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
std::optional<int> TessellationControl::circular_segments(double r, double angle_degrees) const
{
  // FIXME: It would be better to refuse to create an object. Let's do more strict error handling
  // in future versions of OpenSCAD
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return {};
  if (fn > 0.0)
    return std::max(static_cast<int>(std::ceil((fn >= 3 ? fn : 3) * std::fabs(angle_degrees) / 360.0)),
                    1);
  return std::max(static_cast<int>(std::ceil(std::fabs(angle_degrees) / 360.0 *
                                             std::max(std::min(360.0 / fa, r * 2 * M_PI / fs), 5.0))),
                  1);
}

std::ostream& operator<<(std::ostream& stream, const TessellationControl& f)
{
  stream << "$fn = " << f.fn << ", $fa = " << f.fa << ", $fs = " << f.fs;
  return stream;
}
