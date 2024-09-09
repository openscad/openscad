#pragma once

#include <memory>
#include "Polygon2d.h"
#include "Geometry.h"
#include "LinearExtrudeNode.h"

std::unique_ptr<Geometry> extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly);
