#pragma once

#include "cgal.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include "enums.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
typedef CGAL::Epick K;
typedef CGAL::Point_3<K> Vertex3K;
typedef std::vector<Vertex3K> PolygonK;
typedef std::vector<PolygonK> PolyholeK;

namespace CGALUtils {
	bool applyHull(const Geometry::ChildList &children, PolySet &P);
	CGAL_Nef_polyhedron *applyOperator(const Geometry::ChildList &children, OpenSCADOperator op);
	//FIXME: Old, can be removed:
	//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut);
	CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3 &N);
	bool is_approximately_convex(const PolySet &ps);
	Geometry const* applyMinkowski(const Geometry::ChildList &children);

	template <typename Polyhedron> std::string printPolyhedron(const Polyhedron &p);
	template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
	template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p);
	template <class Polyhedron_A, class Polyhedron_B> 
	void copyPolyhedron(const Polyhedron_A &poly_a, Polyhedron_B &poly_b);

	CGAL_Nef_polyhedron *createNefPolyhedronFromGeometry(const class Geometry &geom);
	bool createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3 &N, PolySet &ps);

	bool tessellatePolygon(const PolygonK &polygon,
												 Polygons &triangles,
												 const K::Vector_3 *normal = NULL);
	bool tessellatePolygonWithHoles(const PolyholeK &polygons,
																	Polygons &triangles,
																	const K::Vector_3 *normal = NULL);
	bool tessellate3DFaceWithHoles(std::vector<CGAL_Polygon_3> &polygons, 
																 std::vector<CGAL_Polygon_3> &triangles,
																 CGAL::Plane_3<CGAL_Kernel3> &plane);
};
