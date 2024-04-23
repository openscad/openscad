// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "printutils.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"
#include <CGAL/convex_hull_3.h>
#include <CGAL/Surface_mesh.h>
#endif
#include "PolySetUtils.h"
#include "PolySet.h"
#include "polygon.h"

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
  typedef typename TriangleMesh::Vertex_index vertex_descriptor;

  manifold::Mesh mesh;

  mesh.vertPos.resize(tm.number_of_vertices());
  for (vertex_descriptor vd : tm.vertices()){
    const auto &v = tm.point(vd);
    mesh.vertPos[vd] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }

  mesh.triVerts.reserve(tm.number_of_faces());
  for (const auto& f : tm.faces()) {
    size_t idx[3];
    size_t i = 0;
    for (vertex_descriptor vd : vertices_around_face(tm.halfedge(f), tm)) {
      if (i >= 3) break;
      idx[i++] = vd;
    }
    if (i < 3) continue;
    mesh.triVerts.emplace_back(idx[0], idx[1], idx[2]);
  }

  assert((mesh.triVerts.size() == tm.number_of_faces()) || !"Mesh was not triangular!");

  auto mani = std::make_shared<manifold::Manifold>(std::move(mesh));
  if (mani->Status() != Error::NoError) {
    LOG(message_group::Error,
        "[manifold] Surface_mesh -> Manifold conversion failed: %1$s", 
        ManifoldUtils::statusToString(mani->Status()));
  }
  return std::make_shared<ManifoldGeometry>(mani);
}

#ifdef ENABLE_CGAL
template std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>> &tm);
template std::shared_ptr<ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL_DoubleMesh &tm);
#endif

std::shared_ptr<ManifoldGeometry> createManifoldFromTriangularPolySet(const PolySet& ps)
{
  assert(ps.isTriangular());

  manifold::Mesh mesh;

  mesh.vertPos.reserve(ps.vertices.size());
  for (const auto& v : ps.vertices) {
    mesh.vertPos.emplace_back((float)v.x(), (float)v.y(), (float)v.z());
  }

  mesh.triVerts.reserve(ps.indices.size());
  for (const auto& face : ps.indices) {
    assert(face.size() == 3);
    mesh.triVerts.emplace_back(face[0], face[1], face[2]);
  }

  return std::make_shared<ManifoldGeometry>(std::make_shared<const manifold::Manifold>(std::move(mesh)));
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

  // FIXME: Should we suppress this warning, as it may not be very actionable?
  LOG(message_group::Warning,"PolySet -> Manifold conversion failed: %1$s\n"
      "Trying to repair and reconstruct mesh..",
      ManifoldUtils::statusToString(mani->getManifold().Status()));

  // 2. If the PolySet couldn't be converted into a Manifold object, let's try to repair it.
  // We currently have to utilize some CGAL functions to do this.
  {
  #ifdef ENABLE_CGAL
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

    return createManifoldFromSurfaceMesh(m);
  #else
    return std::make_shared<ManifoldGeometry>();
  #endif
  }
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
