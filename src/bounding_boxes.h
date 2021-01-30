#pragma once

#include "cgal.h"
#include <vector>
#include <boost/variant.hpp>

/*!
 * A set of bounding boxes that can be in double or precise coordinates
 * (i.e. BoundingBox coming from PolySet data vs. CGAL_Iso_cuboid_3 coming from
 * CGAL_Nef_polyhedron data).
 *
 * Best precision occurs when not mixing the two.
 */
class BoundingBoxes
{
public:
	// Union of the two types of bounding boxes.
	typedef boost::variant<CGAL_Iso_cuboid_3, BoundingBox> BoundingBoxoid;

	static CGAL_Iso_cuboid_3 bboxToIsoCuboid(const BoundingBox &bbox)
	{
		auto &min = bbox.min();
		auto &max = bbox.max();

		return CGAL_Iso_cuboid_3(NT3(min.x()), NT3(min.y()), NT3(min.z()), NT3(max.x()), NT3(max.y()),
														 NT3(max.z()));
	}

	static BoundingBox isoCuboidToBbox(const CGAL_Iso_cuboid_3 &cuboid)
	{
		auto &min = cuboid.min();
		auto &max = cuboid.max();
		return BoundingBox(Eigen::Vector3d(to_double(min.x()), to_double(min.y()), to_double(min.z())),
											 Eigen::Vector3d(to_double(max.x()), to_double(max.y()), to_double(max.z())));
	}

	void add(const BoundingBoxoid &x)
	{
		if (auto *bbox = boost::get<BoundingBox>(&x)) {
			if (!bbox->isEmpty()) {
				bboxes.push_back(*bbox);
			}
		}
		else if (auto *cuboid = boost::get<CGAL_Iso_cuboid_3>(&x)) {
			cuboids.push_back(*cuboid);
		}
		else {
			assert(!"Unknown bbox type");
		}
	}

	bool empty() const { return bboxes.empty() && cuboids.empty(); }

	BoundingBoxes &operator+=(const BoundingBoxes &other)
	{
		bboxes.insert(bboxes.end(), other.bboxes.begin(), other.bboxes.end());
		cuboids.insert(cuboids.end(), other.cuboids.begin(), other.cuboids.end());
		return *this;
	}

	/*! Whether these bounding boxes have any intersection with the provided one.
	 * Beware: currently implemented in linear time! (CGAL has ways to improve
	 * that)
	 */
	bool intersects(const BoundingBoxoid &x) const
	{
		if (auto *bbox = boost::get<BoundingBox>(&x)) {
			return intersects_bboxes(*bbox) ||
						 (!bbox->isEmpty() && cuboids.size() && intersects_cuboids(bboxToIsoCuboid(*bbox)));
		}
		else if (auto *cuboid = boost::get<CGAL_Iso_cuboid_3>(&x)) {
			return intersects_cuboids(*cuboid) ||
						 (bboxes.size() && intersects_bboxes(isoCuboidToBbox(*cuboid)));
		}
		else {
			assert(!"Unknown bbox type");
			return false;
		}
	}

	/*! Whether these bounding boxes have any intersection with any of the other.
	 * Beware: currently implemented in quadratic time! (CGAL has ways to improve
	 * that)
	 */
	bool intersects(const BoundingBoxes &other) const
	{
		for (auto &bbox : bboxes) {
			if (other.intersects(bbox)) return true;
		}
		for (auto &cuboid : cuboids) {
			if (other.intersects(cuboid)) return true;
		}
		return false;
	}

private:
	bool intersects_bboxes(const BoundingBox &b) const
	{
		for (auto &bbox : bboxes) {
			if (bbox.intersects(b)) {
				return true;
			}
		}
		return false;
	}

	bool intersects_cuboids(const CGAL_Iso_cuboid_3 &c) const
	{
		for (auto &cuboid : cuboids) {
			if (CGAL::intersection(c, cuboid) != boost::none) {
				return true;
			}
		}
		return false;
	}

	std::vector<BoundingBox> bboxes;
	std::vector<CGAL_Iso_cuboid_3> cuboids;
};
