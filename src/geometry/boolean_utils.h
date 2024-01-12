#pragma once

#include "enums.h"
#include "PolySet.h"
#include "Geometry.h"

std::unique_ptr<PolySet> applyHull(const Geometry::Geometries& children);
std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
