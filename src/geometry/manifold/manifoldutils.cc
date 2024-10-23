// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/manifold/manifoldutils.h"
#include "geometry/manifold/ManifoldGeometry.h"
#include "geometry/PolySetBuilder.h"
#include "Feature.h"
#include "utils/printutils.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#include <optional>
#include <cassert>
#include <map>
#include <set>
#include <exception>
#include <utility>
#include <cstdint>
#include <memory>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Surface_mesh.h>
#endif
#include "geometry/PolySetUtils.h"
#include "geometry/PolySet.h"
#include <manifold/polygon.h>

#include <cstddef>
#include <vector>

using Error = manifold::Manifold::Error;

namespace ManifoldUtils {

const char* statusToString(Error status) {
  switch (status) {
    case Error::NoError: return "NoError";
    case Error::NonFiniteVertex: return "NonFiniteVertex";
    case Error::NotManifold: return "NotManifold";
    case Error::VertexOutOfBounds: return "VertexOutOfBounds";
    case Error::PropertiesWrongLength: return "PropertiesWrongLength";
    case Error::MissingPositionProperties: return "MissingPositionProperties";
    case Error::MergeVectorsDifferentLengths: return "MergeVectorsDifferentLengths";
    case Error::MergeIndexOutOfBounds: return "MergeIndexOutOfBounds";
    case Error::TransformWrongLength: return "TransformWrongLength";
    case Error::RunIndexWrongLength: return "RunIndexWrongLength";
    case Error::FaceIDWrongLength: return "FaceIDWrongLength";
    default: return "unknown";
  }
}

template <class TriangleMesh>
std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const TriangleMesh& tm)
{
  using vertex_descriptor = typename TriangleMesh::Vertex_index;

  manifold::MeshGL64 meshgl;

  meshgl.numProp = 3;
  meshgl.vertProperties.resize(tm.number_of_vertices() * 3);
  for (vertex_descriptor vd : tm.vertices()){
    const auto &v = tm.point(vd);
    meshgl.vertProperties[3 * vd] = v.x();
    meshgl.vertProperties[3 * vd + 1] = v.y();
    meshgl.vertProperties[3 * vd + 2] = v.z();
  }

  meshgl.triVerts.reserve(tm.number_of_faces() * 3);
  for (const auto& f : tm.faces()) {
    size_t idx[3];
    size_t i = 0;
    for (vertex_descriptor vd : vertices_around_face(tm.halfedge(f), tm)) {
      if (i >= 3) break;
      idx[i++] = vd;
    }
    if (i < 3) continue;
    for (size_t j : {0, 1, 2})
      meshgl.triVerts.emplace_back(idx[j]);
  }

  assert((meshgl.triVerts.size() == tm.number_of_faces() * 3) || !"Mesh was not triangular!");

  auto mani = manifold::Manifold(meshgl).AsOriginal();
  if (mani.Status() != Error::NoError) {
    LOG(message_group::Error,
        "[manifold] Surface_mesh -> Manifold conversion failed: %1$s",
        ManifoldUtils::statusToString(mani.Status()));
    return nullptr;
  }
  std::set<uint32_t> originalIDs;
  auto id = mani.OriginalID();
  if (id >= 0) {
    originalIDs.insert(id);
  }
  return std::make_shared<ManifoldGeometry>(mani, originalIDs);
}

#ifdef ENABLE_CGAL
template std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>> &tm);
template std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL_DoubleMesh &tm);
#endif

std::shared_ptr<ManifoldGeometry> createManifoldFromTriangularPolySet(const PolySet& ps)
{
  assert(ps.isTriangular());

  manifold::MeshGL64 mesh;

  mesh.numProp = 3;
  mesh.vertProperties.reserve(ps.vertices.size() * 3);
  for (const auto& v : ps.vertices) {
    mesh.vertProperties.push_back(v.x());
    mesh.vertProperties.push_back(v.y());
    mesh.vertProperties.push_back(v.z());
  }

  mesh.triVerts.reserve(ps.indices.size() * 3);

  std::set<uint32_t> originalIDs;
  std::map<uint32_t, Color4f> originalIDToColor;

  std::map<std::optional<Color4f>, std::vector<size_t>> colorToFaceIndices;
  for (size_t i = 0, n = ps.indices.size(); i < n; i++) {
    auto color_index = i < ps.color_indices.size() ? ps.color_indices[i] : -1;
    std::optional<Color4f> color;
    if (color_index >= 0) {
      color = ps.colors[color_index];
    }
    colorToFaceIndices[color].push_back(i);
  }
  auto next_id = manifold::Manifold::ReserveIDs(colorToFaceIndices.size());
  for (const auto& [color, faceIndices] : colorToFaceIndices) {

    auto id = next_id++;
    if (color.has_value()) {
      originalIDToColor[id] = color.value();
    }

    mesh.runIndex.push_back(mesh.triVerts.size());
    mesh.runOriginalID.push_back(id);
    originalIDs.insert(id);

    for (size_t faceIndex : faceIndices) {
      auto & face = ps.indices[faceIndex];
      assert(face.size() == 3);
      mesh.triVerts.push_back(face[0]);
      mesh.triVerts.push_back(face[1]);
      mesh.triVerts.push_back(face[2]);
    }
  }
  mesh.runIndex.push_back(mesh.triVerts.size());

  auto mani = manifold::Manifold(mesh);
  return std::make_shared<ManifoldGeometry>(mani, originalIDs, originalIDToColor);
}

std::shared_ptr<ManifoldGeometry> createManifoldFromPolySet(const PolySet& ps)
{
  // 1. If the PolySet is already manifold, we should be able to build a Manifold object directly
  // (through using manifold::Mesh).
  // We need to make sure our PolySet is triangulated before doing that.
  // Note: We currently don't have a way of directly checking if a PolySet is manifold,
  // so we just try converting to a Manifold object and check its status.
  std::unique_ptr<const PolySet> triangulated;
  if (!ps.isTriangular()) {
    triangulated = PolySetUtils::tessellate_faces(ps);
  }
  const PolySet& triangle_set = ps.isTriangular() ? ps : *triangulated;

  auto mani = createManifoldFromTriangularPolySet(triangle_set);
  if (mani->getManifold().Status() == Error::NoError) {
    return mani;
  }

  // Before announcing that the conversion failed, let's try to fix the most common
  // causes of a non-manifold topology:
  // Polygon soup of manifold topology with co-incident vertices having identical vertex positions
  //
  // Note: This causes us to lose the ability to represent manifold topologies with duplicate
  // vertex positions (touching cubes, donut with vertex in the center etc.)
  PolySetBuilder builder(ps.vertices.size(), ps.indices.size(),
                         ps.getDimension(), ps.convexValue());
  builder.appendPolySet(triangle_set);
  const std::unique_ptr<PolySet> rebuilt_ps = builder.build();
  mani = createManifoldFromTriangularPolySet(*rebuilt_ps);
  if (mani->getManifold().Status() == Error::NoError) {
    return mani;
  }

  // FIXME: Should we attempt merging vertices within epsilon distance before issuing this warning?
  LOG(message_group::Warning,"PolySet -> Manifold conversion failed: %1$s\n"
      "Trying to repair and reconstruct mesh..",
      ManifoldUtils::statusToString(mani->getManifold().Status()));

  // 2. If the PolySet couldn't be converted into a Manifold object, let's try to repair it.
  // We currently have to utilize some CGAL functions to do this.
#ifdef ENABLE_CGAL
  try {
    PolySet psq(ps);
    std::vector<Vector3d> points3d;
    psq.quantizeVertices(&points3d);
    auto ps_tri = PolySetUtils::tessellate_faces(psq);

    CGAL_DoubleMesh m;

    if (ps_tri->isConvex()) {
      using K = CGAL::Epick;
      // Collect point cloud
      std::vector<K::Point_3> points(points3d.size());
      for (size_t i = 0, n = points3d.size(); i < n; i++) {
        points[i] = CGALUtils::vector_convert<K::Point_3>(points3d[i]);
      }
      if (points.size() <= 3) return std::make_shared<ManifoldGeometry>();

      // Apply hull
      CGAL::Surface_mesh<CGAL::Point_3<K>> r;
      CGAL::convex_hull_3(points.begin(), points.end(), r);
      CGALUtils::copyMesh(r, m);
    } else {
      CGALUtils::createMeshFromPolySet(*ps_tri, m);
    }

    if (!ps_tri->isConvex()) {
      if (CGALUtils::isClosed(m)) {
        CGALUtils::orientToBoundAVolume(m);
      } else {
        LOG(message_group::Error, "[manifold] Input mesh is not closed!");
      }
    }

    auto geom = createManifoldFromSurfaceMesh(m);
    // TODO: preserve color if polyset is fully monochrome, or maybe pass colors around in surface mesh?
    return geom;
  } catch (const std::exception& e) {
    LOG(message_group::Error, "[manifold] CGAL error: %1$s", e.what());
  }
#endif
  return std::make_shared<ManifoldGeometry>();
}

std::shared_ptr<const ManifoldGeometry> createManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom) {
  if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return mani;
  }
  if (auto ps = PolySetUtils::getGeometryAsPolySet(geom)) {
    return createManifoldFromPolySet(*ps);
  }
  return nullptr;
}

Polygon2d polygonsToPolygon2d(const manifold::Polygons& polygons) {
  Polygon2d poly2d;
  for (const auto& polygon : polygons) {
    Outline2d outline;
    for (const auto& v : polygon) {
      outline.vertices.emplace_back(v[0], v[1]);
    }
    poly2d.addOutline(std::move(outline));
  }
  return std::move(poly2d);
}

std::unique_ptr<PolySet> createTriangulatedPolySetFromPolygon2d(const Polygon2d& polygon2d)
{
  auto polyset = std::make_unique<PolySet>(2);
  polyset->setTriangular(true);

  manifold::Polygons polygons;
  for (const auto& outline : polygon2d.outlines()) {
    manifold::SimplePolygon simplePolygon;
    for (const auto& vertex : outline.vertices) {
      polyset->vertices.emplace_back(vertex[0], vertex[1], 0.0);
      simplePolygon.emplace_back(vertex[0], vertex[1]);
    }
    polygons.push_back(std::move(simplePolygon));
  }

  const auto triangles = manifold::Triangulate(polygons);

  for (const auto& triangle : triangles) {
    polyset->indices.push_back({triangle[0], triangle[1], triangle[2]});
  }
  return polyset;

}

}; // namespace ManifoldUtils
