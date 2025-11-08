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
  std::optional<int> getCircularSegmentCount(double r, double angle_degrees = 360.0) const;

  /**
   * @brief Calculate segments for a path.
   * Currently only uses $fn.
   */
  int getPathSegmentCount() const
  {
    // Prior to https://github.com/openscad/openscad/commit/f5816258db263408a7aa2feec1fafffe77644662
    // fn was set to a fixed value of 20 where this is now used.
    // The author decided it should never be smaller than this original value.
    return std::max(static_cast<int>(fn), 20);
  }

  friend std::ostream& operator<<(std::ostream& stream, const CurveDiscretizer& f);
  bool isFnSpecifiedAndOdd() { return static_cast<int>(fn) & 1; }

private:
  CurveDiscretizer(double fn, double fs, double fa) : fn(fn), fs(fs), fa(fa) {}
  double fn, fs, fa;
};
std::ostream& operator<<(std::ostream& stream, const CurveDiscretizer& f);
