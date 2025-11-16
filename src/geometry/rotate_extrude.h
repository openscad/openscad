#pragma once

#include <memory>

#include "core/RotateExtrudeNode.h"
#include "geometry/Geometry.h"
#include "geometry/Polygon2d.h"

std::unique_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly);
