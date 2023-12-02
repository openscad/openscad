// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include "Polygon2d.h"
#include "PolySet.h"

namespace roof_vd {
std::unique_ptr<PolySet> voronoi_diagram_roof(const Polygon2d& poly, double fa, double fs);
}
