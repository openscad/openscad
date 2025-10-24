#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

class Parameters;
class ModuleInstantiation;

class CurveDiscretizer
{
public:
  CurveDiscretizer(const Parameters& parameters, const ModuleInstantiation *inst = nullptr);

  /**
   * @brief Provide a function which returns the value for any named special.
   * Should work on "fn" and not "$fn".
   * Will not hold a reference to valueLookup after object constructed.
   */
  CurveDiscretizer(std::function<std::optional<double>(const char *)> valueLookup);

  /**
   * Create a CurveDiscretizer for a spot in dxfdim.cc that used a hardcoded value.
   * These values of 36,0,0 go back to the first commit in Github.
   * Unknown why it is that.
   */
  static std::shared_ptr<CurveDiscretizer> DefaultForDxf()
  {
    return std::make_shared<CurveDiscretizer>(std::move(CurveDiscretizer(36, 0, 0)));
  }

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
