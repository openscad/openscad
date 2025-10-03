#pragma once

#include <memory>
#include <optional>
#include <sstream>
#include <string>

#ifdef ENABLE_PYTHON
#include <Python.h>
#endif
class Parameters;
class ModuleInstantiation;

class TessellationControl
{
public:
  TessellationControl(const Parameters& parameters, const ModuleInstantiation *inst = nullptr);
#ifdef ENABLE_PYTHON
  /*
   * Extract discretization values from environment if possible,
   * then override any which are explicitly set as keyword arguments.
   */
  TessellationControl(PyObject *kwargs);
#endif

  std::optional<int> circular_segments(double r, double angle_degrees = 360.0) const;
  friend std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);
  bool IsFnSpecifiedAndOdd() { return static_cast<int>(fn) & 1; }

private:
  double fn, fs, fa;
#ifdef ENABLE_PYTHON
  void setValuesFromPyMain();
#endif
};
std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);
