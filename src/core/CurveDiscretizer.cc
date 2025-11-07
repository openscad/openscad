#include "core/CurveDiscretizer.h"

#include <algorithm>

#include "core/AST.h"  // for Location
#include "core/Parameters.h"
#include "geometry/Grid.h"
#include "utils/printutils.h"

#define F_MINIMUM 0.01

CurveDiscretizer::CurveDiscretizer(const Parameters& parameters, const Location& loc)
{
  fn = parameters["$fn"].toDouble();
  fs = parameters["$fs"].toDouble();
  fa = parameters["$fa"].toDouble();

  if (fn < 0.0) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fn negative - setting to 0");
    fn = 0.0;
  }
  if (fs < F_MINIMUM) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fs too small - clamping to %1$f",
        F_MINIMUM);
    fs = F_MINIMUM;
  }
  if (fa < F_MINIMUM) {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "$fa too small - clamping to %1$f",
        F_MINIMUM);
    fa = F_MINIMUM;
  }
}

CurveDiscretizer::CurveDiscretizer(const Parameters& parameters)
{
  fn = std::max(parameters["$fn"].toDouble(), 0.0);
  fs = std::max(parameters["$fs"].toDouble(), F_MINIMUM);
  fa = std::max(parameters["$fa"].toDouble(), F_MINIMUM);
}

CurveDiscretizer::CurveDiscretizer(double segmentsPerCircle)
{
  fn = segmentsPerCircle;
  fs = 0;
  fa = 0;

  if (fn < 5.0) {
    fn = 5.0;
  }
}

CurveDiscretizer::CurveDiscretizer(std::function<std::optional<double>(const char *)> valueLookup)
{
  // These defaults were what the Python code was using.
  // Don't know why it differs from OpenSCAD language.
  fn = std::max(valueLookup("fn").value_or(0.0), 0.0);
  fa = std::max(valueLookup("fa").value_or(12.0), F_MINIMUM);
  fs = std::max(valueLookup("fs").value_or(2.0), F_MINIMUM);
}

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
std::optional<int> CurveDiscretizer::GetCircularSegmentCount(double r, double angle_degrees) const
{
  // FIXME: It would be better to refuse to create an object. Let's do more strict error handling
  // in future versions of OpenSCAD
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return {};

  // We continue to separately call `ceil()` before angle calculations to preserve backward compatibility
  double result;
  if (fn > 0.0) {
    result = std::ceil(fn >= 3 ? fn : 3) * std::fabs(angle_degrees) / 360.0;
  } else {
    result = std::ceil(std::max(std::min(360.0 / fa, r * 2 * M_PI / fs), 5.0)) *
             std::fabs(angle_degrees) / 360.0;
  }
  return std::max(1, static_cast<int>(std::ceil(result)));
}

std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<CurveDiscretizer>& f)
{
  if (!f) {
    return stream << "[null CurveDiscretizer]";
  }
  stream << "$fn = " << f->fn << ", $fa = " << f->fa << ", $fs = " << f->fs;
  return stream;
}
