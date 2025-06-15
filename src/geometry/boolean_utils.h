#pragma once

#include <memory>
#include "geometry/PolySet.h"
#include "geometry/Geometry.h"

std::unique_ptr<Geometry> applyHull(const Geometry::Geometries& children);
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
