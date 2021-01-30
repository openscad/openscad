/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright 2021 Google LLC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#include <set>
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "bounding_boxes.h"
#include "cgal.h"
#include "cgalutils.h"

/*! Holds a Geometry and allows deferred conversion to Nef or PolySet. Also,
 * looks up the cache using the provided keys.
 */
class LazyGeometry
{
public:
	LazyGeometry() : location(Location::NONE) {}
	LazyGeometry(const LazyGeometry &other)
		: geom(other.geom), bboxes(other.bboxes), location(other.location), cacheKey(other.cacheKey)
	{
	}

	// Used for leaf geometry that actually has a node attached (usually).
	LazyGeometry(const shared_ptr<const Geometry> &geom, const Location &location,
							 const std::string &cacheKey)
		: geom(geom), location(location), cacheKey(cacheKey)
	{
		assert(geom);
		assert(isPolySet() || isNef());
	}

	// Used for intermediate results.
	LazyGeometry(const shared_ptr<const Geometry> &geom,
							 const shared_ptr<const BoundingBoxes> &bboxes, const Location &location,
							 const std::string &cacheKey)
		: geom(geom), bboxes(bboxes), location(location), cacheKey(cacheKey)
	{
		assert(geom);
		assert(isPolySet() || isNef());
	}

	size_t getNumberOfFacets() const;
	bool empty() const { return getGeom().get() != nullptr; }
	operator bool() const { return !empty(); }
	operator shared_ptr<const Geometry>() const { return getGeom(); }

	shared_ptr<const Geometry> getGeom() const;
	bool isPolySet() const;
	bool isNef() const;

	/*! May incur a big PolySet->Nef conversion cost if not already a nef or if
	 * the nef isn't in the Geometry cache. */
	shared_ptr<const CGAL_Nef_polyhedron> getNef() const;

	/*! May incur a moderate Nef->PolySet conversion cost if not already a polyset
	 * the nef isn't in the CGAL cache. */
	shared_ptr<const PolySet> getPolySet() const;

	/*! Returns a "meta bounding box" (collection of bboxes of unioned components
	 * of this geometry. */
	shared_ptr<const BoundingBoxes> getBoundingBoxes() const;

	/*! Unions this with the other geometry. Uses bounding boxes to detect and
	 * optimize non overlapping cases. */
	LazyGeometry operator+(const LazyGeometry &other) const;

private:
	/*! Does a union assuming this geometry and other are physically disjoint,
	 * i.e. their polygons don't intersect and none of them is containing the
	 * other. */
	shared_ptr<const Geometry> concatenateDisjoint(const LazyGeometry &other) const;

	/*! Does a union assuming the two geometries are likely to intersect. */
	shared_ptr<const Geometry> joinProbablyOverlapping(const LazyGeometry &other) const;

	// The underlying geometry. Immutable.
	// Note that (Nef or PolySet) conversions of this geometry may live in the
	// relevant cache, which getNef and getPolySet will look up before attempting
	// any conversion.
	shared_ptr<const Geometry> geom;
	mutable shared_ptr<const BoundingBoxes> bboxes;
	Location location;
	std::string cacheKey;
};
