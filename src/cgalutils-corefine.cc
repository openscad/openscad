// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/internal/Corefinement/face_graph_utils.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> CGAL_SurfaceMesh;

/*! Visitor that forces exact numbers for the vertices of all the faces created during corefinement.
 */
template <typename TriangleMesh>
struct ExactNumbersVisitor
	: public CGAL::Polygon_mesh_processing::Corefinement::Default_visitor<TriangleMesh> {
	typedef typename TriangleMesh::Face_index face_descriptor;

	face_descriptor split_face;
	std::vector<face_descriptor> created_faces;

	void before_subface_creations(face_descriptor f_split, TriangleMesh &tm)
	{
		split_face = f_split;
		created_faces.clear();
	}

	void after_subface_creations(TriangleMesh &mesh)
	{
		for (auto &fi : created_faces) {
			CGAL::Vertex_around_face_iterator<TriangleMesh> vbegin, vend;
			for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(fi), mesh); vbegin != vend;
					 ++vbegin) {
				auto &v = mesh.point(*vbegin);
				CGAL::exact(v.x());
				CGAL::exact(v.y());
				CGAL::exact(v.z());
			}
		}
		created_faces.clear();
	}
	void after_subface_created(face_descriptor fi, TriangleMesh &tm) { created_faces.push_back(fi); }
};

template <typename K>
bool corefineAndComputeUnion(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
														 CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_union(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeIntersection(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
																		CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeDifference(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
																	CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(lhs, rhs, out);
}

template bool corefineAndComputeUnion(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																			CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																			CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);
template bool corefineAndComputeIntersection(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																						 CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																						 CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);
template bool corefineAndComputeDifference(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																					 CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																					 CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);

template <typename K>
bool corefineAndComputeUnion(CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
														 CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
														 CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
#ifdef FAST_CSG_KERNEL_IS_LAZY
	if (Feature::ExperimentalFastCsgExactCallback.is_enabled()) {
		return CGAL::Polygon_mesh_processing::corefine_and_compute_union(
				lhs, rhs, out,
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()),
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()));
	}
	else
#endif
	{
		return CGAL::Polygon_mesh_processing::corefine_and_compute_union(lhs, rhs, out);
	}
}

template <typename K>
bool corefineAndComputeIntersection(CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
																		CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
																		CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
#ifdef FAST_CSG_KERNEL_IS_LAZY
	if (Feature::ExperimentalFastCsgExactCallback.is_enabled()) {
		return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(
				lhs, rhs, out,
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()),
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()));
	}
	else
#endif
	{
		return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(lhs, rhs, out);
	}
}

template <typename K>
bool corefineAndComputeDifference(CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
																	CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
																	CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
#ifdef FAST_CSG_KERNEL_IS_LAZY
	if (Feature::ExperimentalFastCsgExactCallback.is_enabled()) {
		return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(
				lhs, rhs, out,
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()),
				CGAL::Polygon_mesh_processing::parameters::visitor(
						ExactNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()));
	}
	else
#endif
	{
		return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(lhs, rhs, out);
	}
}

template bool corefineAndComputeUnion(CGAL_SurfaceMesh &lhs, CGAL_SurfaceMesh &rhs,
																			CGAL_SurfaceMesh &out);
template bool corefineAndComputeIntersection(CGAL_SurfaceMesh &lhs, CGAL_SurfaceMesh &rhs,
																						 CGAL_SurfaceMesh &out);
template bool corefineAndComputeDifference(CGAL_SurfaceMesh &lhs, CGAL_SurfaceMesh &rhs,
																					 CGAL_SurfaceMesh &out);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE
