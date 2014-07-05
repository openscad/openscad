//
//  sweep.cpp
//
//
//  Created by Oskar Linde on 2014-05-17.
//
//

#include "sweep.h"

#include "Polygon2d.h"
#include "polyset.h"
#include "clipper-utils.h"

using namespace ClipperLib;
using namespace ClipperUtils;

namespace /* anonymous */ {

ClipperLib::Path transform_path_2d(Eigen::Projective3d const& transform,
								   ClipperLib::Path const& path) {

	ClipperLib::Path ret(path.size());

	for (int i = 0; i < path.size(); i++) {
		Eigen::Vector3d p(path[i].X / double(CLIPPER_SCALE),
						  path[i].Y / double(CLIPPER_SCALE),
						  0);

		Eigen::Vector4d r1 = transform * p.homogeneous();
		Eigen::Vector3d r = r1.hnormalized();
		ret[i].X = round(r[0] * double(CLIPPER_SCALE));
		ret[i].Y = round(r[1] * double(CLIPPER_SCALE));
	}

	return ret;
}

void sweep_2d_2d(Path const& path1, Path const& path2, Paths & solution) {
	assert(path1.size() == path2.size());
	const int n = path1.size();

	Paths triangles;
	triangles.reserve(2*n);

	for (size_t i = 0; i < n; i++) {
		Path t1,t2;

		t1.reserve(3);
		t1.push_back(path1[i]);
		t1.push_back(path2[i]);
		t1.push_back(path2[(i+1)%n]);

		t2.reserve(3);
		t2.push_back(path1[i]);
		t2.push_back(path2[(i+1)%n]);
		t2.push_back(path1[(i+1)%n]);

		if (!ClipperLib::Orientation(t1)) ReversePath(t1);
		if (!ClipperLib::Orientation(t2)) ReversePath(t2);

		triangles.push_back(t1);
		triangles.push_back(t2);
	}

	Clipper c;
	c.AddPaths(triangles, ptSubject, true);
	c.Execute(ctUnion, solution, pftNonZero, pftNonZero);
}

}


Polygon2d*
sweep2d_2d(std::vector<Eigen::Projective3d> const& path, Polygon2d const& poly) {
	ClipperLib::Paths paths = ClipperUtils::fromPolygon2d(poly);

	Clipper c;

	for (int i = 1; i < path.size(); i++) {
		ClipperLib::Paths terms;
		terms.reserve(paths.size() * 3);

		for (int j = 0; j < paths.size(); j++) {
			ClipperLib::Path path_0 = transform_path_2d(path[i-1], paths[j]);
			ClipperLib::Path path_1 = transform_path_2d(path[i],   paths[j]);

			ClipperLib::Paths result;
			::sweep_2d_2d(path_0, path_1, result);

			terms.insert(terms.end(),result.begin(),result.end());
			terms.push_back(path_0);
			terms.push_back(path_1);
		}

		c.AddPaths(terms, ClipperLib::ptSubject, true);
	}

	ClipperLib::PolyTree polytree;

	c.Execute(ctUnion, polytree, pftNonZero, pftNonZero);

	return toPolygon2d(polytree);
}
