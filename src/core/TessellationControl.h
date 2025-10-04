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

  /*
   * Create a TessellationControl for a spot in dxfdim.cc that used a hardcoded value.
   * These values of 36,0,0 go back to the first commit in Github.
   * Unknown why it is that.
   */
  static TessellationControl DefaultForDxf() { return TessellationControl(36, 0, 0); }

  /*
   * Calculate segments for a circle or circular arc.
   */
  std::optional<int> circular_segments(double r, double angle_degrees = 360.0) const;

  /*
   * @brief Calculate segments for a path.
   * Currently only uses $fn. Won't use $fs or $fa, but future variables could be used.
   */
  unsigned long path_segments(unsigned long minimum) const
  {
    unsigned long result = (fn > 3.0) ? static_cast<unsigned long>(fn) : 3;
    return std::max(result, minimum);
  }

  friend std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);
  bool IsFnSpecifiedAndOdd() { return static_cast<int>(fn) & 1; }

private:
  TessellationControl(double fn, double fs, double fa) : fn(fn), fs(fs), fa(fa) {}
  double fn, fs, fa;
#ifdef ENABLE_PYTHON
  void setValuesFromPyMain();
#endif
};
std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);
