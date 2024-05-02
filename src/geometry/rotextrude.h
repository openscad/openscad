#pragma once

#include <memory>
#include "Polygon2d.h"
#include "Geometry.h"
#include "RotateExtrudeNode.h"

std::shared_ptr<const Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly);
