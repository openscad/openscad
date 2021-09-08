// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include "Polygon2d.h"
#include "polyset.h"

namespace roof_ss {
    PolySet *straight_skeleton_roof(const Polygon2d &poly);
}
