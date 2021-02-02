// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <boost/variant.hpp>
#include "bounding_boxes.h"
#include "cgal.h"

class Geometry;
class PolySet;

/*! A mutable polyhedron backed by a CGAL::Polyhedron_3 and fast Polygon Mesh
 * Processing (PMP) CSG functions when possible (manifold cases), or by a
 * CGAL::Nef_polyhedron_3 when it's not (non manifold cases).
 *
 * Note that even `cube(1); translate([1, 0, 0]) cube(1)` is considered
 * non-manifold because of shared vertices. PMP seems to be fine with edges
 * that share segments with others, so long as there's no shared vertex.
 *
 * Also, keeps track of the bounding boxes of past operations to fast-track
 * unions of disjoint bodies.
 *
 * TODO(ochafik): Turn this into a regular Geometry and handle it everywhere
 * the CGAL_Nef_polyhedron is handled.
 */
class Polyhedron
{
	// https://doc.cgal.org/latest/Kernel_d/structCGAL_1_1Epeck__d.html
	// TODO(ochafik): Try other kernels, e.g. typedef CGAL::Simple_cartesian<CGAL::Gmpq> kernel_t;
	typedef CGAL::Epeck kernel_t;
	typedef CGAL::Point_3<kernel_t> point_t;
	typedef CGAL::Polyhedron_3<kernel_t> polyhedron_t;
	typedef CGAL::Nef_polyhedron_3<kernel_t> nef_polyhedron_t;

	// This contains data either as a polyhedron, or as a nef polyhedron.
	//
	// We stick to nef polyhedra in presence of non-manifold geometry (detected in
	// operations where the operands share any vertex), which breaks the
	// algorithms of Polygon Mesh Processing library that operate on normal
	// (non-nef) polyhedra.
	boost::variant<std::shared_ptr<polyhedron_t>, std::shared_ptr<nef_polyhedron_t>> data;
  // Keeps track of the bounding boxes of the solid components of this polyhedron.
  // This allows fast unions with disjoint polyhedra.
	BoundingBoxes bboxes;

public:
	/*! Builds a polyhedron using the provided, untrusted PolySet.
	 * Face orientation is checked (and reversed if needed), faces are
	 * triangulated (requirement of Polygon Mesh Processing functions),
	 * and we check manifoldness (we use a nef polyhedra for non-manifold cases).
	 */
	Polyhedron(const PolySet &ps);

	/*! Builds a polyhedron using a legacy nef polyhedron object.
	 * This transitional method will disappear when this Polyhedron object is
	 * fully integrated and replaces all of CGAL_Nef_polyhedron's uses.
	 */
	Polyhedron(const CGAL_Nef_polyhedron3 &nef);

	bool isEmpty() const;
	size_t numFacets() const;
	size_t numVertices() const;
	bool isManifold() const;
	void clear();
  /*! TODO(ochafik): Make this class inherit Geometry, plug the gaps and drop this method. */
	std::shared_ptr<const Geometry> toGeometry() const;

	/*! In-place union (this may also mutate/corefine the other polyhedron). */
	void operator+=(Polyhedron &other);
	/*! In-place intersection (this may also mutate/corefine the other polyhedron). */
	void operator*=(Polyhedron &other);
	/*! In-place difference (this may also mutate/corefine the other polyhedron). */
	void operator-=(Polyhedron &other);
	/*! In-place minkowksi operation. If the other polyhedron is non-convex,
	 * it is also modified during the computation, i.e., it is decomposed into convex pieces.
	 */
	void minkowski(Polyhedron &other);

private:
	/*! Iterate over all vertices' points until the function returns true (for done). */
	void foreachVertexUntilTrue(const std::function<bool(const point_t &pt)> &f) const;
	bool sharesAnyVertexWith(const Polyhedron &other) const;

	/*! Runs a binary operation that operates on nef polyhedra, stores the result in
	 * the first one and potentially mutates (e.g. corefines) the second. */
	void nefPolyBinOp(const std::string &opName, Polyhedron &other,
										const std::function<void(nef_polyhedron_t &destinationNef,
																						 nef_polyhedron_t &otherNef)> &operation);

	/*! Runs a binary operation that operates on polyhedra, stores the result in
	 * the first one and potentially mutates (e.g. corefines) the second. */
	void polyBinOp(
			const std::string &opName, Polyhedron &other,
			const std::function<void(polyhedron_t &destinationPoly, polyhedron_t &otherPoly)> &operation);

	nef_polyhedron_t &convertToNefPolyhedron();
	polyhedron_t &convertToPolyhedron();
};
