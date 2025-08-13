#include "geometry/linalg.h"
#include "glview/cgal/CGALRenderUtils.h"
#include "core/Selection.h"

#include <algorithm>

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

double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& dist_lat)
{
  Vector3d v1 = l1e - l1b;
  Vector3d v2 = l2e - l2b;
  Vector3d n = v1.cross(v2);
  if (n.norm() == 0) {
    double dummy;
    SelectedObject rul = calculateLinePointDistance(l1b, l1e, l2b, dummy);
    return (rul.pt[0] - rul.pt[1]).norm();
  }
  double t = n.norm();
  n.normalize();
  double d = n.dot(l1b - l2b);
  dist_lat = (v2.cross(n)).dot(l2b - l1b) / t;
  return d;
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
