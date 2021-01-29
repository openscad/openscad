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
			bboxes.push_back(*bbox);
		}
		else if (auto *cuboid = boost::get<CGAL_Iso_cuboid_3>(&x)) {
			cuboids.push_back(*cuboid);
		}
		else {
			assert(!"Unknown bbox type");
		}
	}

	bool intersects(const BoundingBoxoid &x)
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

private:
	bool intersects_bboxes(const BoundingBox &b)
	{
		for (auto &bbox : bboxes) {
			if (bbox.intersects(b)) {
				return true;
			}
		}
		return false;
	}

	bool intersects_cuboids(const CGAL_Iso_cuboid_3 &c)
	{
		for (auto &cuboid : cuboids) {
			if (CGAL::intersection(c, cuboid).has_value()) {
				return true;
			}
		}
		return false;
	}

	std::vector<BoundingBox> bboxes;
	std::vector<CGAL_Iso_cuboid_3> cuboids;
};
