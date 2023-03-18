// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "IndexedMesh.h"
#include "cgalutils.h"

ManifoldGeometry::ManifoldGeometry() : manifold_(make_shared<manifold::Manifold>()) {}

ManifoldGeometry::ManifoldGeometry(const shared_ptr<manifold::Manifold>& mani) : manifold_(mani) {
  assert(manifold_);
  if (!manifold_) clear();
}

ManifoldGeometry::ManifoldGeometry(const ManifoldGeometry& other) : manifold_(other.manifold_) {}

ManifoldGeometry& ManifoldGeometry::operator=(const ManifoldGeometry& other) {
  manifold_ = other.manifold_;
  return *this;
}

const manifold::Manifold& ManifoldGeometry::getManifold() const {
  assert(manifold_);
  return *manifold_;
}

bool ManifoldGeometry::isEmpty() const {
  return getManifold().IsEmpty();
}

size_t ManifoldGeometry::numFacets() const {
  return getManifold().NumTri();
}

size_t ManifoldGeometry::numVertices() const {
  return getManifold().NumVert();
}

bool ManifoldGeometry::isManifold() const {
  return getManifold().Status() == manifold::Manifold::Error::NoError;
}

bool ManifoldGeometry::isValid() const {
  return manifold_->Status() == manifold::Manifold::Error::NoError;
}

void ManifoldGeometry::clear() {
  manifold_ = make_shared<manifold::Manifold>();
}

size_t ManifoldGeometry::memsize() const {
  // We don't introspect on the manifold here, as this would force it to leaf node (ie. would render it).
  return 0;
}

std::string ManifoldGeometry::dump() const {
  std::ostringstream out;
  auto &manifold = getManifold();
  auto mesh = manifold.GetMesh();
  out << "Manifold:"
      << "\n genus: " << manifold.Genus()
      << "\n num vertices: " << mesh.vertPos.size()
      << "\n num polygons: " << mesh.triVerts.size()
      << "\n polygons data:";

  for (const auto &tv : mesh.triVerts) {
    out << "\n  polygon begin:";
    for (const int j : {0, 1, 2}) {
      Vector3d v = vector_convert<Vector3d>(mesh.vertPos[tv[j]]);
      out << "\n   vertex:" << v;
    }
  }
  out << "Manifold end";
  return out.str();
}

std::shared_ptr<const PolySet> ManifoldGeometry::toPolySet() const {
  auto ps = std::make_shared<PolySet>(3);
  manifold::Mesh mesh = getManifold().GetMesh();
  ps->reserve(mesh.triVerts.size());
  Polygon poly(3);
  for (const auto &tv : mesh.triVerts) {
    for (const int j : {0, 1, 2}) {
      poly[j] = vector_convert<Vector3d>(mesh.vertPos[tv[j]]);
    }
    ps->append_poly(poly);
  }
  return ps;
}

void ManifoldGeometry::transform(const Transform3d& mat) {
  glm::mat4x3 glMat(
    // Column-major ordering
    mat(0, 0), mat(1, 0), mat(2, 0),
    mat(0, 1), mat(1, 1), mat(2, 1),
    mat(0, 2), mat(1, 2), mat(2, 2),
    mat(0, 3), mat(1, 3), mat(2, 3)
  );                            
  manifold_ = make_shared<manifold::Manifold>(std::move(getManifold().Transform(glMat)));
}

BoundingBox ManifoldGeometry::getBoundingBox() const
{
  BoundingBox result;
  manifold::Box bbox = getManifold().BoundingBox();
  result.extend(vector_convert<Eigen::Vector3d>(bbox.min));
  result.extend(vector_convert<Eigen::Vector3d>(bbox.max));
  return result;
}

void ManifoldGeometry::resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) {
  transform(GeometryUtils::getResizeTransform(this->getBoundingBox(), newsize, autosize));
}

/*! Iterate over all vertices' points until the function returns true (for done). */
void ManifoldGeometry::foreachVertexUntilTrue(const std::function<bool(const glm::vec3& pt)>& f) const {
  auto mesh = getManifold().GetMesh();
  for (const auto &pt : mesh.vertPos) {
    if (f(pt)) {
      return;
    }
  }
}
