#pragma once

#include <memory>
#include "Polygon2d.h"
#include "Geometry.h"
#include "src/core/RotateExtrudeNode.h"

std::shared_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly);
