#include "core/TessellationControl.h"

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

/*!
   Returns the number of subdivision of a whole circle, given radius and
   the three special variables $fn, $fs and $fa
 */
std::optional<int> TessellationControl::circular_segments(double r) const
{
  // FIXME: It would be better to refuse to create an object. Let's do more strict error handling
  // in future versions of OpenSCAD
  if (r < GRID_FINE || std::isinf(fn) || std::isnan(fn)) return {};
  if (fn > 0.0) return static_cast<int>(fn >= 3 ? fn : 3);
  return static_cast<int>(ceil(fmax(fmin(360.0 / fa, r * 2 * M_PI / fs), 5)));
}

std::ostream& operator<<(std::ostream& stream, const TessellationControl& f)
{
  stream << "$fn = " << f.fn << ", $fa = " << f.fa << ", $fs = " << f.fs;
  return stream;
}
