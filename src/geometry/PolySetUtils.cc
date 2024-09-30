#include "geometry/PolySetUtils.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <cstddef>
#include <sstream>
#include <vector>

#include <boost/range/adaptor/reversed.hpp>

#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/Polygon2d.h"
#include "utils/printutils.h"
#include "geometry/GeometryUtils.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

namespace PolySetUtils {

// Project all polygons (also back-facing) into a Polygon2d instance.
// It is important to select all faces, since filtering by normal vector here
// will trigger floating point incertainties and cause problems later.
std::unique_ptr<Polygon2d> project(const PolySet& ps) {
  auto poly = std::make_unique<Polygon2d>();

  Vector3d pt;
  for (const auto& p : ps.indices) {
    Outline2d outline;
    for (const auto& v : p) {
      pt=ps.vertices[v];
      outline.vertices.emplace_back(pt[0], pt[1]);
    }
    poly->addOutline(outline);
  }
  return poly;
}

/* Tessellation of 3d PolySet faces

   This code is for tessellating the faces of a 3d PolySet, assuming that
   the faces are near-planar polygons.

   The purpose of this code is originally to fix github issue 349. Our CGAL
   kernel does not accept polygons for Nef_Polyhedron_3 if each of the
   points is not exactly coplanar. "Near-planar" or "Almost planar" polygons
   often occur due to rounding issues on, for example, polyhedron() input.
   By tessellating the 3d polygon into individual smaller tiles that
   are perfectly coplanar (triangles, for example), we can get CGAL to accept
   the polyhedron() input.
 */
/* Given a 3D PolySet with near planar polygonal faces, tessellate the
   faces. As of writing, our only tessellation method is triangulation
   using CGAL's Constrained Delaunay algorithm. This code assumes the input
   polyset has simple polygon faces with no holes.
   The tessellation will be robust wrt. degenerate and self-intersecting
 */
std::unique_ptr<PolySet> tessellate_faces(const PolySet& polyset)
{
  int degeneratePolygons = 0;
  auto result = std::make_unique<PolySet>(3, polyset.convexValue());
  result->setConvexity(polyset.getConvexity());
  result->setTriangular(true);
  // ideally this should not require a copy...
  if (polyset.isTriangular()) {
    result->vertices = polyset.vertices;
    result->indices = polyset.indices;
    result->color_indices = polyset.color_indices;
    result->colors = polyset.colors;
    return result;
  }
  result->vertices.reserve(polyset.vertices.size());
  result->indices.reserve(polyset.indices.size());

  std::vector<bool> used(polyset.vertices.size(), false);
  // best estimate without iterating all polygons, to reduce reallocations
  std::vector<IndexedFace> polygons;
  polygons.reserve(polyset.indices.size());
  std::vector<int32_t> polygon_color_indices;
  auto has_colors = !polyset.color_indices.empty();
  if (has_colors) {
    assert(polyset.color_indices.size() == polyset.indices.size());
    polygon_color_indices.reserve(polyset.color_indices.size());
    result->colors = polyset.colors;
  }
  for (size_t i = 0, n = polyset.indices.size(); i < n; i++) {
    const auto& pgon = polyset.indices[i];
    if (pgon.size() < 3) {
      degeneratePolygons++;
      continue;
    }
    auto& currface = polygons.emplace_back();
    for (const auto& ind : pgon) {
      const Vector3f v = polyset.vertices[ind].cast<float>();
      if (currface.empty() || v != polyset.vertices[currface.back()].cast<float>())
        currface.push_back(ind);
    }
    const Vector3f head = polyset.vertices[currface.front()].cast<float>();
    while (!currface.empty() && head == polyset.vertices[currface.back()].cast<float>())
      currface.pop_back();
    if (currface.size() < 3) {
      polygons.pop_back();
      continue;
    }
    if (has_colors) {
      polygon_color_indices.push_back(polyset.color_indices[i]);
    }
    for (const auto& ind : currface)
      used[ind] = true;
  }
  // remove unreferenced vertices
  std::vector<Vector3f> verts;
  std::vector<int> indexMap(polyset.vertices.size());
  verts.reserve(polyset.vertices.size());
  for (size_t i = 0; i < polyset.vertices.size(); ++i) {
    if (used[i]) {
      indexMap[i] = verts.size();
      verts.emplace_back(polyset.vertices[i].cast<float>());
      result->vertices.push_back(polyset.vertices[i]);
    }
  }
  if (verts.size() != polyset.vertices.size()) {
    // only remap indices when some vertices are really removed
    for (auto& face : polygons) {
      for (auto& ind : face)
        ind = indexMap[ind];
    }
  }

  // we will reuse this memory instead of reallocating for each polygon
  std::vector<IndexedTriangle> triangles;
  std::vector<IndexedFace> facesBuffer(1);
  for (size_t i = 0, n = polygons.size(); i < n; i++) {
    const auto& face = polygons[i];
    if (face.size() == 3) {
      // trivial case - triangles cannot be concave or have holes
       result->indices.push_back({face[0],face[1],face[2]});
       if (has_colors)
         result->color_indices.push_back(polygon_color_indices[i]);
    }
    // Quads seem trivial, but can be concave, and can have degenerate cases.
    // So everything more complex than triangles goes into the general case.
    else {
      triangles.clear();
      facesBuffer[0] = face;
      auto err = GeometryUtils::tessellatePolygonWithHoles(verts, facesBuffer, triangles, nullptr);
      if (!err) {
        for (const auto& t : triangles) {
          result->indices.push_back({t[0],t[1],t[2]});
          if (has_colors)
            result->color_indices.push_back(polygon_color_indices[i]);
        }
      }
    }
  }
  if (degeneratePolygons > 0) {
    LOG(message_group::Warning, "PolySet has degenerate polygons");
  }
  return result;
}

bool is_approximately_convex(const PolySet& ps) {
#ifdef ENABLE_CGAL
  return CGALUtils::is_approximately_convex(ps);
#else
  return false;
#endif
}

// Get as or convert the geometry to a PolySet.
std::shared_ptr<const PolySet> getGeometryAsPolySet(const std::shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    PolySetBuilder builder;
    builder.appendGeometry(geom);
    return builder.build();
  } else if (auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return ps;
  }
#ifdef ENABLE_CGAL
  if (auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    if (!N->isEmpty()) {
      if (auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*N->p3)) {
        ps->setConvexity(N->getConvexity());
        return ps;
      }
      LOG(message_group::Error, "Nef->PolySet failed.");
    }
    return PolySet::createEmpty();
  }
  if (auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return hybrid->toPolySet();
  }
#endif
#ifdef ENABLE_MANIFOLD
  if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return mani->toPolySet();
  }
#endif
  return nullptr;
}


std::string polySetToPolyhedronSource(const PolySet& ps)
{
  std::stringstream sstr;
  sstr << "polyhedron(\n";
  sstr << "  points=[\n";
  for (const auto& v : ps.vertices) {
    sstr << "[" << v[0] << ", " << v[1] << ", " << v[2] << "],\n";
  }
  sstr << "  ],\n";
  sstr << "  faces=[\n";
  for (const auto& polygon : ps.indices) {
    sstr << "[";
    for (const auto idx : boost::adaptors::reverse(polygon)) {
      sstr << idx << ",";
    }
    sstr << "],\n";
  }
  sstr << "  ],\n";
  sstr << ");\n";
  return sstr.str();
}

} // namespace PolySetUtils
