#include "cgalutils.h"


#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include "grid.h"
#include "Reindexer.h"

namespace CGALUtils {

template <typename K>
bool createMeshFromPolySet(const PolySet &ps, CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh, bool use_grid)
{
	bool err = false;
	auto num_vertices = ps.numFacets() * 3;
	auto num_facets = ps.numFacets();
	auto num_edges = num_vertices + num_facets + 2; // Euler's formula.
	mesh.reserve(mesh.number_of_vertices() + num_vertices, mesh.number_of_halfedges() + num_edges,
							 mesh.number_of_faces() + num_facets);

	typedef typename CGAL::Surface_mesh<CGAL::Point_3<K>>::Vertex_index Vertex_index;
	std::vector<Vertex_index> polygon;

  if (use_grid) {
    Grid3d<int> grid(GRID_FINE);
    std::vector<Vertex_index> vertices;
    // Align all vertices to grid and build vertex array in vertices
    for (const auto &p : ps.polygons) {
      polygon.clear();
      for (auto v : p) {
        // align v to the grid; the CGALPoint will receive the aligned vertex
        size_t gridIndex = grid.align(v);
        Vertex_index index;
        if (gridIndex == vertices.size()) {
          index = mesh.add_vertex(vector_convert<CGAL::Point_3<K>>(v));
          vertices.push_back(index);
        }
        else {
          index = vertices[gridIndex];
        }
        polygon.push_back(index);
      }
      mesh.add_face(polygon);
    }
  } else {
    std::unordered_map<Vector3d, Vertex_index> indices;

    for (const auto &p : ps.polygons) {
      polygon.clear();
      for (auto &v : p) {
        auto size_before = indices.size();
        auto &index = indices[v];
        if (size_before != indices.size()) {
          index = mesh.add_vertex(vector_convert<CGAL::Point_3<K>>(v));
        }
        polygon.push_back(index);
      }
      mesh.add_face(polygon);
    }
  }
	return err;
}

template bool createMeshFromPolySet(const PolySet &ps, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &mesh, bool use_grid);

template <typename K>
bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh, PolySet &ps)
{
	bool err = false;
	ps.reserve(ps.numFacets() + mesh.number_of_faces());
	for (auto &f : mesh.faces()) {
		ps.append_poly();

		CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<K>>> vbegin, vend;
		for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(f), mesh); vbegin != vend;
				 ++vbegin) {
			auto &v = mesh.point(*vbegin);
			// for (auto &v : f) {
			double x = CGAL::to_double(v.x());
			double y = CGAL::to_double(v.y());
			double z = CGAL::to_double(v.z());
			ps.append_vertex(x, y, z);
		}
	}
	return err;
}

template bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &mesh, PolySet &ps);

template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>> &input,
							CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>> &output)
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
      auto &output_vertex = reindexer[input_vertex];
      if (size_before != reindexer.size()) {
        output_vertex = output.add_vertex(converter(input.point(input_vertex)));
      }
			polygon.push_back(output_vertex);
		}
		output.add_face(polygon);
	}
}

template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &input,
											 CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &output);
#if !FAST_CSG_SAME_KERNEL
template void copyMesh(const CGAL::Surface_mesh<CGAL_Point_3> &input,
											 CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &output);
template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &input,
											 CGAL::Surface_mesh<CGAL_Point_3> &output);
#endif // FAST_CSG_SAME_KERNEL

template <typename K>
void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh)
{
  CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, /* triangulate_all_faces */ true);
}

template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_Kernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> &mesh);
#if !FAST_CSG_SAME_KERNEL
template void convertNefPolyhedronToTriangleMesh(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &mesh);
#endif // FAST_CSG_SAME_KERNEL

} // namespace CGALUtils

