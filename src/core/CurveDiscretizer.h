#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

class Location;
class ModuleInstantiation;
class Parameters;
struct Outline2d;
class RoofDiscretizer;

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
  int getPathSegmentCount() const { return std::max(static_cast<int>(fn), 3); }

  /**
   * Returns the number of slices for a linear_extrude with twist.
   */
  std::optional<int> getHelixSlices(double r_sqr, double h, double twist_degrees) const;

  std::optional<int> getConicalHelixSlices(double r_sqr, double height, double twist_degrees,
                                           double scale) const;

  /**
   * For linear_extrude with non-uniform scale and no twist.
   * Either use $fn directly as slices,
   * or divide the longest diagonal vertex extrude path by $fs.

   * dr_sqr - the largest 2D delta (before/after scaling) for all vertices, squared.
   * note: $fa is not considered since no twist
   * scale is not passed in since it was already used to calculate the largest delta.
  */
  std::optional<int> getDiagonalSlices(double delta_sqr, double height) const;

  /**
   * Possibly add more segments to an outline for extruding more precisely.
   * If segments is specified, then it is the only segmentation done; then if
   * $fn is non-zero, it is the only segmentation done. Finally, use $fa and/or $fs.
   *
   * @param slices The number of volumetric slices.
   * @param segments If non-zero, then add vertices until outline has this many vertices.
   */
  Outline2d splitOutline(const Outline2d& o, double twist, double scale_x, double scale_y,
                         unsigned int slices, unsigned int segments) const;

  friend std::ostream& operator<<(std::ostream& stream, const CurveDiscretizer& f);
  bool isFnSpecifiedAndOdd() const { return static_cast<int>(fn) & 1; }

private:
  CurveDiscretizer(double fn, double fs, double fa) : fn(fn), fs(fs), fa(fa) {}

protected:
  friend class RoofDiscretizer;
  double fn, fs, fa;
};
std::ostream& operator<<(std::ostream& stream, const CurveDiscretizer& f);

class RoofDiscretizer
{
public:
  RoofDiscretizer(const CurveDiscretizer& d, double scale);
  bool overMaxAngle(double radians) const;
  inline bool hasMaxSegmentSqrLength() const { return max_segment_sqr_length > 0.0; }
  bool overMaxSegmentSqrLength(double segment_sqr_length) const;

private:
  const double max_angle_deviation;
  const double max_segment_sqr_length;
};
