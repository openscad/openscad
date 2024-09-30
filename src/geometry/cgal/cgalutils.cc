// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "geometry/cgal/cgalutils.h"

#include "geometry/cgal/cgal.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySetUtils.h"
#include "core/node.h"
#include "utils/degree_trig.h"

#include <cassert>
#include <set>
#include <utility>
#include <memory>
#include <CGAL/Aff_transformation_3.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>
#include <CGAL/Surface_mesh.h>

#include <CGAL/config.h>
#include <CGAL/version.h>

#include <CGAL/convex_hull_3.h>

#include "geometry/Reindexer.h"
#include "geometry/GeometryUtils.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <cstddef>
#include <map>
#include <queue>
#include <vector>

namespace CGALUtils {

std::unique_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromPolySet(const PolySet& ps)
{
  if (ps.isEmpty()) return std::make_unique<CGAL_Nef_polyhedron>();
  assert(ps.getDimension() == 3);

  // Since is_convex doesn't work well with non-planar faces,
  // we tessellate the polyset before checking.
  PolySet psq(ps);
  std::vector<Vector3d> points3d;
  psq.quantizeVertices(&points3d);
  auto ps_tri = PolySetUtils::tessellate_faces(psq);
  if (ps_tri->isConvex()) {
    using K = CGAL::Epick;
    // Collect point cloud
    std::vector<K::Point_3> points(points3d.size());
    for (size_t i = 0, n = points3d.size(); i < n; i++) {
      points[i] = vector_convert<K::Point_3>(points3d[i]);
    }

    if (points.size() <= 3) return std::make_unique<CGAL_Nef_polyhedron>();

    // Apply hull
    CGAL::Polyhedron_3<K> r;
    CGAL::convex_hull_3(points.begin(), points.end(), r);
    CGAL_Polyhedron r_exact;
    CGALUtils::copyPolyhedron(r, r_exact);
    return std::make_unique<CGAL_Nef_polyhedron>(std::make_shared<CGAL_Nef_polyhedron3>(r_exact));
  }

  std::shared_ptr<CGAL_Nef_polyhedron3> N;
  auto plane_error = false;
  try {
    CGAL_Polyhedron P;
    auto err = CGALUtils::createPolyhedronFromPolySet(psq, P);
    if (!err) {
      if (!P.is_closed()) {
        LOG(message_group::Error, "The given mesh is not closed! Unable to convert to CGAL_Nef_Polyhedron.");
      } else if (!P.is_valid(false, 0)) {
        LOG(message_group::Error, "The given mesh is invalid! Unable to convert to CGAL_Nef_Polyhedron.");
      } else {
        N = std::make_shared<CGAL_Nef_polyhedron3>(P);
      }
    }
  } catch (const CGAL::Assertion_exception& e) {
    // First two tests matches against CGAL < 4.10, the last two tests matches against CGAL >= 4.10
    if ((std::string(e.what()).find("Plane_constructor") != std::string::npos &&
         std::string(e.what()).find("has_on") != std::string::npos) ||
        std::string(e.what()).find("ss_plane.has_on(sv_prev->point())") != std::string::npos ||
        std::string(e.what()).find("ss_circle.has_on(sp)") != std::string::npos) {
      LOG("PolySet has nonplanar faces. Attempting alternate construction");
      plane_error = true;
    } else {
      LOG(message_group::Error, "CGAL error in CGAL_Nef_polyhedron3(): %1$s", e.what());
    }
  }
  if (plane_error) try {
      CGAL_Polyhedron P;
      auto err = CGALUtils::createPolyhedronFromPolySet(*ps_tri, P);
      if (!err) {
        PRINTDB("Polyhedron is closed: %d", P.is_closed());
        PRINTDB("Polyhedron is valid: %d", P.is_valid(false, 0));
      }
      if (!err) N = std::make_shared<CGAL_Nef_polyhedron3>(P);
    } catch (const CGAL::Assertion_exception& e) {
      LOG(message_group::Error, "Alternate construction failed. CGAL error in CGAL_Nef_polyhedron3(): %1$s", e.what());
    }
  return std::make_unique<CGAL_Nef_polyhedron>(N);
}

template <typename K>
CGAL::Iso_cuboid_3<K> boundingBox(const CGAL::Nef_polyhedron_3<K>& N)
{
  CGAL::Iso_cuboid_3<K> result(0, 0, 0, 0, 0, 0);
  typename CGAL::Nef_polyhedron_3<K>::Vertex_const_iterator vi;
  std::vector<typename CGAL::Point_3<K>> points;
  // can be optimized by rewriting bounding_box to accept vertices
  CGAL_forall_vertices(vi, N) points.push_back(vi->point());
  if (points.size()) result = CGAL::bounding_box(points.begin(), points.end());
  return result;
}
template CGAL_Iso_cuboid_3 boundingBox(const CGAL_Nef_polyhedron3& N);

CGAL_Iso_cuboid_3 createIsoCuboidFromBoundingBox(const BoundingBox& bbox)
{
  return {vector_convert<CGAL_Point_3>(bbox.min()),
          vector_convert<CGAL_Point_3>(bbox.max())};
}

namespace {

// lexicographic comparison
bool operator<(Vector3d const& a, Vector3d const& b) {
  for (int i = 0; i < 3; ++i) {
    if (a[i] < b[i]) return true;
    else if (a[i] == b[i]) continue;
    return false;
  }
  return false;
}
}

struct VecPairCompare {
  bool operator()(std::pair<Vector3d, Vector3d> const& a,
                  std::pair<Vector3d, Vector3d> const& b) const {
    return a.first < b.first || (!(b.first < a.first) && a.second < b.second);
  }
};


/*!
   Check if all faces of a polyset is within 0.1 degree of being convex.

   NB! This function can give false positives if the polyset contains
   non-planar faces. To be on the safe side, consider passing a tessellated polyset.
   See issue #1061.
 */
bool is_approximately_convex(const PolySet& ps) {

  const double angle_threshold = cos_degrees(.1); // .1°

  using K = CGAL::Simple_cartesian<double>;
  using Vector = K::Vector_3;
  using Point = K::Point_3;
  using Plane = K::Plane_3;

  // compute edge to face relations and plane equations
  using Edge = std::pair<Vector3d, Vector3d>;
  using Edge_to_facet_map = std::map<Edge, int, VecPairCompare>;
  Edge_to_facet_map edge_to_facet_map;
  std::vector<Plane> facet_planes;
  facet_planes.reserve(ps.indices.size());

  for (size_t i = 0; i < ps.indices.size(); ++i) {
    Plane plane;
    auto N = ps.indices[i].size();
    if (N >= 3) {
      std::vector<Point> v(N);
      for (size_t j = 0; j < N; ++j) {
        v[j] = vector_convert<Point>(ps.vertices[ps.indices[i][j]]);
        Edge edge(ps.vertices[ps.indices[i][j]], ps.vertices[ps.indices[i][(j + 1) % N]]);
        if (edge_to_facet_map.count(edge)) return false; // edge already exists: nonmanifold
        edge_to_facet_map[edge] = i;
      }
      Vector normal;
      CGAL::normal_vector_newell_3(v.begin(), v.end(), normal);
      plane = Plane(v[0], normal);
    }
    facet_planes.push_back(plane);
  }

  for (size_t i = 0; i < ps.indices.size(); ++i) {
    auto N = ps.indices[i].size();
    if (N < 3) continue;
    for (size_t j = 0; j < N; ++j) {
      Edge other_edge(ps.vertices[ps.indices[i][(j + 1) % N]], ps.vertices[ps.indices[i][j]]);
      if (edge_to_facet_map.count(other_edge) == 0) return false; //
      //Edge_to_facet_map::const_iterator it = edge_to_facet_map.find(other_edge);
      //if (it == edge_to_facet_map.end()) return false; // not a closed manifold
      //int other_facet = it->second;
      int other_facet = edge_to_facet_map[other_edge];

      auto p = vector_convert<Point>(ps.vertices[ps.indices[i][(j + 2) % N]]);

      if (facet_planes[other_facet].has_on_positive_side(p)) {
        // Check angle
        const auto& u = facet_planes[other_facet].orthogonal_vector();
        const auto& v = facet_planes[i].orthogonal_vector();

        double cos_angle = u / sqrt(u * u) * v / sqrt(v * v);
        if (cos_angle < angle_threshold) {
          return false;
        }
      }
    }
  }

  std::set<int> explored_facets;
  std::queue<int> facets_to_visit;
  facets_to_visit.push(0);
  explored_facets.insert(0);

  while (!facets_to_visit.empty()) {
    int f = facets_to_visit.front(); facets_to_visit.pop();

    for (size_t i = 0; i < ps.indices[f].size(); ++i) {
      int j = (i + 1) % ps.indices[f].size();
      auto it = edge_to_facet_map.find(Edge(ps.vertices[ps.indices[f][j]], ps.vertices[ps.indices[f][i]]));
      if (it == edge_to_facet_map.end()) return false; // Nonmanifold
      if (!explored_facets.count(it->second)) {
        explored_facets.insert(it->second);
        facets_to_visit.push(it->second);
      }
    }
  }

  // Make sure that we were able to reach all polygons during our visit
  return explored_facets.size() == ps.indices.size();
}

std::shared_ptr<const CGAL_Nef_polyhedron> getNefPolyhedronFromGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return std::shared_ptr<CGAL_Nef_polyhedron>(createNefPolyhedronFromPolySet(*ps));
  } else if (auto poly = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return createNefPolyhedronFromHybrid(*poly);
  } else if (auto poly2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    std::shared_ptr<PolySet> ps(poly2d->tessellate());
    return std::shared_ptr<CGAL_Nef_polyhedron>(createNefPolyhedronFromPolySet(*ps));
  } else if (auto nef = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return nef;
#if ENABLE_MANIFOLD
  } else if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return std::shared_ptr<CGAL_Nef_polyhedron>(createNefPolyhedronFromPolySet(*mani->toPolySet()));
#endif
  }
  return nullptr;
}

/*
   Create a PolySet from a Nef Polyhedron 3. return false on success,
   true on failure. The trick to this is that Nef Polyhedron3 faces have
   'holes' in them. . . while PolySet (and many other 3d polyhedron
   formats) do not allow for holes in their faces. The function documents
   the method used to deal with this
 */
template <typename K>
std::unique_ptr<PolySet> createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<K>& N)
{
  // 1. Build Indexed PolyMesh
  // 2. Validate mesh (manifoldness)
  // 3. Triangulate each face
  //    -> IndexedTriangleMesh
  // 4. Validate mesh (manifoldness)
  // 5. Create PolySet

  using Nef = CGAL::Nef_polyhedron_3<K>;

  // 1. Build Indexed PolyMesh
  Reindexer<Vector3f> allVertices;
  std::vector<std::vector<IndexedFace>> polygons;

  typename Nef::Halffacet_const_iterator hfaceti;
  CGAL_forall_halffacets(hfaceti, N) {
    CGAL::Plane_3<K> plane(hfaceti->plane());
    // Since we're downscaling to float, vertices might merge during this conversion.
    // To avoid passing equal vertices to the tessellator, we remove consecutively identical
    // vertices.
    polygons.emplace_back();
    auto& faces = polygons.back();
    // the 0-mark-volume is the 'empty' volume of space. skip it.
    if (!hfaceti->incident_volume()->mark()) {
      typename Nef::Halffacet_cycle_const_iterator cyclei;
      CGAL_forall_facet_cycles_of(cyclei, hfaceti) {
        typename Nef::SHalfedge_around_facet_const_circulator c1(cyclei);
        typename Nef::SHalfedge_around_facet_const_circulator c2(c1);
        faces.push_back(IndexedFace());
        auto& currface = faces.back();
        CGAL_For_all(c1, c2) {
          auto p = c1->source()->center_vertex()->point();
          // Create vertex indices and remove consecutive duplicate vertices
          auto idx = allVertices.lookup(vector_convert<Vector3f>(p));
          if (currface.empty() || idx != currface.back()) currface.push_back(idx);
        }
        if (!currface.empty() && currface.front() == currface.back()) currface.pop_back();
        if (currface.size() < 3) faces.pop_back(); // Cull empty triangles
      }
    }
    if (faces.empty()) polygons.pop_back(); // Cull empty faces
  }

  // 2. Validate mesh (manifoldness)
  auto unconnected = GeometryUtils::findUnconnectedEdges(polygons);
  if (unconnected > 0) {
    LOG(message_group::Error, "Non-manifold mesh encountered: %1$d unconnected edges", unconnected);
  }
  // 3. Triangulate each face
  const auto& verts = allVertices.getArray();
  std::vector<IndexedTriangle> allTriangles;
  for (const auto& faces : polygons) {
#if 0 // For debugging
    std::cerr << "---\n";
    for (const auto& poly : faces) {
      for (auto i : poly) {
        std::cerr << i << " ";
      }
      std::cerr << "\n";
    }
#if 0 // debug
    std::cerr.precision(20);
    for (const auto& poly : faces) {
      for (auto i : poly) {
        std::cerr << verts[i][0] << "," << verts[i][1] << "," << verts[i][2] << "\n";
      }
      std::cerr << "\n";
    }
#endif // debug
    std::cerr << "-\n";
#endif // debug
#if 0 // For debugging
    std::cerr.precision(20);
    for (size_t i = 0; i < allVertices.size(); ++i) {
      std::cerr << verts[i][0] << ", " << verts[i][1] << ", " << verts[i][2] << "\n";
    }
#endif // debug

    /* at this stage, we have a sequence of polygons. the first
       is the "outside edge' or 'body' or 'border', and the rest of the
       polygons are 'holes' within the first. there are several
       options here to get rid of the holes. we choose to go ahead
       and let the tessellater deal with the holes, and then
       just output the resulting 3d triangles*/

    // We cannot trust the plane from Nef polyhedron to be correct.
    // Passing an incorrect normal vector can cause a crash in the constrained delaunay triangulator
    // See http://cgal-discuss.949826.n4.nabble.com/Nef3-Wrong-normal-vector-reported-causes-triangulator-crash-tt4660282.html
    // CGAL::Vector_3<CGAL_Kernel3> nvec = plane.orthogonal_vector();
    // K::Vector_3 normal(CGAL::to_double(nvec.x()), CGAL::to_double(nvec.y()), CGAL::to_double(nvec.z()));
    std::vector<IndexedTriangle> triangles;
    auto err = GeometryUtils::tessellatePolygonWithHoles(verts, faces, triangles, nullptr);
    if (!err) {
      for (const auto& t : triangles) {
        assert(t[0] >= 0 && t[0] < static_cast<int>(allVertices.size()));
        assert(t[1] >= 0 && t[1] < static_cast<int>(allVertices.size()));
        assert(t[2] >= 0 && t[2] < static_cast<int>(allVertices.size()));
        allTriangles.push_back(t);
      }
    }
  }

#if 0 // For debugging
  for (const auto& t : allTriangles) {
    std::cerr << t[0] << " " << t[1] << " " << t[2] << "\n";
  }
#endif // debug
  // 4. Validate mesh (manifoldness)
  auto unconnected2 = GeometryUtils::findUnconnectedEdges(allTriangles);
  if (unconnected2 > 0) {
    LOG(message_group::Error, "Non-manifold mesh created: %1$d unconnected edges", unconnected2);
  }

  auto polyset = PolySet::createEmpty();
  polyset->vertices.reserve(verts.size());
  for (const auto& v : verts) {
    polyset->vertices.emplace_back(v.cast<double>());
  }
  polyset->indices.reserve(allTriangles.size());
  for (const auto& tri : allTriangles) {
    polyset->indices.push_back({tri[0], tri[1], tri[2]});
  }
  polyset->setTriangular(true);

#if 0 // For debugging
  std::cerr.precision(20);
  for (size_t i = 0; i < allVertices.size(); ++i) {
    std::cerr << verts[i][0] << ", " << verts[i][1] << ", " << verts[i][2] << "\n";
  }
#endif // debug

  return polyset;
}

template std::unique_ptr<PolySet> createPolySetFromNefPolyhedron3(const CGAL_Nef_polyhedron3& N);
template std::unique_ptr<PolySet> createPolySetFromNefPolyhedron3(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& N);

template <typename K>
CGAL::Aff_transformation_3<K> createAffineTransformFromMatrix(const Transform3d& matrix) {
  return CGAL::Aff_transformation_3<K>(
    matrix(0, 0), matrix(0, 1), matrix(0, 2), matrix(0, 3),
    matrix(1, 0), matrix(1, 1), matrix(1, 2), matrix(1, 3),
    matrix(2, 0), matrix(2, 1), matrix(2, 2), matrix(2, 3), matrix(3, 3));
}
template CGAL::Aff_transformation_3<CGAL_HybridKernel3> createAffineTransformFromMatrix(const Transform3d& matrix);

template <typename K>
void transform(CGAL::Nef_polyhedron_3<K>& N, const Transform3d& matrix)
{
  assert(matrix.matrix().determinant() != 0);
  N.transform(createAffineTransformFromMatrix<K>(matrix));
}

template void transform(CGAL_Nef_polyhedron3& N, const Transform3d& matrix);
template void transform(CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& N, const Transform3d& matrix);

template <typename K>
void transform(CGAL::Surface_mesh<CGAL::Point_3<K>>& mesh, const Transform3d& matrix)
{
  assert(matrix.matrix().determinant() != 0);
  auto t = createAffineTransformFromMatrix<K>(matrix);

  for (auto v : mesh.vertices()) {
    auto& pt = mesh.point(v);
    pt = t(pt);
  }
}
template void transform(CGAL_HybridMesh& mesh, const Transform3d& matrix);

template <typename K>
Transform3d computeResizeTransform(
  const CGAL::Iso_cuboid_3<K>& bb, unsigned int dimension, const Vector3d& newsize,
  const Eigen::Matrix<bool, 3, 1>& autosize)
{
  // Based on resize() in Giles Bathgate's RapCAD (but not exactly)

  // The numeric type is our kernel's field type.
  using NT = typename K::FT;

  std::vector<NT> scale, bbox_size;
  for (unsigned int i = 0; i < 3; ++i) {
    scale.push_back(NT(1));
    bbox_size.push_back(bb.max_coord(i) - bb.min_coord(i));
  }
  int newsizemax_index = 0;
  for (unsigned int i = 0; i < dimension; ++i) {
    if (newsize[i]) {
      if (bbox_size[i] == NT(0)) {
        LOG(message_group::Warning, "Resize in direction normal to flat object is not implemented");
        return Transform3d::Identity();
      } else {
        scale[i] = NT(newsize[i]) / bbox_size[i];
      }
      if (newsize[i] > newsize[newsizemax_index]) newsizemax_index = i;
    }
  }

  auto autoscale = NT(1);
  if (newsize[newsizemax_index] != 0) {
    autoscale = NT(newsize[newsizemax_index]) / bbox_size[newsizemax_index];
  }
  for (unsigned int i = 0; i < dimension; ++i) {
    if (autosize[i] && newsize[i] == 0) scale[i] = autoscale;
  }

  Eigen::Matrix4d t;
  t << CGAL::to_double(scale[0]),           0,        0,        0,
    0,        CGAL::to_double(scale[1]),           0,        0,
    0,        0,        CGAL::to_double(scale[2]),           0,
    0,        0,        0,                                   1;

  return Transform3d(t);
}

template Transform3d computeResizeTransform(
  const CGAL_Iso_cuboid_3& bb, unsigned int dimension, const Vector3d& newsize,
  const Eigen::Matrix<bool, 3, 1>& autosize);
template Transform3d computeResizeTransform(
  const CGAL::Iso_cuboid_3<CGAL_HybridKernel3>& bb, unsigned int dimension, const Vector3d& newsize,
  const Eigen::Matrix<bool, 3, 1>& autosize);

std::shared_ptr<const PolySet> getGeometryAsPolySet(const std::shared_ptr<const Geometry>& geom)
{
  if (auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return ps;
  }
  if (auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    auto ps = std::make_shared<PolySet>(3);
    if (!N->isEmpty()) {
      if (auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*N->p3)) {
        ps->setConvexity(N->getConvexity());
        return ps;
      }
      LOG(message_group::Error, "Nef->PolySet failed.");
    }
    return std::make_shared<PolySet>(3);
  }
  if (auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return hybrid->toPolySet();
  }
#ifdef ENABLE_MANIFOLD
  if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return mani->toPolySet();
  }
#endif
  return nullptr;
}

}  // namespace CGALUtils

#endif /* ENABLE_CGAL */
