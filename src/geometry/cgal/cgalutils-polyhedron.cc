#ifdef ENABLE_CGAL

#include "geometry/cgal/cgalutils.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "utils/printutils.h"
#include "geometry/Grid.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <memory>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <boost/range/adaptor/reversed.hpp>

#include <cstddef>
#include <vector>

#undef GEN_SURFACE_DEBUG
namespace /* anonymous */ {

template <typename Polyhedron>
class CGAL_Build_PolySet : public CGAL::Modifier_base<typename Polyhedron::HalfedgeDS>
{
  using HDS = typename Polyhedron::HalfedgeDS;
  using CGAL_Polybuilder = CGAL::Polyhedron_incremental_builder_3<typename Polyhedron::HalfedgeDS>;
public:
  using CGALPoint = typename CGAL_Polybuilder::Point_3;

  const PolySet& ps;
  CGAL_Build_PolySet(const PolySet& ps) : ps(ps) { }

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
  void operator()(HDS& hds) override {
    CGAL_Polybuilder B(hds, true);

    Grid3d<int> grid(GRID_FINE);
    std::vector<CGALPoint> vertices;
    std::vector<std::vector<size_t>> indices;

    // Align all vertices to grid and build vertex array in vertices
    for (const auto& p : ps.indices) {
      indices.emplace_back();
      indices.back().reserve(p.size());
      for (auto ind : boost::adaptors::reverse(p)) {
        // align v to the grid; the CGALPoint will receive the aligned vertex
	Vector3d v=ps.vertices[ind];
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
    B.begin_surface(vertices.size(), ps.indices.size());
    for (const auto& p : vertices) {
      B.add_vertex(p);
    }
    for (auto& pindices : indices) {
#ifdef GEN_SURFACE_DEBUG
      if (pidx++ > 0) printf(",");
#endif

      // We remove duplicate indices since there is a bug in CGAL's
      // Polyhedron_incremental_builder_3::test_facet() which fails to detect this
      auto last = std::unique(pindices.begin(), pindices.end());
      std::advance(last, -1);
      if (*last != pindices.front()) last++; // In case the first & last are equal
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
    for (std::size_t i = 0; i < vertices.size(); ++i) {
      if (i > 0) printf(",");
      const CGALPoint& p = vertices[i];
      printf("[%g,%g,%g]", CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()));
    }
    printf("]);\n");
#endif
  }
#else // Don't use Grid
  void operator()(HDS& hds)
  {
    CGAL_Polybuilder B(hds, true);
    Reindexer<Vector3d> vertices;
    std::vector<size_t> indices(3);

    // Estimating same # of vertices as polygons (very rough)
    B.begin_surface(ps.indices.size(), ps.indices.size());
    int pidx = 0;
#ifdef GEN_SURFACE_DEBUG
    printf("polyhedron(faces=[");
#endif
    for (const auto& p : ps.indices) {
#ifdef GEN_SURFACE_DEBUG
      if (pidx++ > 0) printf(",");
#endif
      indices.clear();
      for (const auto& v: boost::adaptors::reverse(p)) {
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
    for (std::size_t vidx = 0; vidx < vertices.size(); ++vidx) {
      if (vidx > 0) printf(",");
      const Vector3d& v = vertices.getArray()[vidx];
      printf("[%g,%g,%g]", v[0], v[1], v[2]);
    }
    printf("]);\n");
#endif
  }
#endif // if 1
};

template <class InputKernel, class OutputKernel>
struct Copy_polyhedron_to : public CGAL::Modifier_base<typename CGAL::Polyhedron_3<OutputKernel>::HalfedgeDS>
{
  using Polyhedron_output = CGAL::Polyhedron_3<OutputKernel>;
  using Polyhedron_input = CGAL::Polyhedron_3<InputKernel>;

  Copy_polyhedron_to(const Polyhedron_input& in_poly) : in_poly(in_poly) {}

  void operator()(typename Polyhedron_output::HalfedgeDS& out_hds) override
  {
    using Output_HDS = typename Polyhedron_output::HalfedgeDS;

    CGAL::Polyhedron_incremental_builder_3<Output_HDS> builder(out_hds);

    using Vertex_const_iterator = typename Polyhedron_input::Vertex_const_iterator;
    using Facet_const_iterator = typename Polyhedron_input::Facet_const_iterator;
    using HFCC = typename Polyhedron_input::Halfedge_around_facet_const_circulator;

    builder.begin_surface(in_poly.size_of_vertices(),
                          in_poly.size_of_facets(),
                          in_poly.size_of_halfedges());

    auto converter = CGALUtils::getCartesianConverter<InputKernel, OutputKernel>();
    for (Vertex_const_iterator
         vi = in_poly.vertices_begin(), end = in_poly.vertices_end();
         vi != end; ++vi) {
      typename Polyhedron_output::Point_3 p(converter(vi->point().x()),
                                            converter(vi->point().y()),
                                            converter(vi->point().z()));
      builder.add_vertex(p);
    }

    using Index = CGAL::Inverse_index<Vertex_const_iterator>;
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
  } // end operator()(..)
private:
  const Polyhedron_input& in_poly;
};   // end Copy_polyhedron_to<>

} // namespace

namespace CGALUtils {

template <class InputKernel, class OutputKernel>
void copyPolyhedron(const CGAL::Polyhedron_3<InputKernel>& poly_a, CGAL::Polyhedron_3<OutputKernel>& poly_b)
{
  // Copy is also used in "append" cases.
  poly_b.reserve(
    poly_b.size_of_vertices() + poly_a.size_of_vertices(),
    poly_b.size_of_halfedges() + poly_a.size_of_halfedges(),
    poly_b.size_of_facets() + poly_a.size_of_facets());

  Copy_polyhedron_to<InputKernel, OutputKernel> modifier(poly_a);
  poly_b.delegate(modifier);
}

template void copyPolyhedron<CGAL::Epick, CGAL_Kernel3>(const CGAL::Polyhedron_3<CGAL::Epick>&, CGAL_Polyhedron&);
template void copyPolyhedron<CGAL_Kernel3, CGAL::Epick>(const CGAL_Polyhedron&, CGAL::Polyhedron_3<CGAL::Epick>&);

template <typename K>
void convertNefToPolyhedron(
  const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Polyhedron_3<K>& polyhedron)
{
  nef.convert_to_polyhedron(polyhedron);
}

template void convertNefToPolyhedron(const CGAL_Nef_polyhedron3& nef, CGAL_Polyhedron& polyhedron);
template void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL::Polyhedron_3<CGAL_HybridKernel3>& polyhedron);

template <typename Polyhedron>
bool createPolyhedronFromPolySet(const PolySet& ps, Polyhedron& p)
{
  bool err = false;
  try {
    CGAL_Build_PolySet<Polyhedron> builder(ps);
    p.delegate(builder);
  } catch (const CGAL::Assertion_exception& e) {
    LOG(message_group::Error, "CGAL error in CGALUtils::createPolyhedronFromPolySet: %1$s", e.what());
    err = true;
  }
  return err;
}

template bool createPolyhedronFromPolySet(const PolySet& ps, CGAL_Polyhedron& p);
template bool createPolyhedronFromPolySet(const PolySet& ps, CGAL::Polyhedron_3<CGAL::Epick>& p);
template bool createPolyhedronFromPolySet(const PolySet& ps, CGAL::Polyhedron_3<CGAL::Epeck>& p);

template <typename Polyhedron>
std::unique_ptr<PolySet> createPolySetFromPolyhedron(const Polyhedron& p)
{
  using Vertex = typename Polyhedron::Vertex;
  using FCI = typename Polyhedron::Facet_const_iterator;
  using HFCC = typename Polyhedron::Halfedge_around_facet_const_circulator;

  PolySetBuilder builder(0, p.size_of_facets());

  for (FCI fi = p.facets_begin(); fi != p.facets_end(); ++fi) {
    HFCC hc = fi->facet_begin();
    HFCC hc_end = hc;
    builder.beginPolygon(fi->facet_degree());
    do {
      Vertex const& v = *((hc++)->vertex());
      double x = CGAL::to_double(v.point().x());
      double y = CGAL::to_double(v.point().y());
      double z = CGAL::to_double(v.point().z());
      builder.addVertex(Vector3d(x, y, z));
    } while (hc != hc_end);
  }
  return builder.build();
}

template std::unique_ptr<PolySet> createPolySetFromPolyhedron(const CGAL_Polyhedron& p);
template std::unique_ptr<PolySet> createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Epick>& p);
template std::unique_ptr<PolySet> createPolySetFromPolyhedron(const CGAL::Polyhedron_3<CGAL::Simple_cartesian<long>>& p);

class Polyhedron_writer
{
  std::ostream *out{nullptr};
  bool firstv{true};
  std::vector<int> indices;
public:
  Polyhedron_writer() = default;
  void write_header(std::ostream& stream,
                    std::size_t /*vertices*/,
                    std::size_t /*halfedges*/,
                    std::size_t /*facets*/
                    /*bool normals = false*/) {
    this->out = &stream;
    *out << "polyhedron(points=[";
    firstv = true;
  }
  void write_footer() {
    *out << "]);" << std::endl;
  }
  void write_vertex(const double& x, const double& y, const double& z) {
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

}  // namespace CGALUtils

#endif /* ENABLE_CGAL */
