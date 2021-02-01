// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#pragma once

#include <vector>
#include "Geometry.h"

class PolySet;
class Tree;

struct UnionAnalysis {
	// TODO(ochafik): Investigate changing Geometry::Geometries to a vector.
	std::vector<Geometry::Geometries> nonIntersectingGeometries;
	Geometry::Geometries otherGeometries;
};

UnionAnalysis analyzeUnion(Geometry::Geometries::iterator chbegin,
													 Geometry::Geometries::iterator chend, const Tree *tree);

shared_ptr<const PolySet> unionNonIntersecting(const Geometry::Geometries &geometries,
																							 const Tree *tree);
