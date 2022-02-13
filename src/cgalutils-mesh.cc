#include "cgalutils.h"

#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/Surface_mesh.h>
#include "Reindexer.h"

namespace CGALUtils {

template <typename K>
bool createMeshFromPolySet(const PolySet& ps, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh)
{
  bool err = false;
  auto num_vertices = ps.numFacets() * 3;
  auto num_facets = ps.numFacets();
  auto num_edges = num_vertices + num_facets + 2; // Euler's formula.
  mesh.reserve(mesh.number_of_vertices() + num_vertices, mesh.number_of_halfedges() + num_edges,
               mesh.number_of_faces() + num_facets);

  typedef typename CGAL::Surface_mesh<CGAL::Point_3<K>>::Vertex_index Vertex_index;
  std::vector<Vertex_index> polygon;

  std::unordered_map<Vector3d, Vertex_index> indices;

  for (const auto& p : ps.polygons) {
    polygon.clear();
    for (auto& v : p) {
      auto size_before = indices.size();
      auto& index = indices[v];
      if (size_before != indices.size()) {
        index = mesh.add_vertex(vector_convert<CGAL::Point_3<K>>(v));
      }
      polygon.push_back(index);
    }
    mesh.add_face(polygon);
  }
  return err;
}

template bool createMeshFromPolySet(const PolySet& ps, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh);

template <typename K>
bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, PolySet& ps)
{
  bool err = false;
  ps.reserve(ps.numFacets() + mesh.number_of_faces());
  for (auto& f : mesh.faces()) {
    ps.append_poly();

    CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<K>>> vbegin, vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(f), mesh); vbegin != vend;
         ++vbegin) {
      auto& v = mesh.point(*vbegin);
      // for (auto &v : f) {
      double x = CGAL::to_double(v.x());
      double y = CGAL::to_double(v.y());
      double z = CGAL::to_double(v.z());
      ps.append_vertex(x, y, z);
    }
  }
  return err;
}

template bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh, PolySet& ps);

template <class InputKernel, class OutputKernel>
void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output)
{
  typedef CGAL::Surface_mesh<CGAL::Point_3<InputKernel>> InputMesh;
  typedef CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>> OutputMesh;

  auto converter = getCartesianConverter<InputKernel, OutputKernel>();
  output.reserve(output.number_of_vertices() + input.number_of_vertices(),
                 output.number_of_halfedges() + input.number_of_halfedges(),
                 output.number_of_faces() + input.number_of_faces());

  std::vector<typename CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>::Vertex_index> polygon;
  std::unordered_map<typename InputMesh::Vertex_index, typename OutputMesh::Vertex_index> reindexer;
  for (auto face : input.faces()) {
    polygon.clear();

    CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>>
    vbegin, vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(input.halfedge(face), input);
         vbegin != vend; ++vbegin) {
      auto input_vertex = *vbegin;
      auto size_before = reindexer.size();
      auto& output_vertex = reindexer[input_vertex];
      if (size_before != reindexer.size()) {
        output_vertex = output.add_vertex(converter(input.point(input_vertex)));
      }
      polygon.push_back(output_vertex);
    }
    output.add_face(polygon);
  }
}

template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL_Point_3>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& input,
  CGAL::Surface_mesh<CGAL_Point_3>& output);
template void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>>& output);

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh)
{
  CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, /* triangulate_all_faces */ true);
}

template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_Kernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>>& mesh);
template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh);

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
void cleanupMesh(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& mesh, bool is_corefinement_result)
{
  mesh.collect_garbage();

#ifdef ENABLE_CGAL_REMESHING
  if (is_corefinement_result && Feature::ExperimentalFastCsgRemesh.is_enabled()) {
    CGALUtils::remeshPlanarPatches(mesh);
  }
#endif // ENABLE_CGAL_REMESHING

#if FAST_CSG_KERNEL_IS_LAZY
  // If exact corefinement callbacks are enabled, no need to make numbers exact here again.
  auto make_exact =
    Feature::ExperimentalFastCsgExactCorefinementCallback.is_enabled()
      ? !is_corefinement_result
      : Feature::ExperimentalFastCsgExact.is_enabled();

  if (make_exact) {
    for (auto v : mesh.vertices()) {
      auto& pt = mesh.point(v);
      CGAL::exact(pt.x());
      CGAL::exact(pt.y());
      CGAL::exact(pt.z());
    }
  }
#endif // FAST_CSG_KERNEL_IS_LAZY
}

} // namespace CGALUtils

