// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/manifold/ManifoldGeometry.h"
#include "geometry/Polygon2d.h"
#include <map>
#include <set>
#include <functional>
#include <exception>
#include <sstream>
#include <utility>
#include <cstdint>
#include <manifold/cross_section.h>
#include <manifold/manifold.h>
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/manifold/manifoldutils.h"
#include "glview/ColorMap.h"
#include "glview/RenderSettings.h"
#include <cstddef>
#include <string>
#include <memory>
#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#endif

namespace {

template <typename Result, typename V>
Result vector_convert(V const& v) {
  return Result(v[0], v[1], v[2]);
}

}

ManifoldGeometry::ManifoldGeometry() : manifold_(manifold::Manifold()) {}

ManifoldGeometry::ManifoldGeometry(
  manifold::Manifold mani,
  const std::set<uint32_t> & originalIDs,
  const std::map<uint32_t, Color4f> & originalIDToColor,
  const std::set<uint32_t> & subtractedIDs)
    : manifold_(std::move(mani)),
      originalIDs_(originalIDs),
      originalIDToColor_(originalIDToColor),
      subtractedIDs_(subtractedIDs)
{
}

std::unique_ptr<Geometry> ManifoldGeometry::copy() const
{
  return std::make_unique<ManifoldGeometry>(*this);
}

const manifold::Manifold& ManifoldGeometry::getManifold() const {
  return manifold_;
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
  return manifold_.Status() == manifold::Manifold::Error::NoError;
}

void ManifoldGeometry::clear() {
  manifold_ = manifold::Manifold();
}

size_t ManifoldGeometry::memsize() const {
  // We don't introspect on the manifold here, as this would force it to leaf node (ie. would render it).
  return 0;
}

std::string ManifoldGeometry::dump() const {
  std::ostringstream out;
  auto &manifold = getManifold();
  auto meshgl = manifold.GetMeshGL64();
  out << "Manifold:"
      << "\n status: " << ManifoldUtils::statusToString(manifold.Status())
      << "\n genus: " << manifold.Genus()
      << "\n num vertices: " << meshgl.NumVert()
      << "\n num polygons: " << meshgl.NumTri()
      << "\n polygons data:";

  for (size_t faceid = 0; faceid < meshgl.NumTri(); faceid++) {
    out << "\n  polygon begin:";
    for (const int j : {0, 1, 2}) {
      auto v = vector_convert<Vector3d>(meshgl.GetVertPos(meshgl.GetTriVerts(faceid)[j]));
      out << "\n   vertex:" << v;
    }
  }
  out << "Manifold end";
  return out.str();
}

std::shared_ptr<PolySet> ManifoldGeometry::toPolySet() const {
  manifold::MeshGL64 mesh = getManifold().GetMeshGL64();
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

  ps->colors.reserve(originalIDToColor_.size());
  ps->color_indices.reserve(ps->indices.size());

  auto colorScheme = ColorMap::inst()->findColorScheme(RenderSettings::inst()->colorscheme);
  int32_t faceFrontColorIndex = -1;
  int32_t faceBackColorIndex = -1;

  std::map<Color4f, int32_t> colorToIndex;
  std::map<uint32_t, int32_t> originalIDToColorIndex;

  auto getFaceFrontColorIndex = [&]() -> int {
    if (faceFrontColorIndex < 0) {
      faceFrontColorIndex = ps->colors.size();
      ps->colors.push_back(ColorMap::getColor(*colorScheme, RenderColor::CGAL_FACE_FRONT_COLOR));
    }
    return faceFrontColorIndex;
  };
  auto getFaceBackColorIndex = [&]() -> int {
    if (faceBackColorIndex < 0) {
      faceBackColorIndex = ps->colors.size();
      ps->colors.push_back(ColorMap::getColor(*colorScheme, RenderColor::CGAL_FACE_BACK_COLOR));
    }
    return faceBackColorIndex;
  };

  auto getColorIndex = [&](uint32_t originalID) -> int32_t {
    if (subtractedIDs_.find(originalID) != subtractedIDs_.end()) {
      return getFaceBackColorIndex();
    }
    auto colorIndexIt = originalIDToColorIndex.find(originalID);
    if (colorIndexIt != originalIDToColorIndex.end()) {
      return colorIndexIt->second;
    }
    auto colorIt = originalIDToColor_.find(originalID);
    if (colorIt == originalIDToColor_.end()) {
      return getFaceFrontColorIndex();
    }
    const auto & color = colorIt->second;

    auto pair = colorToIndex.insert({color, ps->colors.size()});
    if (pair.second) {
      ps->colors.push_back(color);
    }
    int32_t color_index = pair.first->second;
    originalIDToColorIndex[originalID] = color_index;
    return color_index;
  };

  auto start = mesh.runIndex[0];
  for (int run = 0, numRun = mesh.runIndex.size() - 1; run < numRun; ++run) {
    const auto id = mesh.runOriginalID[run];
    const auto end = mesh.runIndex[run + 1];
    const size_t numTri = (end - start) / 3;
    if (numTri == 0) {
      continue;
    }

    auto colorIndex = getColorIndex(id);
    for (int i = start; i < end; i += 3) {
      ps->indices.push_back({
          static_cast<int>(mesh.triVerts[i]),
          static_cast<int>(mesh.triVerts[i + 1]),
          static_cast<int>(mesh.triVerts[i + 2])});
      ps->color_indices.push_back(colorIndex);
    }
    start = end;
  }
  return ps;
}

#ifdef ENABLE_CGAL
template <typename Polyhedron>
class CGALPolyhedronBuilderFromManifold : public CGAL::Modifier_base<typename Polyhedron::HalfedgeDS>
{
  using HDS = typename Polyhedron::HalfedgeDS;
  using CGAL_Polybuilder = CGAL::Polyhedron_incremental_builder_3<typename Polyhedron::HalfedgeDS>;
public:
  using CGALPoint = typename CGAL_Polybuilder::Point_3;

  const manifold::MeshGL64& meshgl;
  CGALPolyhedronBuilderFromManifold(const manifold::MeshGL64& mesh) : meshgl(mesh) { }

  void operator()(HDS& hds) override {
    CGAL_Polybuilder B(hds, true);

    B.begin_surface(meshgl.NumVert(), meshgl.NumTri());
    for (size_t vertid = 0; vertid < meshgl.NumVert(); vertid++)
      B.add_vertex(CGALUtils::vector_convert<CGALPoint>(meshgl.GetVertPos(vertid)));

    for (size_t faceid = 0; faceid < meshgl.NumTri(); faceid++) {
      const auto tv = meshgl.GetTriVerts(faceid);
      B.begin_facet();
      for (const int j : {0, 1, 2}) {
        B.add_vertex_to_facet(tv[j]);
      }
      B.end_facet();
    }
    B.end_surface();
  }
};

template <class Polyhedron>
std::shared_ptr<Polyhedron> ManifoldGeometry::toPolyhedron() const
{
  auto p = std::make_shared<Polyhedron>();
  try {
    auto meshgl = getManifold().GetMeshGL64();
    CGALPolyhedronBuilderFromManifold<Polyhedron> builder(meshgl);
    p->delegate(builder);
  } catch (const CGAL::Assertion_exception& e) {
    LOG(message_group::Error, "CGAL error in CGALUtils::createPolyhedronFromPolySet: %1$s", e.what());
  }
  return p;
}

template std::shared_ptr<CGAL::Polyhedron_3<CGAL_Kernel3>> ManifoldGeometry::toPolyhedron() const;
#endif

ManifoldGeometry ManifoldGeometry::binOp(const ManifoldGeometry& lhs, const ManifoldGeometry& rhs, manifold::OpType opType) const {
  auto mani = lhs.manifold_.Boolean(rhs.manifold_, opType);
  auto originalIDToColor = lhs.originalIDToColor_;
  auto subtractedIDs = lhs.subtractedIDs_;

  auto originalIDs = lhs.originalIDs_;
  originalIDs.insert(rhs.originalIDs_.begin(), rhs.originalIDs_.end());

  if (opType == manifold::OpType::Subtract) {
    // Mark all the original ids coming from rhs as subtracted, unless they're mapped to a color.
    for (const auto id : rhs.originalIDs_) {
      auto it = rhs.originalIDToColor_.find(id);
      if (it != rhs.originalIDToColor_.end()) {
        originalIDToColor[id] = it->second;
      } else {
        subtractedIDs.insert(id);
      }
    }
  } else {
    // Add the id -> color mapping from the rhs.
    originalIDToColor.insert(rhs.originalIDToColor_.begin(), rhs.originalIDToColor_.end());
    subtractedIDs.insert(rhs.subtractedIDs_.begin(), rhs.subtractedIDs_.end());
  }
  return {mani, originalIDs, originalIDToColor, subtractedIDs};
}

std::shared_ptr<ManifoldGeometry> minkowskiOp(const ManifoldGeometry& lhs, const ManifoldGeometry& rhs) {
// FIXME: How to deal with operation not supported?
#ifdef ENABLE_CGAL
  auto lhs_nef = std::shared_ptr<CGAL_Nef_polyhedron>(CGALUtils::createNefPolyhedronFromPolySet(*lhs.toPolySet()));
  auto rhs_nef = std::shared_ptr<CGAL_Nef_polyhedron>(CGALUtils::createNefPolyhedronFromPolySet(*rhs.toPolySet()));
  if (lhs_nef->isEmpty() || rhs_nef->isEmpty()) {
    return {};
  }
  std::shared_ptr<const PolySet> ps;
  try {
    lhs_nef->minkowski(*rhs_nef);
    ps = PolySetUtils::getGeometryAsPolySet(lhs_nef);
    if (ps) {
      return ManifoldUtils::createManifoldFromPolySet(*ps);
    }
  } catch (const std::exception& e) {
    LOG(message_group::Error,
        "Nef minkoswki operation failed: %1$s\n", e.what());
  } catch (...) {
    LOG(message_group::Warning,
        "Nef minkowski hard-crashed");
  }
  return {};
#endif
}

ManifoldGeometry ManifoldGeometry::operator+(const ManifoldGeometry& other) const {
  return binOp(*this, other, manifold::OpType::Add);
}

ManifoldGeometry ManifoldGeometry::operator*(const ManifoldGeometry& other) const {
  return binOp(*this, other, manifold::OpType::Intersect);
}

ManifoldGeometry ManifoldGeometry::operator-(const ManifoldGeometry& other) const {
  return binOp(*this, other, manifold::OpType::Subtract);
}

ManifoldGeometry ManifoldGeometry::minkowski(const ManifoldGeometry& other) const {
  std::shared_ptr<ManifoldGeometry> geom = minkowskiOp(*this, other);
  if (geom) return *geom;
  else return {};
}

Polygon2d ManifoldGeometry::slice() const {
  auto cross_section = manifold::CrossSection(manifold_.Slice());
  return ManifoldUtils::polygonsToPolygon2d(cross_section.ToPolygons());
}

Polygon2d ManifoldGeometry::project() const {
  auto cross_section = manifold::CrossSection(manifold_.Project());
  return ManifoldUtils::polygonsToPolygon2d(cross_section.ToPolygons());
}

void ManifoldGeometry::transform(const Transform3d& mat) {
  manifold::mat3x4 glMat(
    // Column-major ordering
    {mat(0, 0), mat(1, 0), mat(2, 0)},
    {mat(0, 1), mat(1, 1), mat(2, 1)},
    {mat(0, 2), mat(1, 2), mat(2, 2)},
    {mat(0, 3), mat(1, 3), mat(2, 3)}
  );
  manifold_ = getManifold().Transform(glMat);
}

void ManifoldGeometry::setColor(const Color4f& c) {
  if (manifold_.OriginalID() == -1) {
    manifold_ = manifold_.AsOriginal();
  }
  originalIDs_.clear();
  originalIDs_.insert(manifold_.OriginalID());
  originalIDToColor_.clear();
  originalIDToColor_[manifold_.OriginalID()] = c;
  subtractedIDs_.clear();
}

void ManifoldGeometry::toOriginal() {
  if (manifold_.OriginalID() == -1) {
    manifold_ = manifold_.AsOriginal();
  }
  originalIDs_.clear();
  originalIDs_.insert(manifold_.OriginalID());
  originalIDToColor_.clear();
  subtractedIDs_.clear();
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
void ManifoldGeometry::foreachVertexUntilTrue(const std::function<bool(const manifold::vec3& pt)>& f) const {
  auto mesh = getManifold().GetMeshGL64();
  const auto numVert = mesh.NumVert();
  for (size_t v = 0; v < numVert; ++v) {
    if (f(mesh.GetVertPos(v))) {
      return;
    }
  }
}
