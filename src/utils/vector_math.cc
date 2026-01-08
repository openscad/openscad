#include "utils/vector_math.h"
#include "geometry/Grid.h"

#include <algorithm>

double calculateLinePointDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& pt,
                                  double& dist_lat)
{
  Vector3d d = (l1e - l1b);
  double l = d.norm();
  d.normalize();
  dist_lat = std::clamp((pt - l1b).dot(d), 0.0, l);
  return (l1b + d * dist_lat - pt).norm();
}

double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& parametric_t)
{
  double d;
  Vector3d v1 = l1e - l1b;
  Vector3d v2 = l2e - l2b;
  Vector3d n = v1.cross(v2);
  double t = n.norm();

  if (t < GRID_FINE) {
    // Lines are parallel (or collinear). `parametric_t` makes no sense.
    parametric_t = std::numeric_limits<double>::quiet_NaN();
    Vector3d c = l2b - l1b;
    Vector3d cross_c_v1 = c.cross(v1);

    double dist_numerator = cross_c_v1.norm();
    double v1_norm = v1.norm();
    if (v1_norm < GRID_FINE) {
      // Line 1 is a point. This handles both line 2 is point and line 2 is a line.
      // Leave parametric_t as NaN because it's meaningless.
      double dummy;
      auto ret = calculateLinePointDistance(l2b, l2e, l1b, dummy);
      return ret;
    }
    // This handles line 2 being a point or line:
    return dist_numerator / v1_norm;
  }
  n.normalize();
  d = n.dot(l1b - l2b);
  parametric_t = (v2.cross(n)).dot(l2b - l1b) / t;
  return d;
}

double calculateSegSegDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                               const Vector3d& l2e)
{
  Vector3d u = l1e - l1b;
  Vector3d v = l2e - l2b;
  Vector3d w = l1b - l2b;

  double a = u.dot(u);  // always >= 0
  double b = u.dot(v);
  double c = v.dot(v);  // always >= 0
  double d = u.dot(w);
  double e = v.dot(w);
  double D = a * c - b * b;  // determinant
  double sc, sN, sD = D;     // sc = sN / sD, default sD = D
  double tc, tN, tD = D;     // tc = tN / tD, default tD = D

  const double SMALL_NUM = 1e-9;

  // Check if segments are parallel
  if (D < SMALL_NUM) {
    sN = 0.0;  // force traversal from l1b
    sD = 1.0;  // prevent division by zero
    tN = e;
    tD = c;
  } else {
    // Get the closest points on the infinite lines
    sN = (b * e - c * d);
    tN = (a * e - b * d);

    if (sN < 0.0) {  // sc < 0 => the s=0 edge is closest
      sN = 0.0;
      tN = e;
      tD = c;
    } else if (sN > sD) {  // sc > 1 => the s=1 edge is closest
      sN = sD;
      tN = e + b;
      tD = c;
    }
  }

  // Now check if tc is within [0, 1]
  if (tN < 0.0) {  // tc < 0 => the t=0 edge is closest
    tN = 0.0;
    // re-evaluate sc for this t=0 edge
    if (-d < 0.0) sN = 0.0;
    else if (-d > a) sN = sD;
    else {
      sN = -d;
      sD = a;
    }
  } else if (tN > tD) {  // tc > 1 => the t=1 edge is closest
    tN = tD;
    // re-evaluate sc for this t=1 edge
    if ((-d + b) < 0.0) sN = 0;
    else if ((-d + b) > a) sN = sD;
    else {
      sN = (-d + b);
      sD = a;
    }
  }

  // Final clamping and distance calculation
  sc = (std::fabs(sN) < SMALL_NUM ? 0.0 : sN / sD);
  tc = (std::fabs(tN) < SMALL_NUM ? 0.0 : tN / tD);

  // Closest vector between segments
  Vector3d dP = w + (sc * u) - (tc * v);
  return dP.norm();
}
