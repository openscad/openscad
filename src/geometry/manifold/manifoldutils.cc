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

std::shared_ptr<manifold::Manifold> trustedPolySetToManifold(const PolySet& ps) {
  manifold::Mesh mesh;
  std::unique_ptr<PolySet> buffer;
  if (!ps.isTriangular)
    buffer = PolySetUtils::tessellate_faces(ps);
  const PolySet& triangulated = ps.isTriangular ? ps : *buffer;

  auto numfaces = triangulated.indices.size();
  const auto &vertices = triangulated.vertices;
  const auto &indices = triangulated.indices;

  mesh.vertPos.resize(vertices.size());
  mesh.triVerts.resize(numfaces);
  for (size_t i = 0, n = vertices.size(); i < n; i++) {
    const auto &v = vertices[i];
    mesh.vertPos[i] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }
  const auto vertexCount = mesh.vertPos.size();
//  assert(indices.size() == numfaces * 4);
  for (size_t i = 0; i < numfaces; i++) {
    unsigned int i0 = indices[i][0];
    unsigned int i1 = indices[i][1];
    unsigned int i2 = indices[i][2];
    assert(i0 >= 0 && i0 < vertexCount &&
           i1 >= 0 && i1 < vertexCount &&
           i2 >= 0 && i2 < vertexCount);
    assert(i0 != i1 && i0 != i2 && i1 != i2);
    mesh.triVerts[i] = {i0, i1, i2};
  }
  return std::make_shared<manifold::Manifold>(std::move(mesh));
}

template <class TriangleMesh>
std::shared_ptr<const ManifoldGeometry> createManifoldFromSurfaceMesh(const TriangleMesh& tm)
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
template std::shared_ptr<const ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>> &tm);
template std::shared_ptr<const ManifoldGeometry> createManifoldFromSurfaceMesh(const CGAL_DoubleMesh &tm);
#endif

std::shared_ptr<const ManifoldGeometry> createManifoldFromPolySet(const PolySet& ps)
{
#ifdef ENABLE_CGAL
  PolySet psq(ps);
  std::vector<Vector3d> points3d;
  psq.quantizeVertices(&points3d);
  auto ps_tri = PolySetUtils::tessellate_faces(psq);
  
  CGAL_DoubleMesh m;

  if (ps_tri->is_convex()) {
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

  if (!ps_tri->is_convex()) {
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

std::shared_ptr<const ManifoldGeometry> createManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom) {
  if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return mani;
  }
  if (auto ps = PolySetUtils::getGeometryAsPolySet(geom)) {
    return createManifoldFromPolySet(*ps);
  }
  return nullptr;
}

}; // namespace ManifoldUtils
