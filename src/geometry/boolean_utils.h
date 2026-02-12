#pragma once

#include <memory>

#include "geometry/Geometry.h"
#include "geometry/PolySet.h"

std::unique_ptr<PolySet> applyHull(const Geometry::Geometries& children);
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
