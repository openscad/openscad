// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <unordered_set>

#include "cgalutils.h"
#include "hash.h"
#include "polyset.h"
#include "scoped_timer.h"

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = PMP::parameters;

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
		SCOPED_PERFORMANCE_TIMER("Polyhedron -> PolySet");

		auto ps = make_shared<PolySet>(3, /* convex */ unknown, /* manifold */ false);
		auto err = CGALUtils::createPolySetFromPolyhedron(*poly, *ps);
		assert(!err);
		return ps;
	}
	else if (auto nef = getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("Nef -> PolySet");

		auto ps = make_shared<PolySet>(3, /* convex */ unknown, /* manifold */ nef->is_simple());
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

bool CGALHybridPolyhedron::needsNefForOperationWith(const CGALHybridPolyhedron &other) const
{
	return !isManifold() || !other.isManifold() || sharesAnyVertexWith(other);
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron &other)
{
	if (!intersects(other) && isManifold() && other.isManifold()) {
		polyBinOp("fast union", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			CGALUtils::copyPolyhedron(otherPoly, destinationPoly);
		});
	}
	else {
		if (needsNefForOperationWith(other))
			nefPolyBinOp("union", other,
									 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
										 destinationNef += otherNef;
									 });
		else
			polyBinOp("union", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
				PMP::corefine_and_compute_union(destinationPoly, otherPoly, destinationPoly);
			});
	}
	bboxes.insert(bboxes.end(), other.bboxes.begin(), other.bboxes.end());
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron &other)
{
	if (!intersects(other)) {
		LOG(message_group::Warning, Location::NONE, "", "Empty intersection");
		clear();
		return;
	}

	if (needsNefForOperationWith(other))
		nefPolyBinOp("intersection", other,
								 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
									 destinationNef *= otherNef;
								 });
	else
		polyBinOp("intersection", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			PMP::corefine_and_compute_intersection(destinationPoly, otherPoly, destinationPoly);
		});

	std::vector<bbox_t> new_bboxes;
	for (auto &bbox : bboxes)
		if (other.intersects(bbox)) new_bboxes.push_back(bbox);
	bboxes = new_bboxes;
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron &other)
{
	if (!intersects(other)) {
		LOG(message_group::Warning, Location::NONE, "", "Difference with non-intersecting geometry");
		return;
	}

	// Note: we don't need to change the bbox.
	if (needsNefForOperationWith(other))
		nefPolyBinOp("intersection", other,
								 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
									 destinationNef -= otherNef;
								 });
	else
		polyBinOp("difference", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			PMP::corefine_and_compute_difference(destinationPoly, otherPoly, destinationPoly);
		});
}

void CGALHybridPolyhedron::minkowski(CGALHybridPolyhedron &other)
{
	nefPolyBinOp("minkowski", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 destinationNef = CGAL::minkowski_sum_3(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::transform(const Transform3d &mat)
{
	if (mat.matrix().determinant() == 0) {
		LOG(message_group::Warning, Location::NONE, "", "Scaling a 3D object with 0 - removing object");
		clear();
	}
	else {
		auto t = CGALUtils::createAffineTransformFromMatrix<kernel_t>(mat);
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

bool CGALHybridPolyhedron::sharesAnyVertexWith(const CGALHybridPolyhedron &other) const
{
	if (other.numVertices() < numVertices()) {
		// The other has less vertices to index!
		return other.sharesAnyVertexWith(*this);
	}

	SCOPED_PERFORMANCE_TIMER("sharesAnyVertexWith");

	std::unordered_set<point_t> vertices;
	foreachVertexUntilTrue([&](const auto &p) {
		vertices.insert(p);
		return false;
	});

	auto foundCollision = false;
	other.foreachVertexUntilTrue(
			[&](const auto &p) { return foundCollision = vertices.find(p) != vertices.end(); });

	// printf("foundCollision: %s (%lu vertices)\n", foundCollision ? "yes" : "no", vertices.size());
	return foundCollision;
}

void CGALHybridPolyhedron::nefPolyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<void(nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef)>
				&operation)
{
	SCOPED_PERFORMANCE_TIMER(std::string("nef ") + opName);

	auto &destinationNef = convertToNefPolyhedron();
	auto &otherNef = other.convertToNefPolyhedron();

	auto manifoldBefore = isManifold() && other.isManifold();
	operation(destinationNef, otherNef);

	auto manifoldNow = isManifold();
	if (manifoldNow != manifoldBefore) {
		LOG(message_group::Echo, Location::NONE, "",
				"Nef operation got %1$s operands and produced a %2$s result",
				manifoldBefore ? "manifold" : "non-manifold", manifoldNow ? "manifold" : "non-manifold");
	}
}

void CGALHybridPolyhedron::polyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<void(polyhedron_t &destinationPoly, polyhedron_t &otherPoly)> &operation)
{
	SCOPED_PERFORMANCE_TIMER(std::string("polyhedron ") + opName);

	operation(convertToPolyhedron(), other.convertToPolyhedron());
}

CGALHybridPolyhedron::nef_polyhedron_t &CGALHybridPolyhedron::convertToNefPolyhedron()
{
	if (auto poly = getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("Polyhedron -> Nef");

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
		SCOPED_PERFORMANCE_TIMER("Nef -> Polyhedron");

		auto poly = make_shared<polyhedron_t>();
		nef->convert_to_polyhedron(*poly);
		data = poly;
		return *poly;
	}
	else {
		throw "Bad data state";
	}
}

#endif // FAST_CSG_AVAILABLE
