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

namespace /* anonymous */ {
        template<typename Result, typename V>
        Result vector_convert(V const& v) {
                return Result(CGAL::to_double(v[0]),CGAL::to_double(v[1]),CGAL::to_double(v[2]));
       	}
}

namespace CSGIF_Utils {
	bool applyHull(const Geometry::ChildList &children, PolySet &P);
	CSGIF_polyhedron *applyOperator(const Geometry::ChildList &children, OpenSCADOperator op);
	//FIXME: Old, can be removed:
	//void applyBinaryOperator(CSGIF_polyhedron &target, const CSGIF_polyhedron &src, OpenSCADOperator op);
	Polygon2d *project(const CSGIF_polyhedron &N, bool cut);
	bool is_approximately_convex(const PolySet &ps);
	Geometry const* applyMinkowski(const Geometry::ChildList &children);

	CSGIF_polyhedron *createCsgPolyhedronFromGeometry(const class Geometry &geom);

	bool createPolySetFromCsgPolyhedron(const CSGIF_polyhedron &N, PolySet &ps);
}

//note: only for use within CGAL specific code
namespace CGALUtils {
	CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3 &N);
	template <typename Polyhedron> std::string printPolyhedron(const Polyhedron &p);
	template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
	template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p);
	template <class Polyhedron_A, class Polyhedron_B> 
	void copyPolyhedron(const Polyhedron_A &poly_a, Polyhedron_B &poly_b);

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
}
