// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "manifoldutils.h"
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"
#include "printutils.h"
#include "cgalutils.h"
#include "PolySetUtils.h"
#include "CGALHybridPolyhedron.h"
#include <CGAL/convex_hull_3.h>
#include <CGAL/Surface_mesh.h>

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
  IndexedMesh im;
  {
    PolySet triangulated(3);
    PolySetUtils::tessellate_faces(ps, triangulated);
    im.append_geometry(triangulated);
  }

  auto numfaces = im.numfaces;
  const auto &vertices = im.vertices.getArray();
  const auto &indices = im.indices;

  manifold::Mesh mesh;
  mesh.vertPos.resize(vertices.size());
  mesh.triVerts.resize(numfaces);
  for (size_t i = 0, n = vertices.size(); i < n; i++) {
    const auto &v = vertices[i];
    mesh.vertPos[i] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }
  const auto vertexCount = mesh.vertPos.size();
  assert(indices.size() == numfaces * 4);
  for (size_t i = 0; i < numfaces; i++) {
    auto offset = i * 4; // 3 indices of triangle then -1.
    auto i0 = indices[offset];
    auto i1 = indices[offset + 1];
    auto i2 = indices[offset + 2];
    assert(indices[offset + 3] == -1);
    assert(i0 >= 0 && i0 < vertexCount &&
           i1 >= 0 && i1 < vertexCount &&
           i2 >= 0 && i2 < vertexCount);
    assert(i0 != i1 && i0 != i2 && i1 != i2);
    mesh.triVerts[i] = {i0, i1, i2};
  }
  return make_shared<manifold::Manifold>(std::move(mesh));
}

template <class TriangleMesh>
std::shared_ptr<ManifoldGeometry> createMutableManifoldFromSurfaceMesh(const TriangleMesh& tm)
{
  typedef typename TriangleMesh::Vertex_index vertex_descriptor;
  typedef typename TriangleMesh::Face_index face_descriptor;

  manifold::Mesh mesh;

  mesh.vertPos.resize(tm.number_of_vertices());
  for (vertex_descriptor vd : tm.vertices()){
    const auto &v = tm.point(vd);
    mesh.vertPos[vd] = glm::vec3((float) v.x(), (float) v.y(), (float) v.z());
  }

  mesh.triVerts.reserve(tm.number_of_faces());
  for (auto& f : tm.faces()) {
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

template std::shared_ptr<ManifoldGeometry> createMutableManifoldFromSurfaceMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epick>> &tm);
template std::shared_ptr<ManifoldGeometry> createMutableManifoldFromSurfaceMesh(const CGAL_DoubleMesh &tm);

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromPolySet(const PolySet& ps)
{
  PolySet psq(ps);
  std::vector<Vector3d> points3d;
  psq.quantizeVertices(&points3d);
  PolySet ps_tri(3, psq.convexValue());
  PolySetUtils::tessellate_faces(psq, ps_tri);
  
  CGAL_DoubleMesh m;

  if (ps_tri.is_convex()) {
    using K = CGAL::Epick;
    // Collect point cloud
    std::vector<K::Point_3> points(points3d.size());
    for (size_t i = 0, n = points3d.size(); i < n; i++) {
      points[i] = vector_convert<K::Point_3>(points3d[i]);
    }
    if (points.size() <= 3) return make_shared<ManifoldGeometry>();

    // Apply hull
    CGAL::Surface_mesh<CGAL::Point_3<K>> r;
    CGAL::convex_hull_3(points.begin(), points.end(), r);
    CGALUtils::copyMesh(r, m);
  } else {
    CGALUtils::createMeshFromPolySet(ps_tri, m);
  }

  if (!ps_tri.is_convex()) {
    if (CGALUtils::isClosed(m)) {
      CGALUtils::orientToBoundAVolume(m);
    } else {
      LOG(message_group::Error, "[manifold] Input mesh is not closed!");
    }
  }

  return createMutableManifoldFromSurfaceMesh(m);
}

std::shared_ptr<ManifoldGeometry> createMutableManifoldFromGeometry(const std::shared_ptr<const Geometry>& geom) {
  if (auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return std::make_shared<ManifoldGeometry>(*mani);
  }

  auto ps = CGALUtils::getGeometryAsPolySet(geom);
  if (ps) {
    return createMutableManifoldFromPolySet(*ps);
  }
  
  return nullptr;
}

}; // namespace ManifoldUtils
