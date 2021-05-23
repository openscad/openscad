// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include "Polygon2d.h"
#include "polyset.h"

namespace warp_ns {
    shared_ptr<PolySet> warp(const PolySet &geom, double grid_x, double grid_y, double grid_z, double R, double r);
}
