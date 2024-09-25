#pragma once

#include <memory>
#include "core/enums.h"
#include "geometry/PolySet.h"
#include "geometry/Geometry.h"

std::unique_ptr<PolySet> applyHull(const Geometry::Geometries& children);
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
