#pragma once

#include <memory>
#include "geometry/Polygon2d.h"
#include "geometry/Geometry.h"
#include "core/SkinNode.h"

/*!
  input: List of 2D objects arranged in 3D, each with identical outline count and vertex count
  output: 3D PolySet
 */
std::shared_ptr<const Geometry> skinPolygonSequence(const SkinNode& node,
                                                    std::vector<std::shared_ptr<const Polygon2d>> slices,
                                                    const Location& loc, std::string const& docpath);
