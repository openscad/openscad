#include "geometry/cgal/cgalutils.h"

#include <unordered_map>
#include <memory>
#include <cstddef>
#include <vector>

#include <boost/range/adaptor/transformed.hpp>
#include <CGAL/Point_3.h>
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>

#include "geometry/cgal/cgal.h"
#include "geometry/linalg.h"
#include "geometry/PolySetBuilder.h"

namespace CGALUtils {

namespace PMP = CGAL::Polygon_mesh_processing;

CGAL_DoubleMesh repairPolySet(const PolySet& ps)
{
  std::vector<CGAL_DoubleMesh::Point> points;
  std::vector<std::vector<size_t>> polygons;

  // at least 3*numFacets
  points.reserve(ps.indices.size() * 3);
  polygons.reserve(ps.indices.size());
  for (const auto& inds : ps.indices) {
    std::vector<size_t>& polygon = polygons.emplace_back();
    polygon.reserve(inds.size());
    for (const auto& ind : inds) {
      polygon.push_back(points.size());
      auto& pt = ps.vertices[ind];
      points.emplace_back(pt[0], pt[1], pt[2]);
    }
  }

  PMP::repair_polygon_soup(points, polygons);
  PMP::orient_polygon_soup(points, polygons);
  CGAL_DoubleMesh mesh;
  PMP::polygon_soup_to_polygon_mesh(points, polygons, mesh);
  return mesh;
}

template <class SurfaceMesh>
std::shared_ptr<SurfaceMesh> createSurfaceMeshFromPolySet(const PolySet& ps)
{
  auto mesh = std::make_shared<SurfaceMesh>();
  mesh->reserve(ps.vertices.size(), ps.indices.size() * 3, ps.indices.size());
  for (const auto& v : ps.vertices) {
    mesh->add_vertex(typename SurfaceMesh::Point(v[0], v[1], v[2]));
  }
  for (const auto& face : ps.indices) {
    // TODO(kintel): Technically, this could return nullface. Are we certain that won't happen here?
    mesh->add_face(face | boost::adaptors::transformed(
                            [](uint32_t i) { return typename SurfaceMesh::Vertex_index(i); }));
  }
  return mesh;
}

template std::shared_ptr<CGAL_DoubleMesh> createSurfaceMeshFromPolySet(const PolySet& ps);
template std::shared_ptr<CGAL_Kernel3Mesh> createSurfaceMeshFromPolySet(const PolySet& ps);

template <class SurfaceMesh>
std::unique_ptr<PolySet> createPolySetFromSurfaceMesh(const SurfaceMesh& mesh)
{
  //  FIXME: We may want to convert directly, without PolySetBuilder here, to maintain manifoldness, if
  //  possible.
  PolySetBuilder builder(0, mesh.number_of_faces() + mesh.number_of_faces());
  for (const auto& f : mesh.faces()) {
    builder.beginPolygon(mesh.degree(f));

    CGAL::Vertex_around_face_iterator<SurfaceMesh> vbegin, vend;
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

template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>& input,
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

    CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>> vbegin,
      vend;
    for (boost::tie(vbegin, vend) = vertices_around_face(input.halfedge(face), input); vbegin != vend;
         ++vbegin) {
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

template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>>& input,
                       CGAL_DoubleMesh& output);

}  // namespace CGALUtils
