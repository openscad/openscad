#pragma once

#include "cgal.h"
#include <vector>

/*!
 * A set of bounding boxes that can be in double or cartesian coordinates
 * (best precision occurs when not mixing the two)
 */
class BoundingBoxes
{
public:
	static CGAL_Iso_cuboid_3 bboxToIsoCuboid(const BoundingBox &bbox)
	{
		auto &min = bbox.min();
		auto &max = bbox.max();

		return CGAL_Iso_cuboid_3(NT3(min.x()), NT3(min.y()), NT3(min.z()), NT3(max.x()), NT3(max.y()),
														 NT3(max.z()));
	}

	// static double to_double(const NT3& nt) {
	//   return nt.numerator().to_double() / nt.denominator().to_double();
	// }

	static BoundingBox isoCuboidToBbox(const CGAL_Iso_cuboid_3 &cuboid)
	{
		auto &min = cuboid.min();
		auto &max = cuboid.max();
		return BoundingBox(Eigen::Vector3d(to_double(min.x()), to_double(min.y()), to_double(min.z())),
											 Eigen::Vector3d(to_double(max.x()), to_double(max.y()), to_double(max.z())));
	}

	void add(const BoundingBox &b) { bboxes.push_back(b); }
	void add(const CGAL_Iso_cuboid_3 &b) { cuboids.push_back(b); }

	static bool cuboidsIntersect(const CGAL_Iso_cuboid_3 &a, const CGAL_Iso_cuboid_3 &b)
	{
		auto ba = isoCuboidToBbox(a);
		auto bb = isoCuboidToBbox(b);
		auto result = CGAL::intersection(a, b).has_value();
		return result;
	}

	bool intersects(const BoundingBox &b)
	{
		return intersects_bboxes(b) || intersects_cuboids(bboxToIsoCuboid(b));
	}

	bool intersects(const CGAL_Iso_cuboid_3 &c)
	{
		return intersects_cuboids(c) || intersects_bboxes(isoCuboidToBbox(c));
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
			if (cuboidsIntersect(c, cuboid)) {
				return true;
			}
		}
		return false;
	}

	std::vector<BoundingBox> bboxes;
	std::vector<CGAL_Iso_cuboid_3> cuboids;
};
