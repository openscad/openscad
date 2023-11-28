#pragma once

#include <memory>

#include "Geometry.h"

class Polygon2d;
class PolySet;

namespace PolySetUtils {

Polygon2d *project(const PolySet& ps);
void tessellate_faces(const PolySet& inps, PolySet& outps);
bool is_approximately_convex(const PolySet& ps);

std::shared_ptr<const PolySet> getGeometryAsPolySet(const std::shared_ptr<const class Geometry>&);

}
