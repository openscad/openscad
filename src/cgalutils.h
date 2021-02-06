#pragma once

#include "cgal.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include "enums.h"

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#pragma pop_macro("NDEBUG")
typedef CGAL::Epick K;
typedef CGAL::Point_3<K> Vertex3K;
typedef std::vector<Vertex3K> PolygonK;
typedef std::vector<PolygonK> PolyholeK;

class Tree;

namespace CGAL {
  template <typename Point>
  class Surface_mesh;
  template <class A, class B, class C>
  class Cartesian_converter;
}

namespace /* anonymous */ {
        template<typename Result, typename V>
        Result vector_convert(V const& v) {
                return Result(CGAL::to_double(v[0]),CGAL::to_double(v[1]),CGAL::to_double(v[2]));
       	}
}

namespace CGALUtils {

	class CGALErrorBehaviour {
	public:
		CGALErrorBehaviour(CGAL::Failure_behaviour behaviour) {
			old_behaviour = CGAL::set_error_behaviour(behaviour);
		}
		~CGALErrorBehaviour() {
			CGAL::set_error_behaviour(old_behaviour);
		}
	private:
		CGAL::Failure_behaviour old_behaviour;
	};

	bool applyHull(const Geometry::Geometries &children, PolySet &P);
	shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries &children, OpenSCADOperator op, const Tree* tree = nullptr);
	shared_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend,
		const Tree* tree = nullptr);
	//FIXME: Old, can be removed:
	//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut);
	CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3 &N);
	CGAL_Iso_cuboid_3 boundingBox(const Geometry&geom);
	CGAL_Point_3 center(const CGAL_Iso_cuboid_3 &cuboid);
	CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox &bbox);
	size_t getNumFacets(const Geometry& geom);
	bool is_approximately_convex(const PolySet &ps);
	shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries &children);

	template <typename Polyhedron> std::string printPolyhedron(const Polyhedron &p);
	template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
	template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p);
	template <class InputKernel, class OutputKernel>
	void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel> &poly_a, CGAL::Polyhedron_3<OutputKernel> &poly_b);
	template <class InputKernel, class OutputKernel>
	void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>> &input, CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>> &output);
	template <class Polyhedron_A, class Polyhedron_B>
	void appendToPolyhedron(const Polyhedron_A &poly_a, Polyhedron_B &poly_b);

	CGAL_Nef_polyhedron *createNefPolyhedronFromGeometry(const class Geometry &geom);
	template <typename K>
	bool createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K> &N, PolySet &ps);
	template <typename K>
	bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh, PolySet &ps);
	template <typename K>
	bool createMeshFromPolySet(const PolySet &ps, CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh);
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
};
