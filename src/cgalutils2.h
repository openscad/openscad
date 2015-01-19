#pragma once

#include "GeometryUtils.h"
#include "polyset.h"

namespace CGALUtils {
	bool applyHull(const std::vector<Vector3d>& points, PolySet &P);
};
