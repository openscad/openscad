#pragma once

#include <memory>

#include "core/LinearExtrudeNode.h"
#include "geometry/Geometry.h"
#include "geometry/Polygon2d.h"

std::unique_ptr<Geometry> extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly);
std::unique_ptr<Geometry> extrudeBarcode(const LinearExtrudeNode& node, const Barcode1d& poly);
