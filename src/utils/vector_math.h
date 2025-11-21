#pragma once

#include "geometry/linalg.h"

/**
 * @brief Calculates the shortest distance between a point and a line segment in 3D space.
 *
 * @param l1b The starting endpoint of the line segment.
 * @param l1e The ending endpoint of the line segment.
 * @param pt The point to which the distance is being calculated.
 * @param dist_lat The distance along the line segment, starting from l1b,
 *   that's closest to the point.
 *   So if the point is the origin, l1b is (1,1,1) and l1e is (2,2,2), it'll be 0.
 *   If you reversed l1b and l1e, it'll be sqrt(6).
 *   And if l1e was (-4,1,1), it's 1.0.
 *   This value is always clamped to [0,length(l1)], for better or worse.
 * @return double The shortest (Euclidean) distance from the point pt to the line segment [l1a, l1e].
 */
double calculateLinePointDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& pt,
                                  double& dist_lat);

/**
 * @brief Calculates the signed shortest distance between two infinite 3D lines.
 *
 * This function determines the minimum separation distance between the two lines
 * defined by the input segments, treating them as extending infinitely. It also
 * calculates a related lateral distance metric.
 *
 * The shortest distance vector is perpendicular to both lines. The sign of the
 * returned distance indicates the relative orientation or side of the lines.
 *
 * @param l1b The beginning point of the first line.
 * @param l1e The ending point of the first line.
 * @param l2b The beginning point of the second line.
 * @param l2e The ending point of the second line.
 * @param parametric_t **Ratio** of intersection point distance to L1b over L1's length. More
 * specifically: Consider P1 to be the point on the (infinite) line 1 which is closest to (infinite)
 * line 2. `parametric_t` is the magnitude of the vector from P1->L1b, divided by the magnitude of the
 * vector from L1e->L1b.
 *
 *   Most importantly, when `parametric_t` is 1.0, P1==L1e; when it's 0.0, P1==L1b. So when
 * 0<=parametric_t<=1.0, the closest point on (infinite) line 1 to (infinite) line 2 lies within the line
 * segment l1. When using the parametric line equation v = v1 * t.v2, parametric_t is t. If L1 is
 * actually a point or L1 and L2 are parallel, then it is NaN.
 * @return double The **signed shortest distance** between the two infinite lines.
 */
double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& parametric_t);

/**
 * @brief Calculates the unsigned shortest distance between two 3D line segments.
 *
 * This function determines the minimum separation distance between the two
 * finite line segments, ensuring the closest points lie on or between the
 * defined endpoints (l1b to l1e, and l2b to l2e).
 *
 * @param l1b The beginning point of the first line segment.
 * @param l1e The ending point of the first line segment.
 * @param l2b The beginning point of the second line segment.
 * @param l2e The ending point of the second line segment.
 * @return double The **unsigned magnitude** of the shortest distance vector
 * between the two segments. Returns NaN if the linear system cannot be solved.
 */
double calculateSegSegDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                               const Vector3d& l2e);
