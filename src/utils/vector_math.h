#pragma once

#include "geometry/linalg.h"

/**
 * @brief Calculates the shortest distance between a point and a line segment in 3D space.
 *
 * @param l1b The starting endpoint of the line segment.
 * @param l1e The ending endpoint of the line segment.
 * @param pt The point to which the distance is being calculated.
 * @param dist_lat A reference parameter that will be set to the distance along the
 * line segment from l1b to a closest point on the segment to the point. This is not the same as
 * calculateLineLineDistance's `dist_lat`: this is not a ratio, but an actual distance.
 * @return double The shortest (Euclidean) distance from the point pt to the line segment [l1, l2].
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
 * @param dist_lat Ratio of intersection point distance to L1b over L1's length. More specifically:
 * consider P1 to be the point on the (infinite) line 1 which is closest to (infinite) line 2. `dist_lat`
 * is the magnitude of the vector from P1->L1b divided by the vector from L1e->L1b. Most importantly,
 * when `dist_lat` is 1.0, P1==L1e; when it's 0.0, P1==L1b. So when 0<=dist_lat<=1.0, the closest point
 * on (infinite) line 1 to (infinite) line 2 lies within the line segment l1. This may be considered "an
 * algebraically rearranged version of the parametric coordinate s".
 * @return double The **signed shortest distance** between the two infinite lines.
 * If the lines are parallel (or collinear), the function returns the
 * distance between Line 1 and the point l2b, as calculated by
 * `calculateLinePointDistance`. TODO: coryrc - bad
 */
double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& dist_lat);

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
