#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/Surface_mesh.h>
#include "Reindexer.h"

namespace CGALUtils {

namespace PMP = CGAL::Polygon_mesh_processing;


template <typename TriangleMesh>
void repairMesh(TriangleMesh& tm)
{
  typedef boost::graph_traits<TriangleMesh> GT;
  typedef typename GT::face_descriptor face_descriptor;
  typedef typename GT::halfedge_descriptor halfedge_descriptor;
  typedef typename GT::edge_descriptor edge_descriptor;
  typedef typename GT::vertex_descriptor vertex_descriptor;

  auto rewriteMesh = [&]() {
      TriangleMesh copy;
      copyMesh(tm, copy);
      tm = copy;
    };

  auto debug = Feature::ExperimentalFastCsgDebug.is_enabled();

  auto addedVertexCount = PMP::duplicate_non_manifold_vertices(tm);
  if (addedVertexCount) {
    if (debug) {
      LOG(message_group::None, Location::NONE, "", "[fast-csg-repair] Added %1$s vertices to avoid non manifold vertices", addedVertexCount);
    }
    rewriteMesh();
  }

  if (!CGAL::is_closed(tm)) {
    std::vector<face_descriptor> facesAdded;
    size_t holeCount = 0;

    // TODO(ochafik): Experiment w/ grid + stitch.
    //   #include <CGAL/Polygon_mesh_processing/internal/repair_extra.h>
    //   std::vector<std::pair<halfedge_descriptor, halfedge_descriptor>> halfEdgesToStitch;
    //   // Does not compile with Epeck yet!
    //   PMP::collect_close_stitchable_boundary_edges(tm, /* epsilon= */ GRID_FINE, get(boost::vertex_point, tm), halfEdgesToStitch);
    //   PMP::stitch_borders(tm, halfEdgesToStitch);
    //   // Or simply PMP::stitch_borders(tm); after throwing border vertices into the grid?

    for (auto& he : tm.halfedges()) {
      // if (!tm.is_border(he)) continue;
      if (tm.face(he).is_valid()) continue;
      PMP::triangulate_hole(tm, he, back_inserter(facesAdded));
      holeCount++;
    }

    if (debug) {
      LOG(message_group::None, Location::NONE, "", "[fast-csg-repair] Triangulated %1$lu holes with %2$lu new faces", holeCount, facesAdded.size());
    }

    std::vector<std::pair<face_descriptor, face_descriptor>> selfIntersectionPairs;
    PMP::self_intersections(facesAdded, tm, back_inserter(selfIntersectionPairs));

    if (!selfIntersectionPairs.empty()) {
      size_t removedCount = 0;
      for (auto& p : selfIntersectionPairs) {
        auto& f = p.first;
        if (!tm.is_removed(f)) {
          tm.remove_face(f);
          removedCount++;
        }
      }
      if (debug) {
        LOG(message_group::None, Location::NONE, "", "[fast-csg-repair] Patching holes created %1$lu pairs of self-intersecting faces. Removed %2$lu offending hole-patching faces.", selfIntersectionPairs.size(), removedCount);
      }
    }

    if (!facesAdded.empty()) {
      rewriteMesh();
    }
  }
}

template void repairMesh(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& tm);

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

