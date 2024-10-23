#include "geometry/cgal/cgalutils.h"
#include "Feature.h"
#include "geometry/linalg.h"
#include "utils/hash.h"

#include <unordered_map>
#include <memory>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/Surface_mesh.h>
#include "geometry/PolySetBuilder.h"
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>

#include <cstddef>
#include <vector>

namespace CGALUtils {

namespace PMP = CGAL::Polygon_mesh_processing;

template <class TriangleMesh>
bool createMeshFromPolySet(const PolySet& ps, TriangleMesh& mesh)
{
  std::vector<typename TriangleMesh::Point> points;
  std::vector<std::vector<size_t>> polygons;

  // at least 3*numFacets
  points.reserve(ps.indices.size() * 3);
  polygons.reserve(ps.indices.size());
  for (const auto& inds : ps.indices) {
    std::vector<size_t> &polygon = polygons.emplace_back();
    polygon.reserve(inds.size());
    for (const auto &ind : inds) {
      polygon.push_back(points.size());
        auto &pt = ps.vertices[ind];
      points.push_back({pt[0], pt[1], pt[2]});
    }
  }

  PMP::repair_polygon_soup(points, polygons);
  PMP::orient_polygon_soup(points, polygons);
  PMP::polygon_soup_to_polygon_mesh(points, polygons, mesh);
  return false;
}

template bool createMeshFromPolySet(const PolySet& ps, CGAL_HybridMesh& mesh);
template bool createMeshFromPolySet(const PolySet& ps, CGAL_DoubleMesh& mesh);


template <class TriangleMesh>
std::unique_ptr<PolySet> createPolySetFromMesh(const TriangleMesh& mesh)
{
  //  FIXME: We may want to convert directly, without PolySetBuilder here, to maintain manifoldness, if possible.
  PolySetBuilder builder(0, mesh.number_of_faces()+ mesh.number_of_faces());
  for (const auto& f : mesh.faces()) {
    builder.beginPolygon(mesh.degree(f));

    CGAL::Vertex_around_face_iterator<TriangleMesh> vbegin, vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(f), mesh); vbegin != vend;
         ++vbegin) {
      auto& v = mesh.point(*vbegin);
      // for (auto &v : f) {
      double x = CGAL::to_double(v.x());
      double y = CGAL::to_double(v.y());
      double z = CGAL::to_double(v.z());
      builder.addVertex(Vector3d(x, y, z));
    }
  }
  return builder.build();
}

template std::unique_ptr<PolySet> createPolySetFromMesh(const CGAL_HybridMesh& mesh);

template <class InputKernel, class OutputKernel>
void copyMesh(
  const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
  CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>& output)
{
  using InputMesh = CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>;
  using OutputMesh = CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>;

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

template void copyMesh(const CGAL_HybridMesh& input, CGAL_HybridMesh& output);
template void copyMesh(const CGAL::Surface_mesh<CGAL_Point_3>& input, CGAL_HybridMesh& output);
template void copyMesh(const CGAL_HybridMesh& input, CGAL::Surface_mesh<CGAL_Point_3>& output);
template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& input, CGAL_HybridMesh& output);
template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& input, CGAL_DoubleMesh& output);

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh)
{
  CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, /* triangulate_all_faces */ true);
}

template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_Kernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>>& mesh);
template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL_HybridMesh& mesh);

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
void cleanupMesh(CGAL_HybridMesh& mesh, bool is_corefinement_result)
{
  mesh.collect_garbage();
#if FAST_CSG_KERNEL_IS_LAZY
  // Don't make exact again if exact corefinement callbacks already did the job.
  if (!is_corefinement_result) {
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
