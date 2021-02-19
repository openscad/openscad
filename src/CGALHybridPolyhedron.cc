// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include "cgalutils.h"
#include "hash.h"
#include "scoped_timer.h"

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t> &nef)
{
	assert(nef);
	data = nef;
	bboxes.push_back(CGALUtils::boundingBox(*nef));
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<polyhedron_t> &polyhedron)
{
	assert(polyhedron);
	data = polyhedron;
	bboxes.push_back(CGALUtils::boundingBox(*polyhedron));
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const CGALHybridPolyhedron &other) : bboxes(other.bboxes)
{
	if (auto poly = other.getPolyhedron()) {
		data = make_shared<polyhedron_t>(*poly);
	}
	else if (auto nef = other.getNefPolyhedron()) {
		data = make_shared<nef_polyhedron_t>(*nef);
	}
	else {
		assert(!"Bad hybrid polyhedron state");
	}
}

CGALHybridPolyhedron::nef_polyhedron_t *CGALHybridPolyhedron::getNefPolyhedron() const
{
	auto pp = boost::get<shared_ptr<nef_polyhedron_t>>(&data);
	return pp ? pp->get() : nullptr;
}

CGALHybridPolyhedron::polyhedron_t *CGALHybridPolyhedron::getPolyhedron() const
{
	auto pp = boost::get<shared_ptr<polyhedron_t>>(&data);
	return pp ? pp->get() : nullptr;
}

BoundingBox CGALHybridPolyhedron::getBoundingBox() const
{
	return CGALUtils::createBoundingBoxFromIsoCuboid(getExactBoundingBox());
}

CGALHybridPolyhedron::bbox_t CGALHybridPolyhedron::getExactBoundingBox() const
{
	bbox_t result(0, 0, 0, 0, 0, 0);
	std::vector<point_t> points;
	for (auto &bbox : bboxes) {
		points.push_back(bbox.min());
		points.push_back(bbox.max());
	}
	if (points.size()) CGAL::bounding_box(points.begin(), points.end());
	return result;
}

bool CGALHybridPolyhedron::isEmpty() const
{
	return numFacets() == 0;
}

size_t CGALHybridPolyhedron::numFacets() const
{
	if (auto poly = getPolyhedron()) {
		return poly->size_of_facets();
	}
	if (auto nef = getNefPolyhedron()) {
		return nef->number_of_facets();
	}
	assert(!"Bad hybrid polyhedron state");
	return 0;
}

size_t CGALHybridPolyhedron::numVertices() const
{
	if (auto poly = getPolyhedron()) {
		return poly->size_of_vertices();
	}
	else if (auto nef = getNefPolyhedron()) {
		return nef->number_of_vertices();
	}
	assert(!"Bad hybrid polyhedron state");
	return 0;
}

bool CGALHybridPolyhedron::isManifold() const
{
	if (getPolyhedron()) {
		// We don't keep simple polyhedra if they're not manifold (see contructor)
		return true;
	}
	else if (auto nef = getNefPolyhedron()) {
		return nef->is_simple();
	}
	assert(!"Bad hybrid polyhedron state");
	return false;
}

shared_ptr<const PolySet> CGALHybridPolyhedron::toPolySet() const
{
	if (auto poly = getPolyhedron()) {
		auto ps = make_shared<PolySet>(3, /* convex */ unknown);
		auto err = CGALUtils::createPolySetFromPolyhedron(*poly, *ps);
		assert(!err);
		return ps;
	}
	else if (auto nef = getNefPolyhedron()) {
		auto ps = make_shared<PolySet>(3, /* convex */ unknown);
		auto err = CGALUtils::createPolySetFromNefPolyhedron3(*nef, *ps);
		assert(!err);
		return ps;
	}
	else {
		assert(!"Bad hybrid polyhedron state");
		return nullptr;
	}
}

void CGALHybridPolyhedron::clear()
{
	data = make_shared<polyhedron_t>();
	bboxes.clear();
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron &other)
{
	if (!boundingBoxesIntersect(other) && isManifold() && other.isManifold()) {
		polyBinOp("fast union", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			CGALUtils::copyPolyhedron(otherPoly, destinationPoly);
			return true;
		});
	}
	else if (!isManifold() || !other.isManifold() ||
					 !polyBinOp("corefinement union", other,
											[&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
												return CGALUtils::corefineAndComputeUnion(destinationPoly, otherPoly);
											})) {
		nefPolyBinOp("nef union", other,
								 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
									 CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
								 });
	}

	bboxes.insert(bboxes.end(), other.bboxes.begin(), other.bboxes.end());
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron &other)
{
	if (!boundingBoxesIntersect(other)) {
		LOG(message_group::Warning, Location::NONE, "", "[fast-csg] Empty intersection");
		clear();
		return;
	}

	std::vector<bbox_t> new_bboxes;
	for (auto &bbox : bboxes)
		if (other.boundingBoxesIntersect(bbox)) new_bboxes.push_back(bbox);
	bboxes = new_bboxes;

	if (isManifold() && other.isManifold() &&
			polyBinOp("corefinement intersection", other,
								[&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
									return CGALUtils::corefineAndComputeIntersection(destinationPoly, otherPoly);
								})) {
		return;
	}

	nefPolyBinOp("nef intersection", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron &other)
{
	if (!boundingBoxesIntersect(other)) {
		LOG(message_group::Warning, Location::NONE, "",
				"[fast-csg] Difference with non-intersecting geometry");
		return;
	}

	// Note: we don't need to change the bbox.

	if (isManifold() && other.isManifold() &&
			polyBinOp("corefinement difference", other,
								[&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
									return CGALUtils::corefineAndComputeDifference(destinationPoly, otherPoly);
								})) {
		return;
	}

	nefPolyBinOp("nef difference", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefDifference(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::minkowski(CGALHybridPolyhedron &other)
{
	nefPolyBinOp("minkowski", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefMinkowski(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::transform(const Transform3d &mat)
{
	if (mat.matrix().determinant() == 0) {
		LOG(message_group::Warning, Location::NONE, "", "Scaling a 3D object with 0 - removing object");
		clear();
	}
	else {
		auto t = CGALUtils::createAffineTransformFromMatrix<CGAL_HybridKernel3>(mat);
		if (auto poly = getPolyhedron()) {
			CGALUtils::transform(*poly, mat);
		}
		else if (auto nef = getNefPolyhedron()) {
			CGALUtils::transform(*nef, mat);
		}
		else {
			assert(!"Bad hybrid polyhedron state");
		}

		for (auto &bbox : bboxes) bbox.transform(t);
	}
}

void CGALHybridPolyhedron::resize(const Vector3d &newsize,
																	const Eigen::Matrix<bool, 3, 1> &autosize)
{
	if (this->isEmpty()) return;

	transform(
			CGALUtils::computeResizeTransform(getExactBoundingBox(), getDimension(), newsize, autosize));
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
	total += bboxes.size() * sizeof(bbox_t);
	if (auto poly = getPolyhedron()) {
		total += poly->bytes();
	}
	else if (auto nef = getNefPolyhedron()) {
		total += nef->bytes();
	}
	return total;
}

void CGALHybridPolyhedron::foreachVertexUntilTrue(
		const std::function<bool(const point_t &pt)> &f) const
{
	if (auto poly = getPolyhedron()) {
		polyhedron_t::Vertex_const_iterator vi;
		CGAL_forall_vertices(vi, *poly)
		{
			if (f(vi->point())) return;
		}
	}
	else if (auto nef = getNefPolyhedron()) {
		nef_polyhedron_t::Vertex_const_iterator vi;
		CGAL_forall_vertices(vi, *nef)
		{
			if (f(vi->point())) return;
		}
	}
	else {
		assert(!"Bad hybrid polyhedron state");
	}
}

void CGALHybridPolyhedron::nefPolyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<void(nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef)>
				&operation)
{
	SCOPED_PERFORMANCE_TIMER(opName);

	LOG(message_group::Echo, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
			opName.c_str(), numFacets(), other.numFacets());

	operation(convertToNefPolyhedron(), other.convertToNefPolyhedron());
}

bool CGALHybridPolyhedron::polyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<bool(polyhedron_t &destinationPoly, polyhedron_t &otherPoly)> &operation)
{
	SCOPED_PERFORMANCE_TIMER(opName);

#ifdef FAST_CSG_TEST_SHARED_VERTICES
	if (sharesAnyVertexWith(other)) {
		// Looks like corefinement functions can leave the polyhedron with degenerate
		// faces or other invalid state when they bail out (returning false or after
		// some of precondition failure (examples: testdata/scad/3D/features/mirror-tests.scad
		// and testdata/scad/3D/features/polyhedron-tests.scad).
		// This makes it impossible to build a nef out of the resulting polyhedron,
		// so this check aims to avoid some of those cases.
		// It's probably the wrong check to do, but this fixes 2 tests.
		LOG(message_group::Warning, Location::NONE, "",
				"[fast-csg] Operands share vertices, opting out of corefinement out of precaution.");
		return false;
	}
#endif // FAST_CSG_TRUST_COREFINEMENT

	LOG(message_group::Echo, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
			opName.c_str(), numFacets(), other.numFacets());

	auto previousData = data;
	auto previousOtherData = other.data;

	auto success = false;
	CGALUtils::CGALErrorBehaviour behaviour{CGAL::THROW_EXCEPTION};
	try {
		if (!(success = operation(convertToPolyhedron(), other.convertToPolyhedron()))) {
			LOG(message_group::Warning, Location::NONE, "", "[fast-csg] Corefinement %1$s failed",
					opName.c_str());
		}
	} catch (const std::exception &e) {
		// This can be a CGAL::Failure_exception, a CGAL::Intersection_of_constraints_exception or who
		// knows what else...
		success = false;
		LOG(message_group::Warning, Location::NONE, "",
				"[fast-csg] Corefinement %1$s failed with an error: %2$s", opName.c_str(), e.what());
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

#ifdef FAST_CSG_TEST_SHARED_VERTICES
bool CGALHybridPolyhedron::sharesAnyVertexWith(const CGALHybridPolyhedron &other) const
{
	if (other.numVertices() < numVertices()) {
		// The other has less vertices to index!
		return other.sharesAnyVertexWith(*this);
	}

	std::unordered_set<point_t> vertices;
	foreachVertexUntilTrue([&](const auto &p) {
		vertices.insert(p);
		return false;
	});

	auto foundCollision = false;
	other.foreachVertexUntilTrue(
			[&](const auto &p) { return foundCollision = vertices.find(p) != vertices.end(); });

	return foundCollision;
}
#endif // FAST_CSG_TEST_SHARED_VERTICES

CGALHybridPolyhedron::nef_polyhedron_t &CGALHybridPolyhedron::convertToNefPolyhedron()
{
	if (auto poly = getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("polyhedron -> nef");

		auto nef = make_shared<nef_polyhedron_t>(*poly);
		data = nef;
		return *nef;
	}
	else if (auto nef = getNefPolyhedron()) {
		return *nef;
	}
	else {
		throw "Bad data state";
	}
}

CGALHybridPolyhedron::polyhedron_t &CGALHybridPolyhedron::convertToPolyhedron()
{
	if (auto poly = getPolyhedron()) {
		return *poly;
	}
	else if (auto nef = getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("nef -> polyhedron");

		auto poly = make_shared<polyhedron_t>();
		CGALUtils::convertNefToPolyhedron(*nef, *poly);
		data = poly;
		return *poly;
	}
	else {
		throw "Bad data state";
	}
}

#endif // FAST_CSG_AVAILABLE
