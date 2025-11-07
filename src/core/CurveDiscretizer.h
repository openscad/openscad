#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

class Location;
class ModuleInstantiation;
class Parameters;

class CurveDiscretizer
{
public:
  CurveDiscretizer(const Parameters& parameters);

  CurveDiscretizer(const Parameters& parameters, const Location& loc);

  CurveDiscretizer(double segmentsPerCircle);

  /**
   * @brief Initialize with a variable-lookup callback
   * Provide a function which returns the value for any named special.
   * Must work with "fn" (i.e. string will not have `$` in it).
   * Will not hold a reference to valueLookup after object constructed.
   * @param valueLookup Return the double value for a given named special when it's defined.
   */
  CurveDiscretizer(std::function<std::optional<double>(const char *)> valueLookup);

  /**
   * Calculate segments for a circle or circular arc.
   */
  std::optional<int> GetCircularSegmentCount(double r, double angle_degrees = 360.0) const;

  /**
   * @brief Calculate segments for a path.
   * Currently only uses $fn. Won't use $fs or $fa, but future variables could be used.
   */
  unsigned long GetPathSegmentCount(unsigned long minimum) const
  {
    unsigned long result = (fn > 3.0) ? static_cast<unsigned long>(fn) : 3;
    return std::max(result, minimum);
  }

  friend std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<CurveDiscretizer>& f);
  bool IsFnSpecifiedAndOdd() { return static_cast<int>(fn) & 1; }

private:
  CurveDiscretizer(double fn, double fs, double fa) : fn(fn), fs(fs), fa(fa) {}
  double fn, fs, fa;
};
std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<CurveDiscretizer>& f);
