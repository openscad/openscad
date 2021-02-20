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

class CGALHybridPolyhedron;

namespace CGAL {
	template <class A, class B, class C>
	class Cartesian_converter;
}

#ifdef FAST_CSG_AVAILABLE
namespace CGAL {
	inline std::size_t hash_value(const CGAL_HybridKernel3::FT &x) {
		std::hash<double> dh;
		return dh(CGAL::to_double(x));
	}
}
#endif // FAST_CSG_AVAILABLE

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
	shared_ptr<const Geometry> applyOperator3D(const Geometry::Geometries &children, OpenSCADOperator op);
	shared_ptr<const Geometry> applyUnion3D(Geometry::Geometries::iterator chbegin, Geometry::Geometries::iterator chend);
	shared_ptr<CGALHybridPolyhedron> applyUnion3DHybrid(
		const Geometry::Geometries::const_iterator &chbegin,
		const Geometry::Geometries::const_iterator &chend);
	shared_ptr<CGALHybridPolyhedron> applyOperator3DHybrid(const Geometry::Geometries &children, OpenSCADOperator op);
	//FIXME: Old, can be removed:
	//void applyBinaryOperator(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut);
	template <typename K>
	CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K> &N);
	template <typename K>
	CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Polyhedron_3<K> &poly);
	CGAL_Iso_cuboid_3 boundingBox(const Geometry&geom);
	CGAL_Point_3 center(const CGAL_Iso_cuboid_3 &cuboid);
	CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox &bbox);
	template <typename K>
	CGAL::Point_3<K> vector3dToPoint3(const Eigen::Vector3d& v);
	template <typename K>
	BoundingBox createBoundingBoxFromIsoCuboid(const CGAL::Iso_cuboid_3<K> &bbox);
	size_t getNumFacets(const Geometry& geom);
	bool is_approximately_convex(const PolySet &ps);
	shared_ptr<const Geometry> applyMinkowski(const Geometry::Geometries &children);

	template <typename Polyhedron> std::string printPolyhedron(const Polyhedron &p);
	template <typename Polyhedron> bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
	template <typename Polyhedron> bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p, bool invert_orientation = true, bool use_grid = true);
	template <class InputKernel, class OutputKernel>
	void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel> &poly_a, CGAL::Polyhedron_3<OutputKernel> &poly_b);

	shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromGeometry(const class Geometry &geom);

	shared_ptr<const PolySet> getGeometryAsPolySet(const shared_ptr<const Geometry>&);
	shared_ptr<const CGAL_Nef_polyhedron> getGeometryAsNefPolyhedron(const shared_ptr<const Geometry>&);

	template <typename K>
	bool createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K> &N, PolySet &ps);
	template <typename K>
	CGAL::Aff_transformation_3<K> createAffineTransformFromMatrix(const Transform3d &matrix);
	template <typename K>
	void transform(CGAL::Nef_polyhedron_3<K> &N, const Transform3d &matrix);
	template <typename K>
	void transform(CGAL::Polyhedron_3<K> &N, const Transform3d &matrix);
	template <typename K>
	Transform3d computeResizeTransform(
		const CGAL::Iso_cuboid_3<K>& bb, int dimension, const Vector3d &newsize,
		const Eigen::Matrix<bool,3,1> &autosize);

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
	shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(const CGALHybridPolyhedron &hybrid);
	std::shared_ptr<CGALHybridPolyhedron> createHybridPolyhedronFromGeometry(const Geometry &geom);
	template <typename Polyhedron>
	void triangulateFaces(Polyhedron &polyhedron);
	template <typename Polyhedron>
	bool isClosed(Polyhedron &polyhedron);
	template <typename Polyhedron>
	void orientToBoundAVolume(Polyhedron &polyhedron);
	template <typename K>
	void inPlaceNefUnion(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs);
	template <typename K>
	void inPlaceNefDifference(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs);
	template <typename K>
	void inPlaceNefIntersection(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs);
	template <typename K>
	void inPlaceNefMinkowski(CGAL::Nef_polyhedron_3<K> &lhs, CGAL::Nef_polyhedron_3<K> &rhs);
	template <typename K>
	void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<K> &nef, CGAL::Polyhedron_3<K> &polyhedron);
	template <typename K>
	bool corefineAndComputeUnion(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs, CGAL::Polyhedron_3<K> &out);
	template <typename K>
	bool corefineAndComputeIntersection(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs, CGAL::Polyhedron_3<K> &out);
	template <typename K>
	bool corefineAndComputeDifference(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs, CGAL::Polyhedron_3<K> &out);
};
