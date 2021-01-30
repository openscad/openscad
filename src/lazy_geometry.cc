/*
 * OpenSCAD (https://www.openscad.org)
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "mixed_cache.h"
#include "polyset.h"
#include "node.h"
#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "cgal.h"
#include "cgalutils.h"

#include "bounding_boxes.h"
#include "lazy_geometry.h"

size_t LazyGeometry::getNumberOfFacets() const
{
	if (!geom) return 0;
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return polyset->numFacets();
	}
	else if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		return nef->p3->number_of_facets();
	}
	else {
		assert(!"Unsupported geom type");
		return 0;
	}
}

shared_ptr<const Geometry> LazyGeometry::getGeom() const
{
	return geom;
}
bool LazyGeometry::isPolySet() const
{
	return dynamic_pointer_cast<const PolySet>(geom).get() != nullptr;
}
bool LazyGeometry::isNef() const
{
	return dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom).get() != nullptr;
}

BoundingBoxes::BoundingBoxoid computeBoundingBox(const shared_ptr<const Geometry> &geom)
{
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return polyset->getBoundingBox();
	}
	else {
		auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
		assert(nef);
		auto res = CGALUtils::boundingBox(*nef->p3);
		return res;
	}
}

shared_ptr<const BoundingBoxes> LazyGeometry::getBoundingBoxes() const
{
	if (!bboxes && geom) {
		auto bbox = computeBoundingBox(geom);
		auto new_bboxes = make_shared<BoundingBoxes>();
		new_bboxes->add(bbox);
		bboxes = new_bboxes;
	}
	return bboxes;
}

LazyGeometry LazyGeometry::operator+(const LazyGeometry &other) const
{
	if (!geom) return other;
	if (!other.geom) return *this;

	auto ourBoxes = getBoundingBoxes();
	auto othersBoxes = other.getBoundingBoxes();
	auto result_bboxes = make_shared<BoundingBoxes>(*ourBoxes);
	*result_bboxes += *othersBoxes;

	shared_ptr<const Geometry> result;

	if (ourBoxes->intersects(*othersBoxes)) {
		LOG(message_group::Echo, location, "",
				"Doing full union of potentially overlapping geometries");
		result = joinProbablyOverlapping(other);
	}
	else {
		LOG(message_group::Echo, location, "", "Doing fast-union of disjoint geometries");
		result = concatenateDisjoint(other);
	}

	// Note: we could cache intermediate results with a composite, normalized
	// key (e.g. with sorted keys of components of the union) but this can
	// cause lots of cache evictions and be counterproductive.
	return LazyGeometry(result, result_bboxes, location, "");
}

shared_ptr<const Geometry> LazyGeometry::concatenateDisjoint(const LazyGeometry &other) const
{
	// No matter what, we don't care if we have nefs here.
	// The assumption is that joining two nefs is always more costly than creating
	// the nef resulting from joining the polysets (also, it's not even sure we'd
	// need a Nef result), but that might not be true in all cases.
	auto concatenation = shared_ptr<PolySet>(new PolySet(*getPolySet()));
	concatenation->append(*other.getPolySet());
	concatenation->quantizeVertices();

#ifndef OPTIMISTIC_FAST_UNION
	// Very costly detection of pathological cases (can represent more than half
	// the render time for purely-fast-unioned assemblies).
	CGAL_Polyhedron P;
	auto err = CGALUtils::createPolyhedronFromPolySet(*concatenation, P);
	if (err || !P.is_closed() || !P.is_valid(false, 0)) {
		LOG(message_group::Warning, location, "",
				"Fast union result couldn't be converted to a polyhedron. Reverting to slower full "
				"union.");
		return joinProbablyOverlapping(other);
	}
#endif
	return concatenation;
}

shared_ptr<const Geometry> LazyGeometry::joinProbablyOverlapping(const LazyGeometry &other) const
{
	auto thisNef = getNef();
	auto otherNef = other.getNef();
	if (!thisNef || !thisNef->p3) return otherNef;
	if (!otherNef || !otherNef->p3) return thisNef;

	return make_shared<CGAL_Nef_polyhedron>(*thisNef + *otherNef);
}

shared_ptr<const CGAL_Nef_polyhedron> LazyGeometry::getNef() const
{
	if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		return nef;
	}
	return MixedCache::getOrSet<CGAL_Nef_polyhedron>(cacheKey, [&]() {
		return shared_ptr<const CGAL_Nef_polyhedron>(CGALUtils::createNefPolyhedronFromGeometry(*geom));
	});
}

shared_ptr<const PolySet> LazyGeometry::getPolySet() const
{
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return polyset;
	}
	return MixedCache::getOrSet<PolySet>(cacheKey, [&]() {
		auto polyset = make_shared<PolySet>(3);
		auto nef = getNef();
		assert(nef);
		bool err = CGALUtils::createPolySetFromNefPolyhedron3(*nef->p3, *polyset);
		if (err) {
			LOG(message_group::Error, location, "", "LazyGeometry::getPolySet: Nef->PolySet failed");
		}
		return polyset;
	});
}
