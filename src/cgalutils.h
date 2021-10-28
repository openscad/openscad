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

namespace CGALUtils {

	bool applyHull(const Geometry::Geometries &children, PolySet &P);
	template<typename K>
	bool is_weakly_convex(const CGAL::Polyhedron_3<K> & p);
	shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries &children, OpenSCADOperator op);
	shared_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend);
	//FIXME: Old, can be removed:
	//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut);
	template <typename K>
	CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K> &N);
	bool is_approximately_convex(const PolySet &ps);
	shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries &children);

	template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
	template <class InputKernel, class OutputKernel>
	void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel> &poly_a, CGAL::Polyhedron_3<OutputKernel> &poly_b);
	template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p, bool invert_orientation = true, bool use_grid = true);

	template <typename K>
	bool createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K> &N, PolySet &ps);
	shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromGeometry(const class Geometry &geom);
	shared_ptr<const PolySet> getGeometryAsPolySet(const shared_ptr<const Geometry>&);
	shared_ptr<const CGAL_Nef_polyhedron> getGeometryAsNefPolyhedron(const shared_ptr<const Geometry>&);

	bool tessellatePolygon(const PolygonK &polygon,
												 Polygons &triangles,
												 const K::Vector_3 *normal = nullptr);
	bool tessellatePolygonWithHoles(const PolyholeK &polygons,
																	Polygons &triangles,
																	const K::Vector_3 *normal = nullptr);
	bool tessellate3DFaceWithHoles(std::vector<CGAL_Polygon_3> &polygons, 
																 std::vector<CGAL_Polygon_3> &triangles,
																 CGAL::Plane_3<CGAL_Kernel3> &plane);
	template <typename FromKernel, typename ToKernel>
	struct KernelConverter {
		// Note: we could have this return `CGAL::to_double(n)` by default, but
		// that would mean that failure to provide a proper specialization would
		// default to lossy conversion.
		typename ToKernel::FT operator()(const typename FromKernel::FT &n) const;
	};
	template <typename FromKernel, typename ToKernel>
	CGAL::Cartesian_converter<FromKernel, ToKernel, KernelConverter<FromKernel, ToKernel>>
	getCartesianConverter()
	{
		return CGAL::Cartesian_converter<
			FromKernel, ToKernel, KernelConverter<FromKernel, ToKernel>>();
	}
	template <typename K>
	void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<K> &nef, CGAL::Polyhedron_3<K> &polyhedron);
};
