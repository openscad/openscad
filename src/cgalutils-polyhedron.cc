#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "polyset-utils.h"
#include "grid.h"

#include "cgal.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <boost/range/adaptor/reversed.hpp>

#undef GEN_SURFACE_DEBUG
namespace /* anonymous */ {

template <typename Polyhedron>
class CGAL_Build_PolySet : public CGAL::Modifier_base<typename Polyhedron::HalfedgeDS>
{
	typedef typename Polyhedron::HalfedgeDS HDS;
	typedef CGAL::Polyhedron_incremental_builder_3<typename Polyhedron::HalfedgeDS> CGAL_Polybuilder;
public:
	typedef typename CGAL_Polybuilder::Point_3 CGALPoint;

	const PolySet &ps;
	CGAL_Build_PolySet(const PolySet &ps) : ps(ps) { }

/*
   Using Grid here is important for performance reasons. See following model.
   If we don't grid the geometry before converting to a Nef Polyhedron, the quads
   in the cylinders to tessellated into triangles since floating point
   incertainty causes the faces to not be 100% planar. The incertainty is exaggerated
   by the transform. This wasn't a problem earlier since we used Nef for everything,
   but optimizations since then has made us keep it in floating point space longer.

   minkowski() {
   cube([200, 50, 7], center = true);
   rotate([90,0,0]) cylinder($fn = 8, h = 1, r = 8.36, center = true);
   rotate([0,90,0]) cylinder($fn = 8, h = 1, r = 8.36, center = true);
   }
 */
#if 1 // Use Grid
	void operator()(HDS &hds) {
		CGAL_Polybuilder B(hds, true);

		Grid3d<int> grid(GRID_FINE);
		std::vector<CGALPoint> vertices;
		std::vector<std::vector<size_t>> indices;

		// Align all vertices to grid and build vertex array in vertices
		for (const auto &p : ps.polygons) {
			indices.push_back(std::vector<size_t>());
			indices.back().reserve(p.size());
			for (auto v : boost::adaptors::reverse(p)) {
				// align v to the grid; the CGALPoint will receive the aligned vertex
				size_t idx = grid.align(v);
				if (idx == vertices.size()) {
					CGALPoint p(v[0], v[1], v[2]);
					vertices.push_back(p);
				}
				indices.back().push_back(idx);
			}
		}

#ifdef GEN_SURFACE_DEBUG
		printf("polyhedron(faces=[");
		int pidx = 0;
#endif
		B.begin_surface(vertices.size(), ps.polygons.size());
		for (const auto &p : vertices) {
			B.add_vertex(p);
		}
		for (auto &pindices : indices) {
#ifdef GEN_SURFACE_DEBUG
			if (pidx++ > 0) printf(",");
#endif

			// We remove duplicate indices since there is a bug in CGAL's
			// Polyhedron_incremental_builder_3::test_facet() which fails to detect this
			std::vector<size_t>::iterator last = std::unique(pindices.begin(), pindices.end());
			std::advance(last, -1);
			if (*last != pindices.front()) last++;   // In case the first & last are equal
			pindices.erase(last, pindices.end());
			if (pindices.size() >= 3 && B.test_facet(pindices.begin(), pindices.end())) {
				B.add_facet(pindices.begin(), pindices.end());
			}
#ifdef GEN_SURFACE_DEBUG
			printf("[");
			int fidx = 0;
			for (auto i : boost::adaptors::reverse(pindices)) {
				if (fidx++ > 0) printf(",");
				printf("%ld", i);
			}
			printf("]");
#endif
		}
		B.end_surface();
#ifdef GEN_SURFACE_DEBUG
		printf("],\n");
#endif
#ifdef GEN_SURFACE_DEBUG
		printf("points=[");
		for (int i = 0; i < vertices.size(); i++) {
			if (i > 0) printf(",");
			const CGALPoint &p = vertices[i];
			printf("[%g,%g,%g]", CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()));
		}
		printf("]);\n");
#endif
	}
#else // Don't use Grid
	void operator()(HDS &hds)
	{
		CGAL_Polybuilder B(hds, true);
		Reindexer<Vector3d> vertices;
		std::vector<size_t> indices(3);

		// Estimating same # of vertices as polygons (very rough)
		B.begin_surface(ps.polygons.size(), ps.polygons.size());
		int pidx = 0;
#ifdef GEN_SURFACE_DEBUG
		printf("polyhedron(faces=[");
#endif
		for (const auto &p : ps.polygons) {
#ifdef GEN_SURFACE_DEBUG
			if (pidx++ > 0) printf(",");
#endif
			indices.clear();
			for (const auto &v, boost::adaptors::reverse(p)) {
				size_t s = vertices.size();
				size_t idx = vertices.lookup(v);
				// If we added a vertex, also add it to the CGAL builder
				if (idx == s) B.add_vertex(CGALPoint(v[0], v[1], v[2]));
				indices.push_back(idx);
			}
			// We perform this test since there is a bug in CGAL's
			// Polyhedron_incremental_builder_3::test_facet() which
			// fails to detect duplicate indices
			bool err = false;
			for (std::size_t i = 0; i < indices.size(); ++i) {
				// check if vertex indices[i] is already in the sequence [0..i-1]
				for (std::size_t k = 0; k < i && !err; ++k) {
					if (indices[k] == indices[i]) {
						err = true;
						break;
					}
				}
			}
			if (!err && B.test_facet(indices.begin(), indices.end())) {
				B.add_facet(indices.begin(), indices.end());
#ifdef GEN_SURFACE_DEBUG
				printf("[");
				int fidx = 0;
				for (auto i : indices) {
					if (fidx++ > 0) printf(",");
					printf("%ld", i);
				}
				printf("]");
#endif
			}
		}
		B.end_surface();
#ifdef GEN_SURFACE_DEBUG
		printf("],\n");

		printf("points=[");
		for (int vidx = 0; vidx < vertices.size(); vidx++) {
			if (vidx > 0) printf(",");
			const Vector3d &v = vertices.getArray()[vidx];
			printf("[%g,%g,%g]", v[0], v[1], v[2]);
		}
		printf("]);\n");
#endif
	}
#endif // if 1
};

// This code is from CGAL/demo/Polyhedron/Scene_nef_polyhedron_item.cpp
// quick hacks to convert polyhedra from exact to inexact and vice-versa
template <class Polyhedron_input, class Polyhedron_output>
struct Copy_polyhedron_to : public CGAL::Modifier_base<typename Polyhedron_output::HalfedgeDS>
{
	Copy_polyhedron_to(const Polyhedron_input &in_poly) : in_poly(in_poly) {}

	void operator()(typename Polyhedron_output::HalfedgeDS &out_hds)
	{
		typedef typename Polyhedron_output::HalfedgeDS Output_HDS;

		CGAL::Polyhedron_incremental_builder_3<Output_HDS> builder(out_hds);

		typedef typename Polyhedron_input::Vertex_const_iterator Vertex_const_iterator;
		typedef typename Polyhedron_input::Facet_const_iterator Facet_const_iterator;
		typedef typename Polyhedron_input::Halfedge_around_facet_const_circulator HFCC;

		builder.begin_surface(in_poly.size_of_vertices(),
													in_poly.size_of_facets(),
													in_poly.size_of_halfedges());

		for (Vertex_const_iterator
				 vi = in_poly.vertices_begin(), end = in_poly.vertices_end();
				 vi != end; ++vi) {
			typename Polyhedron_output::Point_3 p(::CGAL::to_double(vi->point().x()),
																						::CGAL::to_double(vi->point().y()),
																						::CGAL::to_double(vi->point().z()));
			builder.add_vertex(p);
		}

		typedef CGAL::Inverse_index<Vertex_const_iterator> Index;
		Index index(in_poly.vertices_begin(), in_poly.vertices_end());

		for (Facet_const_iterator
				 fi = in_poly.facets_begin(), end = in_poly.facets_end();
				 fi != end; ++fi) {
			HFCC hc = fi->facet_begin();
			HFCC hc_end = hc;
			//     std::size_t n = circulator_size(hc);
			//     CGAL_assertion(n >= 3);
			builder.begin_facet();
			do {
				builder.add_vertex_to_facet(index[hc->vertex()]);
				++hc;
			} while (hc != hc_end);
			builder.end_facet();
		}
		builder.end_surface();
	}   // end operator()(..)
private:
	const Polyhedron_input &in_poly;
};   // end Copy_polyhedron_to<>

}

namespace CGALUtils {

template <class Polyhedron_A, class Polyhedron_B>
void copyPolyhedron(const Polyhedron_A &poly_a, Polyhedron_B &poly_b)
{
	Copy_polyhedron_to<Polyhedron_A, Polyhedron_B> modifier(poly_a);
	poly_b.delegate(modifier);
}

template void copyPolyhedron(const CGAL::Polyhedron_3<CGAL::Epick> &, CGAL_Polyhedron &);
template void copyPolyhedron(const CGAL_Polyhedron &, CGAL::Polyhedron_3<CGAL::Epick> &);

template <typename Polyhedron>
bool createPolyhedronFromPolySet(const PolySet &ps, Polyhedron &p)
{
	bool err = false;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		CGAL_Build_PolySet<Polyhedron> builder(ps);
		p.delegate(builder);
	}
	catch (const CGAL::Assertion_exception &e) {
		PRINTB("CGAL error in CGALUtils::createPolyhedronFromPolySet: %s", e.what());
		err = true;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return err;
}

template bool createPolyhedronFromPolySet(const PolySet &ps, CGAL_Polyhedron &p);
template bool createPolyhedronFromPolySet(const PolySet &ps, CGAL::Polyhedron_3<CGAL::Epick> &p);

template <typename Polyhedron>
bool createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps)
{
	bool err = false;
	typedef typename Polyhedron::Vertex Vertex;
	typedef typename Polyhedron::Vertex_const_iterator VCI;
	typedef typename Polyhedron::Facet_const_iterator FCI;
	typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	for (FCI fi = p.facets_begin(); fi != p.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		ps.append_poly();
		do {
			Vertex const &v = *((hc++)->vertex());
			double x = CGAL::to_double(v.point().x());
			double y = CGAL::to_double(v.point().y());
			double z = CGAL::to_double(v.point().z());
			ps.append_vertex(x, y, z);
		} while (hc != hc_end);
	}
	return err;
}

template bool createPolySetFromPolyhedron(const CGAL_Polyhedron &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epick> &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epeck> &p, PolySet &ps);
template bool createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Simple_cartesian<long>> &p, PolySet &ps);

class Polyhedron_writer
{
	std::ostream *out;
	bool firstv;
	std::vector<int> indices;
public:
	Polyhedron_writer() {}
	void write_header(std::ostream &stream,
										std::size_t /*vertices*/,
										std::size_t /*halfedges*/,
										std::size_t   /*facets*/
	                  /*bool normals = false*/) {
		this->out = &stream;
		*out << "polyhedron(points=[";
		firstv = true;
	}
	void write_footer() {
		*out << "]);" << std::endl;
	}
	void write_vertex(const double &x, const double &y, const double &z) {
		*out << (firstv ? "" : ",") << '[' << x << ',' << y << ',' << z << ']';
		firstv = false;
	}
	void write_facet_header() {
		*out << "], faces=[";
		firstv = true;
	}
	void write_facet_begin(std::size_t /*no*/) {
		*out << (firstv ? "" : ",") << '[';
		indices.clear();
		firstv = false;
	}
	void write_facet_vertex_index(std::size_t index) {
		indices.push_back(index);
	}
	void write_facet_end() {
		bool firsti = true;
		for (auto i : boost::adaptors::reverse(indices)) {
			*out << (firsti ? "" : ",") << i;
			firsti = false;
		}
		*out << ']';
	}
};

template <typename Polyhedron>
std::string printPolyhedron(const Polyhedron &p) {
	std::stringstream sstream;
	sstream.precision(20);

	Polyhedron_writer writer;
	generic_print_polyhedron(sstream, p, writer);

	return sstream.str();
}

template std::string printPolyhedron(const CGAL_Polyhedron &p);

}  // namespace CGALUtils

#endif /* ENABLE_CGAL */

