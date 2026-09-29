#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "geometry/PolySet.h"
#include "geometry/linalg.h"

// Builds a patched surface between an outer boundary ring and zero or more
// hole rings, all given as 3D point lists. 'proj' maps a 3D point to its
// (u,v) parameterization used for triangulation; 'displacement' optionally
// offsets interior (non-boundary) points along the local surface normal.
//
// 'proj' is OPTIONAL: pass a default-constructed (empty) std::function to
// let patch() pick one automatically. It distinguishes two common cases:
//   - the hole(s) sit well away from the outer ring's own centroid relative
//     to the outer ring's size (e.g. two ports/flanges connected by a
//     tube) -> project onto the plane perpendicular to the axis connecting
//     the centroids, so outer and hole overlap concentrically in (u,v);
//   - outer and holes are roughly coplanar (e.g. a flat panel with
//     cutouts) -> fit the best plane through all boundary points (PCA) and
//     project onto it.
// An explicitly supplied 'proj' always overrides this and is used as-is.
//
// 'outer_normal' / 'holes_normal' are OPTIONAL, parallel arrays giving a
// tangent direction at each boundary point (one entry per point in 'outer'
// / each entry of 'holes', respectively). When given (non-empty), the patch
// between rings is built as a cubic (Hermite-style) surface that leaves each
// ring tangent to the given direction, instead of the default flat/linear
// barycentric blend. Leave both empty (the default) to keep the original,
// purely linear behavior.
std::unique_ptr<PolySet> patch(const std::vector<Vector3d>& outer,
                               const std::vector<std::vector<Vector3d>>& holes,
                               const std::function<Vector2d(const Vector3d&)>& proj,
                               double grid_spacing_uv,
                               const std::function<double(const Vector3d&)>& displacement,
                               const std::vector<Vector3d>& outer_normal = {},
                               const std::vector<std::vector<Vector3d>>& holes_normal = {});
