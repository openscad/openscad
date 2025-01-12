// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <memory>
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"

namespace roof_ss {
std::unique_ptr<PolySet> straight_skeleton_roof(const Polygon2d& poly);
}
