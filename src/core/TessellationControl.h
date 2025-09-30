#pragma once

#include <memory>
#include <optional>
#include <sstream>
#include <string>

class Parameters;
class ModuleInstantiation;
class TessellationControl
{
public:
  TessellationControl(const Parameters& parameters, const ModuleInstantiation *inst = nullptr);
  std::optional<int> circular_segments(double r, double angle_degrees = 360.0) const;
  friend std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);

private:
  double fn, fs, fa;
};
std::ostream& operator<<(std::ostream& stream, const TessellationControl& f);
