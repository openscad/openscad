// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#if CGAL_VERSION_NR >= CGAL_VERSION_NUMBER(5, 1, 0)
#include <CGAL/Polygon_mesh_processing/manifoldness.h>
#else
#include <CGAL/Polygon_mesh_processing/repair.h>
#endif
#include <unordered_set>

#include "CGAL_Nef_polyhedron.h"
#include "cgalutils.h"
#include "feature.h"
#include "grid.h"
#include "hash.h"
#include "polyset.h"
#include "scoped_timer.h"

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = PMP::parameters;

CGALPolyhedron::CGALPolyhedron(const PolySet &ps)
{
	SCOPED_PERFORMANCE_TIMER("CGALPolyhedron(PolySet)");

	auto polyhedron = make_shared<polyhedron_t>();
	data = polyhedron;
	bboxes.add(CGALUtils::boundingBox(ps));

	auto err = CGALUtils::createPolyhedronFromPolySet(ps, *polyhedron);
	assert(!err);
	PMP::triangulate_faces(*polyhedron);

	if (!PMP::is_outward_oriented(*polyhedron, params::all_default())) {
		LOG(message_group::Warning, Location::NONE, "", "Fixing inverted faces orientation.");
		PMP::reverse_face_orientations(*polyhedron);
	}
	if (!ps.is_manifold() && !ps.is_convex()) {
		SCOPED_PERFORMANCE_TIMER("CGALPolyhedron::buildPolyhedron: manifoldness checks");

		if (PMP::duplicate_non_manifold_vertices(*polyhedron)) {
			LOG(message_group::Warning, Location::NONE, "",
					"Non-manifold, converting to nef polyhedron.");
			convertToNefPolyhedron();
		}
	}
}

CGALPolyhedron::CGALPolyhedron(const CGAL_Nef_polyhedron3 &nef)
{
	SCOPED_PERFORMANCE_TIMER("Polyhedron(CGAL_Nef_polyhedron3)");

	auto polyhedron = make_shared<polyhedron_t>();
	CGAL_Polyhedron poly;
	nef.convert_to_polyhedron(poly);
	CGALUtils::copyPolyhedron(poly, *polyhedron);
	data = polyhedron;
	bboxes.add(CGALUtils::boundingBox(nef));
}

CGALPolyhedron::nef_polyhedron_t *CGALPolyhedron::getNefPolyhedron() const
{
	auto pp = boost::get<shared_ptr<nef_polyhedron_t>>(&data);
	return pp ? pp->get() : nullptr;
}

CGALPolyhedron::polyhedron_t *CGALPolyhedron::getPolyhedron() const
{
	auto pp = boost::get<shared_ptr<polyhedron_t>>(&data);
	return pp ? pp->get() : nullptr;
}

bool CGALPolyhedron::isEmpty() const
{
	return numFacets() == 0;
}

size_t CGALPolyhedron::numFacets() const
{
	if (auto poly = getPolyhedron()) {
		return poly->size_of_facets();
	}
	if (auto nef = getNefPolyhedron()) {
		return nef->number_of_facets();
	}
	assert(!"Invalid Polyhedron.data state");
	return 0;
}

size_t CGALPolyhedron::numVertices() const
{
	if (auto poly = getPolyhedron()) {
		return poly->size_of_vertices();
	}
	else if (auto nef = getNefPolyhedron()) {
		return nef->number_of_vertices();
	}
	assert(!"Invalid Polyhedron.data state");
	return 0;
}

bool CGALPolyhedron::isManifold() const
{
	if (getPolyhedron()) {
		// We don't keep simple polyhedra if they're not manifold (see contructor)
		return true;
	}
	else if (auto nef = getNefPolyhedron()) {
		return nef->is_simple();
	}
	assert(!"Invalid Polyhedron.data state");
	return false;
}

shared_ptr<const PolySet> CGALPolyhedron::toPolySet() const
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

		// TODO(ochafik): Convert to CGAL_Nef_polyhedron to keep things exact.
		auto ps = make_shared<PolySet>(3, /* convex */ unknown, /* manifold */ nef->is_simple());
		auto err = CGALUtils::createPolySetFromNefPolyhedron3(*nef, *ps);
		assert(!err);
		return ps;
	}
	else {
		assert(!"Invalid Polyhedron.data state");
		return nullptr;
	}
}

shared_ptr<const CGAL_Nef_polyhedron> CGALPolyhedron::toNefPolyhedron() const
{
	if (auto poly = getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("Polyhedron -> Nef");

		CGAL_Polyhedron alien_poly;
		CGALUtils::copyPolyhedron(*poly, alien_poly);
		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
	}
	else if (auto nef = getNefPolyhedron()) {
		assert(nef);
		if (!nef) return nullptr;
		SCOPED_PERFORMANCE_TIMER("Nef -> Nef (through polyhedron)");

		polyhedron_t poly;
		nef->convert_to_polyhedron(poly);

		CGAL_Polyhedron alien_poly;
		CGALUtils::copyPolyhedron(poly, alien_poly);

		return make_shared<CGAL_Nef_polyhedron>(make_shared<CGAL_Nef_polyhedron3>(alien_poly));
	}
	else {
		assert(!"Invalid Polyhedron.data state");
		return nullptr;
	}
}

void CGALPolyhedron::clear()
{
	data = make_shared<polyhedron_t>();
	bboxes.clear();
}

bool CGALPolyhedron::needsNefForOperationWith(const CGALPolyhedron &other) const
{
	return !isManifold() || !other.isManifold() || sharesAnyVertexWith(other);
}

void CGALPolyhedron::operator+=(CGALPolyhedron &other)
{
	if (!bboxes.intersects(other.bboxes) && isManifold() && other.isManifold()) {
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
	bboxes += other.bboxes;
}

void CGALPolyhedron::operator*=(CGALPolyhedron &other)
{
	if (!bboxes.intersects(other.bboxes)) {
		printf("Empty intersection difference!\n");
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
	bboxes *= other.bboxes;
}

void CGALPolyhedron::operator-=(CGALPolyhedron &other)
{
	if (!bboxes.intersects(other.bboxes)) {
		printf("Non intersecting difference!\n");
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

void CGALPolyhedron::minkowski(CGALPolyhedron &other)
{
	nefPolyBinOp("minkowski", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 destinationNef = CGAL::minkowski_sum_3(destinationNef, otherNef);
							 });
}

void CGALPolyhedron::transform(const Transform3d &mat)
{
	if (auto poly = getPolyhedron()) {
		CGALUtils::transform(*poly, mat);
	}
	else if (auto nef = getNefPolyhedron()) {
		CGALUtils::transform(*nef, mat);
	}
	else {
		assert(!"Invalid Polyhedron.data state");
	}
}

void CGALPolyhedron::resize(const Vector3d &newsize, const Eigen::Matrix<bool, 3, 1> &autosize)
{
	if (this->isEmpty()) return;

	assert(!"TODO: implement CGALPolyhedron::resize");
}

std::string CGALPolyhedron::dump() const
{
	assert(!"TODO: implement CGALPolyhedron::dump!");
	return "?";
	// return OpenSCAD::dump_svg(toPolySet());
}

size_t CGALPolyhedron::memsize() const
{
	size_t total = sizeof(CGALPolyhedron);
	total += bboxes.memsize();
	if (auto poly = getPolyhedron()) {
		total += poly->bytes();
	}
	else if (auto nef = getNefPolyhedron()) {
		total += nef->bytes();
	}
	return total;
}

void CGALPolyhedron::foreachVertexUntilTrue(const std::function<bool(const point_t &pt)> &f) const
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
		assert(!"Invalid Polyhedron.data state");
	}
}

bool CGALPolyhedron::sharesAnyVertexWith(const CGALPolyhedron &other) const
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

void CGALPolyhedron::nefPolyBinOp(const std::string &opName, CGALPolyhedron &other,
																	const std::function<void(nef_polyhedron_t &destinationNef,
																													 nef_polyhedron_t &otherNef)> &operation)
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

void CGALPolyhedron::polyBinOp(
		const std::string &opName, CGALPolyhedron &other,
		const std::function<void(polyhedron_t &destinationPoly, polyhedron_t &otherPoly)> &operation)
{
	SCOPED_PERFORMANCE_TIMER(std::string("polyhedron ") + opName);

	operation(convertToPolyhedron(), other.convertToPolyhedron());
}

CGALPolyhedron::nef_polyhedron_t &CGALPolyhedron::convertToNefPolyhedron()
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

CGALPolyhedron::polyhedron_t &CGALPolyhedron::convertToPolyhedron()
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

std::shared_ptr<CGALPolyhedron> CGALPolyhedron::fromGeometry(const Geometry &geom)
{
	if (auto poly = dynamic_cast<const CGALPolyhedron *>(&geom)) {
		return make_shared<CGALPolyhedron>(*poly);
	}
	else if (auto ps = dynamic_cast<const PolySet *>(&geom)) {
		return make_shared<CGALPolyhedron>(*ps);
	}
	else if (auto nef = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
		assert(nef->p3);
		return nef->p3 ? make_shared<CGALPolyhedron>(*nef->p3) : nullptr;
	}
	else {
		LOG(message_group::Warning, Location::NONE, "", "Unsupported geometry format.");
		return nullptr;
	}
}

#endif // FAST_CSG_AVAILABLE
