// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#pragma once

#include "cgal.h"
#include <vector>
#include <boost/variant.hpp>

/*!
 * A set of bounding boxes. Useful during unions of many geometries to know
 * if a new geometry intersects with any of the previously unioned ones.
 */
template <typename K>
class BoundingBoxes
{
public:
	void clear() { cuboids.clear(); }
	size_t memsize() const
	{
		return sizeof(BoundingBoxes) + cuboids.size() * sizeof(CGAL::Iso_cuboid_3<K>);
	}

	void add(const CGAL::Iso_cuboid_3<K> &c) { cuboids.push_back(c); }

	bool empty() const { return cuboids.empty(); }

	BoundingBoxes &operator+=(const BoundingBoxes &other)
	{
		cuboids.insert(cuboids.end(), other.cuboids.begin(), other.cuboids.end());
		return *this;
	}
	BoundingBoxes &operator*=(const BoundingBoxes &other)
	{
		std::vector<CGAL::Iso_cuboid_3<K>> survivors;
		for (auto &cuboid : cuboids) {
			if (other.intersects(cuboid)) survivors.push_back(cuboid);
		}
		cuboids = survivors;
		return *this;
	}

	/*! Whether these bounding boxes have any intersection with the provided one.
	 * Beware: currently implemented in linear time! (CGAL has ways to improve
	 * that)
	 */
	bool intersects(const CGAL::Iso_cuboid_3<K> &c) const
	{
		for (auto &cuboid : cuboids) {
			if (CGAL::intersection(c, cuboid) != boost::none) {
				return true;
			}
		}
		return false;
	}

	/*! Whether these bounding boxes have any intersection with any of the other.
	 * Beware: currently implemented in quadratic time! (CGAL has ways to improve
	 * that)
	 */
	bool intersects(const BoundingBoxes &other) const
	{
		for (auto &cuboid : cuboids) {
			if (other.intersects(cuboid)) return true;
		}
		return false;
	}

private:
	std::vector<CGAL::Iso_cuboid_3<K>> cuboids;
};
