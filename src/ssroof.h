#pragma once

#include "Polygon2d.h"
#include "polyset.h"

PolySet *straight_skeleton_roof(const Polygon2d &poly);
PolySet *voronoi_diagram_roof(const Polygon2d &poly);
