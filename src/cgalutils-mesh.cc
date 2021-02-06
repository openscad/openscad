#include "cgalutils.h"
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include "grid.h"

namespace CGALUtils {

template <typename K>
bool createMeshFromPolySet(const PolySet &ps, CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh)
{
	bool err = false;
	auto num_vertices = ps.numFacets() * 3;
	auto num_facets = ps.numFacets();
	auto num_edges = num_vertices + num_facets + 2; // Euler's formula.
	mesh.reserve(mesh.number_of_vertices() + num_vertices, mesh.number_of_halfedges() + num_edges,
							 mesh.number_of_faces() + num_facets);

	typedef typename CGAL::Surface_mesh<CGAL::Point_3<K>>::Vertex_index Vertex_index;
	std::vector<Vertex_index> polygon;
#if 1 // Use grid
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
				index = mesh.add_vertex(CGAL::Point_3<K>(v[0], v[1], v[2]));
				vertices.push_back(index);
			}
			else {
				index = vertices[gridIndex];
			}
			polygon.push_back(index);
		}
		mesh.add_face(polygon);
	}
#else
	for (const auto &p : ps.polygons) {
		polygon.clear();
		for (auto &v : p) {
			polygon.push_back(mesh.add_vertex(CGAL::Point_3<K>(v[0], v[1], v[2])));
		}
		mesh.add_face(polygon);
	}
#endif
	return err;
}

template bool createMeshFromPolySet(const PolySet &ps,
																		CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &mesh);

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

template bool createPolySetFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &mesh,
																		PolySet &ps);

template <class InputKernel, class OutputKernel>
void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<InputKernel>> &input,
							CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>> &output)
{
	auto converter = getCartesianConverter<InputKernel, OutputKernel>();
	output.reserve(output.number_of_vertices() + input.number_of_vertices(),
								 output.number_of_halfedges() + input.number_of_halfedges(),
								 output.number_of_faces() + input.number_of_faces());

	std::vector<typename CGAL::Surface_mesh<CGAL::Point_3<OutputKernel>>::Vertex_index> polygon;
	for (auto face : input.faces()) {
		polygon.clear();

		CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<CGAL::Point_3<InputKernel>>>
				vbegin, vend;
		for (boost::tie(vbegin, vend) = vertices_around_face(input.halfedge(face), input);
				 vbegin != vend; ++vbegin) {
			auto &v = input.point(*vbegin);
			polygon.push_back(output.add_vertex(converter(v)));
		}
		output.add_face(polygon);
	}
}

template void copyMesh(const CGAL::Surface_mesh<CGAL_Point_3> &input,
											 CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &output);
template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &input,
											 CGAL::Surface_mesh<CGAL_Point_3> &output);
template void copyMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &input,
											 CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &output);
} // namespace CGALUtils

