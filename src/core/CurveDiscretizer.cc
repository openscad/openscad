#include "core/CurveDiscretizer.h"

#ifdef ENABLE_PYTHON
#include "python/pyopenscad.h"
#endif
#include <algorithm>

#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "geometry/Grid.h"
#include "utils/printutils.h"

#define F_MINIMUM 0.01

CurveDiscretizer::CurveDiscretizer(const Parameters& parameters, const ModuleInstantiation *inst)
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
 * Attempts to set a variable from a Python dictionary.
 * On any error, target unchanged.
 * @param kwargs The Python dict to look up the value in.
 * @param key The key to search for.
 * @param var_ptr A pointer to the variable to set on success.
 * @param minimum Only set if the new value is >= this number
 */
void trySetVariable(PyObject *kwargs, const char *key, double *var_ptr, double minimum = F_MINIMUM)
{
  if (!PyDict_Check(kwargs) || !key || !var_ptr) {
    return;
  }

  PyObject *value = PyDict_GetItemString(kwargs, key);

  if (PyFloat_Check(value)) {
    double result = PyFloat_AsDouble(value);
    if (result == -1.0 && PyErr_Occurred()) {
      // Pass on an exception:
      return;
    }
    if (result >= minimum) {
      *var_ptr = result;
    }
  }
  // else pass on exception (if any), but usually just not present.
}

CurveDiscretizer::CurveDiscretizer(PyObject *kwargs)
{
  setValuesFromPyMain();
  if (kwargs == nullptr || !PyDict_Check(kwargs)) {
    return;
  }

  trySetVariable(kwargs, "fn", &this->fn, 0);
  trySetVariable(kwargs, "fs", &this->fs);
  trySetVariable(kwargs, "fa", &this->fa);
}

void CurveDiscretizer::setValuesFromPyMain()
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
      if (isnan(fn) || fn < 0) {
        fn = 0;
      }
    }
  }

  if (PyObject_HasAttrString(mainModule, "fa")) {
    PyObjectUniquePtr varFa(PyObject_GetAttrString(mainModule, "fa"), PyObjectDeleter);
    if (varFa.get() != nullptr) {
      fa = PyFloat_AsDouble(varFa.get());
      if (isnan(fa) || fa < F_MINIMUM) {
        fa = F_MINIMUM;
      }
    }
  }

  if (PyObject_HasAttrString(mainModule, "fs")) {
    PyObjectUniquePtr varFs(PyObject_GetAttrString(mainModule, "fs"), PyObjectDeleter);
    if (varFs.get() != nullptr) {
      fs = PyFloat_AsDouble(varFs.get());
      if (isnan(fs) || fs < F_MINIMUM) {
        fs = F_MINIMUM;
      }
    }
  }
}
#endif

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
std::optional<int> CurveDiscretizer::GetCircularSegmentCount(double r, double angle_degrees) const
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

std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<CurveDiscretizer>& f)
{
  if (!f) {
    return stream << "[null CurveDiscretizer]";
  }
  stream << "$fn = " << f->fn << ", $fa = " << f->fa << ", $fs = " << f->fs;
  return stream;
}
