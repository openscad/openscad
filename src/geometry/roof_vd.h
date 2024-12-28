// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <memory>
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"

namespace roof_vd {
std::unique_ptr<PolySet> voronoi_diagram_roof(const Polygon2d& poly, double fa, double fs);
}
