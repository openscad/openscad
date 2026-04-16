#pragma once

#include <memory>

#include "geometry/Geometry.h"

std::shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries& children);
