// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/CGALHybridPolyhedron.h"

#include "geometry/cgal/cgalutils.h"
#include "Feature.h"
#include <cassert>
#include <map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/helpers.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <variant>

CGALHybridPolyhedron::CGALHybridPolyhedron(const std::shared_ptr<CGAL_HybridNef>& nef)
{
  assert(nef);
  data = nef;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const std::shared_ptr<CGAL_HybridMesh>& mesh)
{
  assert(mesh);
  data = mesh;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const CGALHybridPolyhedron& other) : Geometry(other)
{
  *this = other;
}

std::unique_ptr<Geometry> CGALHybridPolyhedron::copy() const
{
  return std::make_unique<CGALHybridPolyhedron>(*this);
}

CGALHybridPolyhedron& CGALHybridPolyhedron::operator=(const CGALHybridPolyhedron& other)
{
  Geometry::operator=(other);
  if (auto nef = other.getNefPolyhedron()) {
    data = std::make_shared<CGAL_HybridNef>(*nef);
  } else if (auto mesh = other.getMesh()) {
    data = std::make_shared<CGAL_HybridMesh>(*mesh);
  } else {
    assert(!"Bad hybrid polyhedron state");
  }
  return *this;
}

CGALHybridPolyhedron::CGALHybridPolyhedron() : Geometry()
{
  data = std::make_shared<CGAL_HybridMesh>();
}

std::shared_ptr<CGAL_HybridNef> CGALHybridPolyhedron::getNefPolyhedron() const
{
  return std::holds_alternative<std::shared_ptr<CGAL_HybridNef>>(data) ?
         std::get<std::shared_ptr<CGAL_HybridNef>>(data) :
         nullptr;
}

std::shared_ptr<CGAL_HybridMesh> CGALHybridPolyhedron::getMesh() const
{
  return std::holds_alternative<std::shared_ptr<CGAL_HybridMesh>>(data) ?
         std::get<std::shared_ptr<CGAL_HybridMesh>>(data) :
         nullptr;
}

bool CGALHybridPolyhedron::isEmpty() const
{
  return numFacets() == 0;
}

size_t CGALHybridPolyhedron::numFacets() const
{
  if (auto nef = getNefPolyhedron()) {
    return nef->number_of_facets();
  } else if (auto mesh = getMesh()) {
    return mesh->number_of_faces();
  }
  assert(!"Bad hybrid polyhedron state");
  return 0;
}

size_t CGALHybridPolyhedron::numVertices() const
{
  if (auto nef = getNefPolyhedron()) {
    return nef->number_of_vertices();
  } else if (auto mesh = getMesh()) {
    return mesh->number_of_vertices();
  }
  assert(!"Bad hybrid polyhedron state");
  return 0;
}

bool CGALHybridPolyhedron::isManifold() const
{
  if (auto mesh = getMesh()) {
    // Note: haven't tried mesh->is_valid() but it could be too expensive.
    // TODO: use is_valid_polygon_mesh and remember
    auto isValid = CGAL::is_valid_polygon_mesh(*mesh);
    return isValid;
  } else if (auto nef = getNefPolyhedron()) {
    return nef->is_simple();
  }
  assert(!"Bad hybrid polyhedron state");
  return false;
}

bool CGALHybridPolyhedron::isValid() const
{
  if (auto mesh = getMesh()) {
    return CGAL::is_valid_polygon_mesh(*mesh);
  } else if (auto nef = getNefPolyhedron()) {
    return nef->is_valid();
  }
  assert(!"Bad hybrid polyhedron state");
  return false;
}

std::shared_ptr<const PolySet> CGALHybridPolyhedron::toPolySet() const
{
  if (auto mesh = getMesh()) {
    auto ps = CGALUtils::createPolySetFromMesh(*mesh);
    assert(ps && "Error from CGALUtils::createPolySetFromMesh");
    ps->setConvexity(convexity);
    return ps;
  } else if (auto nef = getNefPolyhedron()) {
    auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*nef);
    assert(ps && "Error from CGALUtils::createPolySetFromNefPolyhedron3");
    ps->setConvexity(convexity);
    return ps;
  } else {
    assert(!"Bad hybrid polyhedron state");
    return nullptr;
  }
}

void CGALHybridPolyhedron::clear()
{
  data = std::make_shared<CGAL_HybridMesh>();
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh union", other, [&](CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out) {
      return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef union", other,
               [&](CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef) {
    CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh intersection", other,
                  [&](CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out) {
      return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef intersection", other,
               [&](CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef) {
    CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh difference", other,
                  [&](CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out) {
      return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef difference", other,
               [&](CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef) {
    CGALUtils::inPlaceNefDifference(destinationNef, otherNef);
  });
}

bool CGALHybridPolyhedron::canCorefineWith(const CGALHybridPolyhedron& other) const
{
  if (!Feature::ExperimentalFastCsgSafer.is_enabled()) {
    return true;
  }
  const char *reasonWontCorefine = nullptr;
  if (sharesAnyVertexWith(other)) {
    reasonWontCorefine = "operands share some vertices";
  } else if (!isManifold() || !other.isManifold()) {
    reasonWontCorefine = "non manifoldness detected";
  }
  if (reasonWontCorefine) {
    LOG("[fast-csg] Performing safer but slower nef operation instead of corefinement because %1$s.",
        reasonWontCorefine);
  }
  return !reasonWontCorefine;
}

void CGALHybridPolyhedron::minkowski(CGALHybridPolyhedron& other)
{
  nefPolyBinOp("minkowski", other,
               [&](CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef) {
    CGALUtils::inPlaceNefMinkowski(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::transform(const Transform3d& mat)
{
  auto det = mat.matrix().determinant();
  if (det == 0) {
    LOG(message_group::Warning, "Scaling a 3D object with 0 - removing object");
    clear();
  } else {
    auto t = CGALUtils::createAffineTransformFromMatrix<CGAL_HybridKernel3>(mat);

    if (auto mesh = getMesh()) {
      CGALUtils::transform(*mesh, mat);
      CGALUtils::cleanupMesh(*mesh, /* is_corefinement_result */ false);
      if (det < 0) {
        CGALUtils::reverseFaceOrientations(*mesh);
      }
    } else if (auto nef = getNefPolyhedron()) {
      CGALUtils::transform(*nef, mat);
    } else {
      assert(!"Bad hybrid polyhedron state");
    }
  }
}

void CGALHybridPolyhedron::resize(
  const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize)
{
  if (this->isEmpty()) return;

  transform(GeometryUtils::getResizeTransform(getBoundingBox(), newsize, autosize));
}

BoundingBox CGALHybridPolyhedron::getBoundingBox() const
{
  BoundingBox bbox;
  foreachVertexUntilTrue([&](const auto& pt) {
    bbox.extend(CGALUtils::vector_convert<Vector3d>(pt));
    return false;
  });
  return bbox;
}

std::string CGALHybridPolyhedron::dump() const
{
  assert(!"TODO: implement CGALHybridPolyhedron::dump!");
  return "?";
  // return OpenSCAD::dump_svg(toPolySet());
}

size_t CGALHybridPolyhedron::memsize() const
{
  size_t total = sizeof(CGALHybridPolyhedron);
  if (auto mesh = getMesh()) {
    total += numFacets() * 3 * sizeof(size_t);
    total += numVertices() * sizeof(point_t);
  } else if (auto nef = getNefPolyhedron()) {
    total += nef->bytes();
  }
  return total;
}

void CGALHybridPolyhedron::foreachVertexUntilTrue(
  const std::function<bool(const point_t& pt)>& f) const
{
  if (auto mesh = getMesh()) {
    for (auto v : mesh->vertices()) {
      if (f(mesh->point(v))) return;
    }
  } else if (auto nef = getNefPolyhedron()) {
    CGAL_HybridNef::Vertex_const_iterator vi;
    CGAL_forall_vertices(vi, *nef)
    {
      if (f(vi->point())) return;
    }
  } else {
    assert(!"Bad hybrid polyhedron state");
  }
}

std::string describeForDebug(const CGAL_HybridNef& nef)
{
  std::ostringstream stream;
  stream
  // << (nef.is_valid() ? "valid " : "INVALID ")
    << (nef.is_simple() ? "" : "NOT 2-manifold ")
    << nef.number_of_facets() << " facets"
  ;
  return stream.str();
}

std::string describeForDebug(const CGAL_HybridMesh& mesh) {
  std::ostringstream stream;
  stream
    << (CGAL::is_valid_polygon_mesh(mesh) ? "" : "INVALID ")
    << (CGAL::is_closed(mesh) ? "" : "UNCLOSED ")
    << mesh.number_of_faces() << " facets";
  return stream.str();
}

void CGALHybridPolyhedron::nefPolyBinOp(
  const std::string& opName, CGALHybridPolyhedron& other,
  const std::function<void(CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef)>
  & operation)
{
  auto& lhs = *convertToNef();
  auto& rhs = *other.convertToNef();

  if (Feature::ExperimentalFastCsgDebug.is_enabled()) {
    LOG("[fast-csg] %1$s: %2$s vs. %3$s",
        opName.c_str(), describeForDebug(lhs), describeForDebug(rhs));
  }

  operation(lhs, rhs);

  if (Feature::ExperimentalFastCsgDebug.is_enabled()) {
    if (!lhs.is_simple()) {
      LOG("[fast-csg] %1$s output is a %2$s", opName.c_str(), describeForDebug(lhs));
    }
  }
}

bool CGALHybridPolyhedron::meshBinOp(
  const std::string& opName, CGALHybridPolyhedron& other,
  const std::function<bool(CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out)>& operation)
{
  auto previousData = data;
  auto previousOtherData = other.data;

  auto success = false;

  auto debug = Feature::ExperimentalFastCsgDebug.is_enabled();
  std::string lhsDebugDumpFile, rhsDebugDumpFile;

  static std::map<std::string, int> opCount;
  auto opNumber = opCount[opName]++;

  try {
    auto& lhs = *convertToMesh();
    auto& rhs = *other.convertToMesh();

    if (debug) {
      LOG("[fast-csg] %1$s #%2$lu: %3$s vs. %4$s",
          opName.c_str(), opNumber, describeForDebug(lhs), describeForDebug(rhs));

      std::ostringstream lhsOut, rhsOut;
      lhsOut << opName << " " << opNumber << " lhs.off";
      rhsOut << opName << " " << opNumber << " rhs.off";
      lhsDebugDumpFile = lhsOut.str();
      rhsDebugDumpFile = rhsOut.str();

      std::ofstream(lhsDebugDumpFile) << lhs;
      std::ofstream(rhsDebugDumpFile) << rhs;
    }

    if ((success = operation(lhs, rhs, lhs))) {
      CGALUtils::cleanupMesh(lhs, /* is_corefinement_result */ true);

      if (debug) {
        remove(lhsDebugDumpFile.c_str());
        remove(rhsDebugDumpFile.c_str());
      }
    } else {
      LOG(message_group::Warning, "[fast-csg] Corefinement %1$s #%2$lu failed",
          opName.c_str(), opNumber);
    }
    if (debug) {
      if (!CGAL::is_valid_polygon_mesh(lhs) || !CGAL::is_closed(lhs)) {
        LOG(message_group::Warning,
            "[fast-csg] %1$s #%2$lu output is %3$s", opName.c_str(), opNumber, describeForDebug(lhs));
      }
    }
    // union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
  } catch (const CGAL::Failure_exception& e) {
    success = false;
    LOG(message_group::Warning,
        "[fast-csg] Corefinement %1$s #%2$lu failed with an error: %3$s\n", opName.c_str(), opNumber, e.what());
  }

  if (!success) {
    // Nef polyhedron is a costly object to create, and maybe we've just ditched some
    // to create our polyhedra. Revert back to whatever we had in case we already
    // had nefs.
    data = previousData;
    other.data = previousOtherData;

    if (debug) {
      LOG(message_group::Warning,
          "Dumps of operands were written to %1$s and %2$s", lhsDebugDumpFile.c_str(), rhsDebugDumpFile.c_str());
    }
  }

  return success;
}

std::shared_ptr<CGAL_HybridNef> CGALHybridPolyhedron::convertToNef()
{
  if (auto mesh = getMesh()) {
    auto nef = std::make_shared<CGAL_HybridNef>(*mesh);
    data = nef;
    return nef;
  } else if (auto nef = getNefPolyhedron()) {
    return nef;
  } else {
    throw "Bad data state";
  }
}

std::shared_ptr<CGAL_HybridMesh> CGALHybridPolyhedron::convertToMesh()
{
  if (auto mesh = getMesh()) {
    return mesh;
  } else if (auto nef = getNefPolyhedron()) {
    auto mesh = std::make_shared<CGAL_HybridMesh>();
    CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, *mesh);
    CGALUtils::cleanupMesh(*mesh, /* is_corefinement_result */ false);
    data = mesh;
    return mesh;
  } else {
    throw "Bad data state";
  }
}

bool CGALHybridPolyhedron::sharesAnyVertexWith(const CGALHybridPolyhedron& other) const
{
  if (other.numVertices() < numVertices()) {
    // The other has less vertices to index!
    return other.sharesAnyVertexWith(*this);
  }

  std::unordered_set<point_t> vertices;
  foreachVertexUntilTrue([&](const auto& p) {
    vertices.insert(p);
    return false;
  });

  auto foundCollision = false;
  other.foreachVertexUntilTrue(
    [&](const auto& p) {
    return foundCollision = vertices.find(p) != vertices.end();
  });

  return foundCollision;
}
