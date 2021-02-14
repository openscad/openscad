// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <CGAL/version.h>
#include "cgal.h"

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if CGAL_VERSION_NR >= CGAL_VERSION_NUMBER(5, 1, 0)
#define FAST_CSG_AVAILABLE
#else
#pragma message("[fast-csg] No support for fast-csg with CGAL " STRING( \
		CGAL_VERSION) ". Please compile against CGAL 5.1 or later to test the feature.")
#endif

#ifdef FAST_CSG_AVAILABLE

#include <boost/variant.hpp>
#include "Geometry.h"

class CGAL_Nef_polyhedron;
class CGALHybridPolyhedron;
class PolySet;
namespace CGALUtils {
std::shared_ptr<CGAL_Nef_polyhedron> createNefPolyhedronFromHybrid(
		const CGALHybridPolyhedron &hybrid);
}

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
class CGALHybridPolyhedron : public Geometry
{
public:
	VISITABLE_GEOMETRY();

	// Notes on kernels:
	// - CGAL::Epick isn't exact and gives errors with some algorithms on some models.
	// - CGAL_Kernel3 (CGAL::Cartesian<CGAL::Gmpq>) doesn't work with PMP's corefinement
	//   functions that seem to make assumptions of CGAL::Epeck or CGAL::Epick.
	// - It's relatively straightfoward to convert between CGAL::Epeck and CGAL_Kernel3
  //   (see cgalutils-kernel.cc). However we may want to migrate CGAL_Nef_polyhedron
  //   to CGAL::Epeck (and possibly, retire it).
	// https://doc.cgal.org/latest/Kernel_d/structCGAL_1_1Epeck__d.html
	typedef CGAL::Epeck kernel_t;
	typedef CGAL::Point_3<kernel_t> point_t;
	typedef CGAL::Nef_polyhedron_3<kernel_t> nef_polyhedron_t;
	typedef CGAL::Polyhedron_3<kernel_t> polyhedron_t;
	typedef CGAL::Iso_cuboid_3<kernel_t> bbox_t;

	CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t> &nef);
	CGALHybridPolyhedron(const shared_ptr<polyhedron_t> &polyhedron);
	CGALHybridPolyhedron(const CGALHybridPolyhedron &other);
	CGALHybridPolyhedron() = delete;

	bool isEmpty() const;
	size_t numFacets() const;
	size_t numVertices() const;
	bool isManifold() const;
	void clear();

	size_t memsize() const override;
	BoundingBox getBoundingBox() const override;

	std::string dump() const override;
	unsigned int getDimension() const override { return 3; }
	Geometry *copy() const override { return new CGALHybridPolyhedron(*this); }

	std::shared_ptr<const PolySet> toPolySet() const;

	/*! In-place union (this may also mutate/corefine the other polyhedron). */
	void operator+=(CGALHybridPolyhedron &other);
	/*! In-place intersection (this may also mutate/corefine the other polyhedron). */
	void operator*=(CGALHybridPolyhedron &other);
	/*! In-place difference (this may also mutate/corefine the other polyhedron). */
	void operator-=(CGALHybridPolyhedron &other);
	/*! In-place minkowksi operation. If the other polyhedron is non-convex,
	 * it is also modified during the computation, i.e., it is decomposed into convex pieces.
	 */
	void minkowski(CGALHybridPolyhedron &other);
	virtual void transform(const Transform3d &mat) override;
	virtual void resize(const Vector3d &newsize, const Eigen::Matrix<bool, 3, 1> &autosize) override;

	/*! Iterate over all vertices' points until the function returns true (for done). */
	void foreachVertexUntilTrue(const std::function<bool(const point_t &pt)> &f) const;

private:
	// Old GCC versions used to build releases have object file limitations.
	// This conversion function could have been in the class but it requires knowledge
	// of polyhedra of two different kernels, which instantiates huge amounts of templates.
	friend std::shared_ptr<CGAL_Nef_polyhedron> CGALUtils::createNefPolyhedronFromHybrid(
			const CGALHybridPolyhedron &hybrid);

	bool sharesAnyVertexWith(const CGALHybridPolyhedron &other) const;
	bool needsNefForOperationWith(const CGALHybridPolyhedron &other) const;

	/*! Runs a binary operation that operates on nef polyhedra, stores the result in
	 * the first one and potentially mutates (e.g. corefines) the second. */
	void nefPolyBinOp(const std::string &opName, CGALHybridPolyhedron &other,
										const std::function<void(nef_polyhedron_t &destinationNef,
																						 nef_polyhedron_t &otherNef)> &operation);

	/*! Runs a binary operation that operates on polyhedra, stores the result in
	 * the first one and potentially mutates (e.g. corefines) the second. */
	void polyBinOp(
			const std::string &opName, CGALHybridPolyhedron &other,
			const std::function<void(polyhedron_t &destinationPoly, polyhedron_t &otherPoly)> &operation);

	polyhedron_t &convertToPolyhedron();
	nef_polyhedron_t &convertToNefPolyhedron();

	/*! Returns the polyhedron if that's what's in the current data, or else nullptr.
	 * Do NOT make this public. */
	polyhedron_t *getPolyhedron() const;
	/*! Returns the nef polyhedron if that's what's in the current data, or else nullptr.
	 * Do NOT make this public. */
	nef_polyhedron_t *getNefPolyhedron() const;

	bbox_t getExactBoundingBox() const;

	bool boundingBoxesIntersect(const CGALHybridPolyhedron &other) const
	{
		for (auto &bbox : bboxes)
			if (other.boundingBoxesIntersect(bbox)) return true;
		return false;
	}

	bool boundingBoxesIntersect(const bbox_t &c) const
	{
		for (auto &bbox : bboxes)
			if (CGAL::intersection(c, bbox) != boost::none) return true;
		return false;
	}

	// This contains data either as a polyhedron, or as a nef polyhedron.
	//
	// We stick to nef polyhedra in presence of non-manifold geometry (detected in
	// operations where the operands share any vertex), which breaks the
	// algorithms of Polygon Mesh Processing library that operate on normal
	// (non-nef) polyhedra.
	boost::variant<std::shared_ptr<polyhedron_t>, std::shared_ptr<nef_polyhedron_t>> data;
	// Keeps track of the bounding boxes of the solid components of this polyhedron.
	// This allows fast unions with disjoint polyhedra.
	std::vector<bbox_t> bboxes;
};

#endif // FAST_CSG_AVAILABLE
