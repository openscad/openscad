// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "PolySet.h"
#include "PolySetUtils.h"
#include "manifoldutils.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

namespace {

template <typename Result, typename V>
Result vector_convert(V const& v) {
  return Result(v[0], v[1], v[2]);
}

}

ManifoldGeometry::ManifoldGeometry() : manifold_(std::make_shared<const manifold::Manifold>()) {}

ManifoldGeometry::ManifoldGeometry(const std::shared_ptr<const manifold::Manifold>& mani) : manifold_(mani) {
  assert(manifold_);
  if (!manifold_) clear();
}

std::unique_ptr<Geometry> ManifoldGeometry::copy() const
{
  return std::make_unique<ManifoldGeometry>(*this);
}

ManifoldGeometry& ManifoldGeometry::operator=(const ManifoldGeometry& other) {
  if (this == &other) return *this;
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
  manifold_ = std::make_shared<manifold::Manifold>();
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
      << "\n status: " << ManifoldUtils::statusToString(manifold.Status())
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
  manifold::MeshGL mesh = getManifold().GetMeshGL();
  auto ps = std::make_shared<PolySet>(3);
  ps->setTriangular(true);
  ps->vertices.reserve(mesh.NumVert());
  ps->indices.reserve(mesh.NumTri());
  ps->setConvexity(convexity);

  // first 3 channels are xyz coordinate
  for (size_t i = 0; i < mesh.vertProperties.size(); i += mesh.numProp)
    ps->vertices.emplace_back(
        mesh.vertProperties[i],
        mesh.vertProperties[i + 1],
        mesh.vertProperties[i + 2]);
  for (size_t i = 0; i < mesh.triVerts.size(); i += 3)
    ps->indices.push_back({
        static_cast<int>(mesh.triVerts[i]),
        static_cast<int>(mesh.triVerts[i + 1]),
        static_cast<int>(mesh.triVerts[i + 2])});
  return ps;
}

std::shared_ptr<manifold::Manifold> binOp(const manifold::Manifold& lhs, const manifold::Manifold& rhs, manifold::OpType opType) {
  return std::make_shared<manifold::Manifold>(lhs.Boolean(rhs, opType));
}

std::shared_ptr<ManifoldGeometry> minkowskiOp(const ManifoldGeometry& lhs, const ManifoldGeometry& rhs) {
// FIXME: How to deal with operation not supported?
#ifdef ENABLE_CGAL
  auto lhs_nef = std::shared_ptr<CGAL_Nef_polyhedron>(CGALUtils::createNefPolyhedronFromPolySet(*lhs.toPolySet()));
  auto rhs_nef = std::shared_ptr<CGAL_Nef_polyhedron>(CGALUtils::createNefPolyhedronFromPolySet(*rhs.toPolySet()));
  if (lhs_nef->isEmpty() || rhs_nef->isEmpty()) {
    return {};
  }
  lhs_nef->minkowski(*rhs_nef);

  auto ps = PolySetUtils::getGeometryAsPolySet(lhs_nef);
  if (!ps) return {};
  else {
    return ManifoldUtils::createManifoldFromPolySet(*ps);
  }
#endif
}

ManifoldGeometry ManifoldGeometry::operator+(const ManifoldGeometry& other) const {
  return {binOp(*this->manifold_, *other.manifold_, manifold::OpType::Add)};
}

ManifoldGeometry ManifoldGeometry::operator*(const ManifoldGeometry& other) const {
  return {binOp(*this->manifold_, *other.manifold_, manifold::OpType::Intersect)};
}

ManifoldGeometry ManifoldGeometry::operator-(const ManifoldGeometry& other) const {
  return {binOp(*this->manifold_, *other.manifold_, manifold::OpType::Subtract)};
}

void ManifoldGeometry::transform(const Transform3d& mat) {
  glm::mat4x3 glMat(
    // Column-major ordering
    mat(0, 0), mat(1, 0), mat(2, 0),
    mat(0, 1), mat(1, 1), mat(2, 1),
    mat(0, 2), mat(1, 2), mat(2, 2),
    mat(0, 3), mat(1, 3), mat(2, 3)
  );                            
  manifold_ = std::make_shared<manifold::Manifold>(getManifold().Transform(glMat));
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
