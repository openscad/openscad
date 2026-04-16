#include "utils/vector_math.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "geometry/Grid.h"

double calculateLinePointDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& pt,
                                  double& dist_lat)
{
  Vector3d d = (l1e - l1b);
  double l = d.norm();
  d.normalize();
  dist_lat = std::clamp((pt - l1b).dot(d), 0.0, l);
  return (l1b + d * dist_lat - pt).norm();
}

Vector3d calculateLineLineVector(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& parametric_t, double& signed_distance)
{
  parametric_t = std::numeric_limits<double>::quiet_NaN();
  Vector3d v1 = l1e - l1b;
  Vector3d v2 = l2e - l2b;
  Vector3d n = v1.cross(v2);
  double t = n.norm();

  double v1_squaredNorm = v1.squaredNorm();
  double v2_squaredNorm = v2.squaredNorm();
  if (v1_squaredNorm < GRID_FINE * GRID_FINE || v2_squaredNorm < GRID_FINE * GRID_FINE) {
    // An input is indistinguishable from a point, so we can't usefully calculate a result.
    signed_distance = std::numeric_limits<double>::quiet_NaN();
    return Vector3d(signed_distance, signed_distance, signed_distance);
  }

  if (t < GRID_FINE) {
    // Lines are parallel (or collinear). `parametric_t` makes no sense.
    Vector3d c_original = l1b - l2b;
    double v1_mag = sqrt(v1_squaredNorm);

    // Original distance logic for parallel: |(l2b-l1b) x v1| / |v1|
    Vector3d w = l2b - l1b;
    Vector3d cross_w_v1 = w.cross(v1);

    signed_distance = cross_w_v1.norm() / v1_mag;
    // For parallel, the vector is the perpendicular projection of l1b onto Line 2
    double t2 = -w.dot(v2) / v2_squaredNorm;
    return (l2b + t2 * v2) - l1b;
  }

  n /= t;  // Normalize n.

  signed_distance = n.dot(l1b - l2b);

  // parametric_t logic remains the same
  parametric_t = (v2.cross(n)).dot(l2b - l1b) / t;

  // The vector pointing from Line 1 to Line 2 is actually -(signed_distance * n)
  // because signed_distance was calculated using (l1b - l2b).
  return -signed_distance * n;
}

double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& parametric_t)
{
  double dist;
  calculateLineLineVector(l1b, l1e, l2b, l2e, parametric_t, dist);
  return dist;
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
