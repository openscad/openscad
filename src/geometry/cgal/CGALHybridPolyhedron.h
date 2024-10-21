// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <functional>
#include <memory>
#include <cstddef>
#include <string>
#include <variant>

#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"

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

  using point_t = CGAL::Point_3<CGAL_HybridKernel3>;
  using bbox_t = CGAL::Iso_cuboid_3<CGAL_HybridKernel3>;

  CGALHybridPolyhedron();
  CGALHybridPolyhedron(const std::shared_ptr<CGAL_HybridNef>& nef);
  CGALHybridPolyhedron(const std::shared_ptr<CGAL_HybridMesh>& mesh);
  CGALHybridPolyhedron(const CGALHybridPolyhedron& other);
  CGALHybridPolyhedron& operator=(const CGALHybridPolyhedron& other);

  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] size_t numFacets() const override;
  [[nodiscard]] size_t numVertices() const;
  [[nodiscard]] bool isManifold() const;
  [[nodiscard]] bool isValid() const;
  void clear();

  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;

  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 3; }
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;

  [[nodiscard]] std::shared_ptr<const PolySet> toPolySet() const;

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
  void transform(const Transform3d& mat) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  /*! Iterate over all vertices' points until the function returns true (for done). */
  void foreachVertexUntilTrue(const std::function<bool(const point_t& pt)>& f) const;

  std::shared_ptr<CGAL_HybridNef> convertToNef();
  std::shared_ptr<CGAL_HybridMesh> convertToMesh();

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
    const std::function<void(CGAL_HybridNef& destinationNef, CGAL_HybridNef& otherNef)>& operation);

  /*! Runs a binary operation that operates on polyhedra, stores the result in
   * the first one and potentially mutates (e.g. corefines) the second.
   * Returns false if the operation failed (e.g. because of shared edges), in
   * which case it may still have corefined the polyhedron, but it reverts the
   * original nef if there was one. */
  bool meshBinOp(
    const std::string& opName, CGALHybridPolyhedron& other,
    const std::function<bool(CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out)>& operation);

  [[nodiscard]] bool sharesAnyVertexWith(const CGALHybridPolyhedron& other) const;

  [[nodiscard]] bool canCorefineWith(const CGALHybridPolyhedron& other) const;

  /*! Returns the mesh if that's what's in the current data, or else nullptr.
   * Do NOT make this public. */
  [[nodiscard]] std::shared_ptr<CGAL_HybridMesh> getMesh() const;
  /*! Returns the nef polyhedron if that's what's in the current data, or else nullptr.
   * Do NOT make this public. */
  [[nodiscard]] std::shared_ptr<CGAL_HybridNef> getNefPolyhedron() const;

  // This contains data either as a polyhedron, or as a nef polyhedron.
  //
  // We stick to nef polyhedra in presence of non-manifold geometry or literal
  // edge-cases of the Polygon Mesh Processing corefinement functions (e.g. it
  // does not like shared edges, but tells us so politely).
  std::variant<std::shared_ptr<CGAL_HybridMesh>, std::shared_ptr<CGAL_HybridNef>> data;
};
