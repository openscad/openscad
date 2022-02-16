// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include "cgal.h"

#include <boost/variant.hpp>
#include "Geometry.h"

class CGAL_Nef_polyhedron;
class CGALHybridPolyhedron;
class PolySet;

namespace CGAL {
template <typename P>
class Surface_mesh;
}
namespace CGALUtils {
std::shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(
  const CGALHybridPolyhedron& hybrid);

std::shared_ptr<const Geometry> applyMinkowskiHybrid(
  const Geometry::Geometries& children);
} // namespace CGALUtils

/*! A mutable polyhedron backed by a CGAL::Surface_mesh and fast Polygon Mesh
 * Processing (PMP) CSG functions when possible (manifold cases), or by a
 * CGAL::Nef_polyhedron_3 when it's not (non manifold cases).
 *
 * Note that even `cube(1); translate([1, 0, 0]) cube(1)` is considered
 * non-manifold because of shared vertices. PMP seems to be fine with edges
 * that share segments with others, so long as there's no shared vertex.
 */
class CGALHybridPolyhedron : public Geometry
{
public:
  VISITABLE_GEOMETRY();

  typedef CGAL::Point_3<CGAL_HybridKernel3> point_t;
  typedef CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> nef_polyhedron_t;
  typedef CGAL::Iso_cuboid_3<CGAL_HybridKernel3> bbox_t;
  typedef CGAL::Surface_mesh<point_t> mesh_t;

  CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t>& nef);
  CGALHybridPolyhedron(const shared_ptr<mesh_t>& mesh);
  CGALHybridPolyhedron(const CGALHybridPolyhedron& other);
  CGALHybridPolyhedron();
  CGALHybridPolyhedron& operator=(const CGALHybridPolyhedron& other);

  bool isEmpty() const override;
  size_t numFacets() const override;
  size_t numVertices() const;
  bool isManifold() const;
  void clear();

  size_t memsize() const override;
  BoundingBox getBoundingBox() const override
  {
    assert(false && "not implemented");
    return BoundingBox();
  }

  std::string dump() const override;
  unsigned int getDimension() const override { return 3; }
  Geometry *copy() const override { return new CGALHybridPolyhedron(*this); }

  std::shared_ptr<const PolySet> toPolySet() const;

  /*! In-place union (this may also mutate/corefine the other polyhedron). */
  void operator+=(CGALHybridPolyhedron& other);
  /*! In-place intersection (this may also mutate/corefine the other polyhedron). */
  void operator*=(CGALHybridPolyhedron& other);
  /*! In-place difference (this may also mutate/corefine the other polyhedron). */
  void operator-=(CGALHybridPolyhedron& other);
  /*! In-place minkowksi operation. If the other polyhedron is non-convex,
   * it is also modified during the computation, i.e., it is decomposed into convex pieces.
   */
  void minkowski(CGALHybridPolyhedron& other);
  virtual void transform(const Transform3d& mat) override;
  virtual void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  /*! Iterate over all vertices' points until the function returns true (for done). */
  void foreachVertexUntilTrue(const std::function<bool(const point_t& pt)>& f) const;

private:
  // Old GCC versions used to build releases have object file limitations.
  // This conversion function could have been in the class but it requires knowledge
  // of polyhedra of two different kernels, which instantiates huge amounts of templates.
  friend std::shared_ptr<CGAL_Nef_polyhedron> CGALUtils::createNefPolyhedronFromHybrid(
    const CGALHybridPolyhedron& hybrid);

  friend std::shared_ptr<const Geometry> CGALUtils::applyMinkowskiHybrid(
    const Geometry::Geometries& children);

  /*! Runs a binary operation that operates on nef polyhedra, stores the result in
   * the first one and potentially mutates (e.g. corefines) the second. */
  void nefPolyBinOp(
    const std::string& opName, CGALHybridPolyhedron& other,
    const std::function<void(
                          nef_polyhedron_t& destinationNef,
                          nef_polyhedron_t& otherNef)>& operation);

  /*! Runs a binary operation that operates on polyhedra, stores the result in
   * the first one and potentially mutates (e.g. corefines) the second.
   * Returns false if the operation failed (e.g. because of shared edges), in
   * which case it may still have corefined the polyhedron, but it reverts the
   * original nef if there was one. */
  bool meshBinOp(
    const std::string& opName, CGALHybridPolyhedron& other,
    const std::function<bool(mesh_t& lhs, mesh_t& rhs, mesh_t& out)>& operation);

  nef_polyhedron_t& convertToNef();
  mesh_t& convertToMesh();

  bool sharesAnyVertexWith(const CGALHybridPolyhedron& other) const;

  /*! Returns the mesh if that's what's in the current data, or else nullptr.
   * Do NOT make this public. */
  std::shared_ptr<mesh_t> getMesh() const;
  /*! Returns the nef polyhedron if that's what's in the current data, or else nullptr.
   * Do NOT make this public. */
  std::shared_ptr<nef_polyhedron_t> getNefPolyhedron() const;

  bbox_t getExactBoundingBox() const;

  // This contains data either as a polyhedron, or as a nef polyhedron.
  //
  // We stick to nef polyhedra in presence of non-manifold geometry or literal
  // edge-cases of the Polygon Mesh Processing corefinement functions (e.g. it
  // does not like shared edges, but tells us so politely).
  boost::variant<std::shared_ptr<mesh_t>, std::shared_ptr<nef_polyhedron_t>> data;
};
