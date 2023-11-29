#pragma once

#include "enums.h"
#include "PolySet.h"
#include "Geometry.h"

bool applyHull(const Geometry::Geometries& children, PolySet& P);
shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
