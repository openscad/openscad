#include "core/Selection.h"
#include "utils/vector_math.h"
#include "geometry/Grid.h"

#include <algorithm>
#include <cmath>
#include <limits>

// this function resolves a 3x3 linear eqauation system
/*
 * res[0] * v1 + res[1] *v2 + res[2] * vf3 = pt
 */

bool linsystem(Vector3d v1, Vector3d v2, Vector3d v3, Vector3d pt, Vector3d& res, double *detptr)
{
  double det, ad11, ad12, ad13, ad21, ad22, ad23, ad31, ad32, ad33;
  det = v1[0] * (v2[1] * v3[2] - v3[1] * v2[2]) - v1[1] * (v2[0] * v3[2] - v3[0] * v2[2]) +
        v1[2] * (v2[0] * v3[1] - v3[0] * v2[1]);
  if (detptr != nullptr) *detptr = det;
  ad11 = v2[1] * v3[2] - v3[1] * v2[2];
  ad12 = v3[0] * v2[2] - v2[0] * v3[2];
  ad13 = v2[0] * v3[1] - v3[0] * v2[1];
  ad21 = v3[1] * v1[2] - v1[1] * v3[2];
  ad22 = v1[0] * v3[2] - v3[0] * v1[2];
  ad23 = v3[0] * v1[1] - v1[0] * v3[1];
  ad31 = v1[1] * v2[2] - v2[1] * v1[2];
  ad32 = v2[0] * v1[2] - v1[0] * v2[2];
  ad33 = v1[0] * v2[1] - v2[0] * v1[1];

  if (fabs(det) < 0.00001) return true;

  res[0] = (ad11 * pt[0] + ad12 * pt[1] + ad13 * pt[2]) / det;
  res[1] = (ad21 * pt[0] + ad22 * pt[1] + ad23 * pt[2]) / det;
  res[2] = (ad31 * pt[0] + ad32 * pt[1] + ad33 * pt[2]) / det;
  return false;
}

SelectedObject calculateLinePointDistance(const Vector3d& l1, const Vector3d& l2, const Vector3d& pt,
                                          double& dist_lat)
{
  SelectedObject ruler;
  ruler.type = SelectionType::SELECTION_SEGMENT;
  Vector3d d = (l2 - l1);
  double l = d.norm();
  d.normalize();
  dist_lat = std::clamp((pt - l1).dot(d), 0.0, l);
  ruler.pt.push_back(l1 + d * dist_lat);
  ruler.pt.push_back(pt);
  return ruler;
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
    Vector3d c = l2b - l1b;
    Vector3d c_original = l1b - l2b;
    double v1_mag = sqrt(v1_squaredNorm);
    Vector3d cross_c_v1 = c.cross(v1);

    double dist_numerator = cross_c_v1.norm();
    double v1_norm = v1.norm();
    if (v1_norm < GRID_FINE) {
      // Line 1 is a point. This handles both line 2 is point and line 2 is a line.
      // Leave parametric_t as NaN because it's meaningless.
      double dummy;
      auto ret = calculateLinePointDistance(l2b, l2e, l1b, dummy);
      return (ret.pt[0] = ret.pt[1]);
    }
    // This handles line 2 being a point or line:
    return v1 * dist_numerator / (v1_norm * v1_norm);
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

SelectedObject calculateSegSegDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                       const Vector3d& l2e)
{
  SelectedObject ruler;
  ruler.type = SelectionType::SELECTION_SEGMENT;

  Vector3d v1 = l1e - l1b;
  Vector3d v2 = l2e - l2b;
  Vector3d n = v1.cross(v2);
  Vector3d res;
  if (n.norm() < 1e-6) {
    double dummy;
    return calculateLinePointDistance(l1b, l1e, l2b, dummy);
  }
  if (linsystem(v1, n, v2, l2e - l1b, res, nullptr)) {
    ruler.type = SelectionType::SELECTION_INVALID;
    return ruler;
  }
  double d1 = std::clamp(res[0], 0.0, 1.0);
  double d2 = std::clamp(res[2], 0.0, 1.0);
  ruler.pt.push_back(l1b + v1 * d1);
  ruler.pt.push_back(l2e - v2 * d2);

  return ruler;
}

SelectedObject calculatePointFaceDistance(const Vector3d& pt, const Vector3d& p1, const Vector3d& p2,
                                          const Vector3d& p3)
{
  SelectedObject ruler;
  ruler.type = SelectionType::SELECTION_SEGMENT;
  ruler.pt.push_back(pt);
  Vector3d n = (p2 - p1).cross(p3 - p1).normalized();
  double dist = fabs((pt - p1).dot(n));
  ruler.pt.push_back(pt + n * dist);
  return ruler;
}
