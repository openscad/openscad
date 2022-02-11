// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALHybridPolyhedron.h"

#include "cgalutils.h"
#include "hash.h"
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/helpers.h>
#include <fstream>
#include <sstream>
#include <stdio.h>

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
void cleanupMesh(CGALHybridPolyhedron::mesh_t& mesh, bool is_corefinement_result)
{
  mesh.collect_garbage();
#if FAST_CSG_KERNEL_IS_LAZY
  // If exact corefinement callbacks are enabled, no need to make numbers exact here again.
  auto make_exact = 
    Feature::ExperimentalFastCsgExactCorefinementCallback.is_enabled()
      ? !is_corefinement_result
      : Feature::ExperimentalFastCsgExact.is_enabled();

  if (make_exact) {
    for (auto v : mesh.vertices()) {
      auto &pt = mesh.point(v);
      CGAL::exact(pt.x());
      CGAL::exact(pt.y());
      CGAL::exact(pt.z());
    }
  }
#endif // FAST_CSG_KERNEL_IS_LAZY
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t>& nef)
{
  assert(nef);
  data = nef;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<mesh_t>& mesh)
{
  assert(mesh);
  data = mesh;

}

CGALHybridPolyhedron::CGALHybridPolyhedron(const CGALHybridPolyhedron& other)
{
  *this = other;
}

CGALHybridPolyhedron& CGALHybridPolyhedron::operator=(const CGALHybridPolyhedron& other)
{
  if (auto nef = other.getNefPolyhedron()) {
    data = make_shared<nef_polyhedron_t>(*nef);
  } else if (auto mesh = other.getMesh()) {
    data = make_shared<mesh_t>(*mesh);
  } else {
    assert(!"Bad hybrid polyhedron state");
  }
  return *this;
}

CGALHybridPolyhedron::CGALHybridPolyhedron()
{
  data = make_shared<CGALHybridPolyhedron::mesh_t>();
}

CGALHybridPolyhedron::nef_polyhedron_t *CGALHybridPolyhedron::getNefPolyhedron() const
{
  auto pp = boost::get<shared_ptr<nef_polyhedron_t>>(&data);
  return pp ? pp->get() : nullptr;
}

CGALHybridPolyhedron::mesh_t *CGALHybridPolyhedron::getMesh() const
{
  auto pp = boost::get<shared_ptr<mesh_t>>(&data);
  return pp ? pp->get() : nullptr;
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
    return CGAL::is_closed(*mesh);
  } else if (auto nef = getNefPolyhedron()) {
    return nef->is_simple();
  }
  assert(!"Bad hybrid polyhedron state");
  return false;
}

shared_ptr<const PolySet> CGALHybridPolyhedron::toPolySet() const
{
  if (auto mesh = getMesh()) {
    auto ps = make_shared<PolySet>(3, /* convex */ unknown);
    auto err = CGALUtils::createPolySetFromMesh(*mesh, *ps);
    assert(!err);
    return ps;
  } else if (auto nef = getNefPolyhedron()) {
    auto ps = make_shared<PolySet>(3, /* convex */ unknown);
    auto err = CGALUtils::createPolySetFromNefPolyhedron3(*nef, *ps);
    assert(!err);
    return ps;
  } else {
    assert(!"Bad hybrid polyhedron state");
    return nullptr;
  }
}

void CGALHybridPolyhedron::clear()
{
  data = make_shared<mesh_t>();
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh union", other, [&](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
      return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef union", other,
               [&](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
    CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh intersection", other,
                  [&](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
      return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef intersection", other,
               [&](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
    CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron& other)
{
  if (canCorefineWith(other)) {
    if (meshBinOp("corefinement mesh difference", other,
                  [&](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
      return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
    })) return;
  }

  nefPolyBinOp("nef difference", other,
               [&](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
    CGALUtils::inPlaceNefDifference(destinationNef, otherNef);
  });
}

bool CGALHybridPolyhedron::canCorefineWith(const CGALHybridPolyhedron& other) const
{
  if (Feature::ExperimentalFastCsgTrustCorefinement.is_enabled()) {
    return true;
  }
  const char* reasonWontCorefine = nullptr;
  if (sharesAnyVertexWith(other)) {
    reasonWontCorefine = "operands share some vertices";
  } else if (!isManifold() || !other.isManifold()) {
    reasonWontCorefine = "non manifoldness detected";
  }
  if (reasonWontCorefine) {
    LOG(message_group::None, Location::NONE, "",
        "[fast-csg] Performing safer but slower nef operation instead of corefinement because %1$s. "
        "(can override with fast-csg-trust-corefinement)",
        reasonWontCorefine);
  }
  return !reasonWontCorefine;
}

void CGALHybridPolyhedron::minkowski(CGALHybridPolyhedron& other)
{
  nefPolyBinOp("minkowski", other,
               [&](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
    CGALUtils::inPlaceNefMinkowski(destinationNef, otherNef);
  });
}

void CGALHybridPolyhedron::transform(const Transform3d& mat)
{
  if (mat.matrix().determinant() == 0) {
    LOG(message_group::Warning, Location::NONE, "", "Scaling a 3D object with 0 - removing object");
    clear();
  } else {
    auto t = CGALUtils::createAffineTransformFromMatrix<CGAL_HybridKernel3>(mat);

    if (auto mesh = getMesh()) {
      CGALUtils::transform(*mesh, mat);
      cleanupMesh(*mesh, /* is_corefinement_result */ false);
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

  transform(
    CGALUtils::computeResizeTransform(getExactBoundingBox(), getDimension(), newsize, autosize));
}

CGALHybridPolyhedron::bbox_t CGALHybridPolyhedron::getExactBoundingBox() const
{
  bbox_t result(0, 0, 0, 0, 0, 0);
  std::vector<point_t> points;
  // TODO(ochafik): Optimize this!
  foreachVertexUntilTrue([&](const auto& pt) {
    points.push_back(pt);
    return false;
  });
  if (points.size()) CGAL::bounding_box(points.begin(), points.end());
  return result;
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
    nef_polyhedron_t::Vertex_const_iterator vi;
    CGAL_forall_vertices(vi, *nef)
    {
      if (f(vi->point())) return;
    }
  } else {
    assert(!"Bad hybrid polyhedron state");
  }
}

void CGALHybridPolyhedron::nefPolyBinOp(
  const std::string& opName, CGALHybridPolyhedron& other,
  const std::function<void(nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef)>
  & operation)
{
  LOG(message_group::None, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
      opName.c_str(), numFacets(), other.numFacets());

  operation(convertToNef(), other.convertToNef());
}

bool CGALHybridPolyhedron::meshBinOp(
  const std::string& opName, CGALHybridPolyhedron& other,
  const std::function<bool(mesh_t& lhs, mesh_t& rhs, mesh_t& out)>& operation)
{
  LOG(message_group::None, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
      opName.c_str(), numFacets(), other.numFacets());

  auto previousData = data;
  auto previousOtherData = other.data;

  auto success = false;

  std::string lhsDebugDumpFile, rhsDebugDumpFile;

  try {
    auto& lhs = convertToMesh();
    auto& rhs = other.convertToMesh();

    if (Feature::ExperimentalFastCsgDebugCorefinement.is_enabled()) {
      static std::map<std::string, size_t> opCount;
      auto opNumber = opCount[opName]++;

      std::ostringstream lhsOut, rhsOut;
      lhsOut << opName << " " << opNumber << " lhs.off";
      rhsOut << opName << " " << opNumber << " rhs.off";
      lhsDebugDumpFile = lhsOut.str();
      rhsDebugDumpFile = rhsOut.str();
      
      std::ofstream(lhsDebugDumpFile) << lhs;
      std::ofstream(rhsDebugDumpFile) << rhs;
    }

    auto operationWithRepairs = [&]() {
      auto repair = Feature::ExperimentalFastCsgRepairSelfIntersections.is_enabled();
      try {
        return operation(lhs, rhs, lhs, /* throwOnSelfIntersection= */ repair);
      } catch (CGAL::Polygon_mesh_processing::Corefinement::Self_intersection_exception &e) {
        LOG(message_group::Warning, Location::NONE, "",
          "[fast-csg] Self intersections detected during %1$s: attempting repairs (%2$s)...\n", opName.c_str(), e.what());
        CGALUtils::removeSelfIntersections(lhs);
        CGALUtils::removeSelfIntersections(rhs);
        return operation(lhs, rhs, lhs, /* throwOnSelfIntersection= */ false);
      }
    };

    if ((success = operationWithRepairs())) {
      cleanupMesh(lhs, /* is_corefinement_result */ true);

      if (Feature::ExperimentalFastCsgDebugCorefinement.is_enabled()) {
        remove(lhsDebugDumpFile.c_str());
        remove(rhsDebugDumpFile.c_str());
      }
    } else {
      LOG(message_group::Warning, Location::NONE, "", "[fast-csg] Corefinement %1$s failed",
          opName.c_str());
    }
  } catch (const std::exception& e) {
    // This can be a CGAL::Failure_exception, a CGAL::Intersection_of_constraints_exception or who
    // knows what else...
    success = false;
    LOG(message_group::Warning, Location::NONE, "",
        "[fast-csg] Corefinement %1$s failed with an error: %2$s\n", opName.c_str(), e.what());
    if (Feature::ExperimentalFastCsgDebugCorefinement.is_enabled()) {
      LOG(message_group::Warning, Location::NONE, "",
          "Dumps of operands were written to %1$s and %2$s", lhsDebugDumpFile.c_str(), rhsDebugDumpFile.c_str());
    }
  }

  if (!success) {
    // Nef polyhedron is a costly object to create, and maybe we've just ditched some
    // to create our polyhedra. Revert back to whatever we had in case we already
    // had nefs.
    data = previousData;
    other.data = previousOtherData;
  }

  return success;
}

CGALHybridPolyhedron::nef_polyhedron_t& CGALHybridPolyhedron::convertToNef()
{
  if (auto mesh = getMesh()) {
    auto nef = make_shared<nef_polyhedron_t>(*mesh);
    data = nef;
    return *nef;
  } else if (auto nef = getNefPolyhedron()) {
    return *nef;
  } else {
    throw "Bad data state";
  }
}

CGALHybridPolyhedron::mesh_t& CGALHybridPolyhedron::convertToMesh()
{
  if (auto mesh = getMesh()) {
    return *mesh;
  } else if (auto nef = getNefPolyhedron()) {
    auto mesh = make_shared<mesh_t>();
    CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, *mesh);
    cleanupMesh(*mesh, /* is_corefinement_result */ false);
    data = mesh;
    return *mesh;
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
