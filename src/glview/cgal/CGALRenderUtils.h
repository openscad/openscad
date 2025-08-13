#pragma once

#include "geometry/linalg.h"
#include "core/Selection.h"

SelectedObject calculateLinePointDistance(const Vector3d& l1, const Vector3d& l2, const Vector3d& pt,
                                          double& dist_lat);
double calculateLineLineDistance(const Vector3d& l1b, const Vector3d& l1e, const Vector3d& l2b,
                                 const Vector3d& l2e, double& dist_lat);
