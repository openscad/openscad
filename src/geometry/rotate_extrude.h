#pragma once

#include <memory>

#include "core/RotateExtrudeNode.h"
#include "geometry/Geometry.h"
#include "geometry/Polygon2d.h"

std::unique_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly);
std::unique_ptr<Geometry> rotateBarcode(const RotateExtrudeNode& node, const Barcode1d& poly);
