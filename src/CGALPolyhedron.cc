// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALPolyhedron.h"
#ifdef FAST_POLYHEDRON_AVAILABLE
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/manifoldness.h>
#include <unordered_set>

#include "feature.h"
#include "hash.h"
#include "polyset.h"
#include "cgalutils.h"
#include "scoped_timer.h"

namespace PMP = CGAL::Polygon_mesh_processing;
namespace params = PMP::parameters;

CGALPolyhedron::CGALPolyhedron(const PolySet &ps)
{
	SCOPED_PERFORMANCE_TIMER("Polyhedron(PolySet)");

	auto polyhedron = make_shared<polyhedron_t>();
	auto err = CGALUtils::createPolyhedronFromPolySet(ps, *polyhedron);
	assert(!err && "createPolyhedronFromPolySet failed in CGALPolyhedron ctor");
	data = polyhedron;
	bboxes.add(CGALUtils::boundingBox(ps));

	{
		SCOPED_PERFORMANCE_TIMER("Polyhedron(): triangulate_faces");
		PMP::triangulate_faces(*polyhedron);
	}

	{
		SCOPED_PERFORMANCE_TIMER("Polyhedron(): orientation switch");
		if (!PMP::is_outward_oriented(*polyhedron, params::all_default())) {
			LOG(message_group::Warning, Location::NONE, "",
					"This polyhedron's %1$lu faces were oriented inwards and need inversion.",
					polyhedron->size_of_facets());
			PMP::reverse_face_orientations(*polyhedron);
		}
	}

	if (!ps.is_manifold() && !ps.is_convex() && !Feature::ExperimentalTrustManifold.is_enabled()) {
		LOG(message_group::Echo, Location::NONE, "",
				"Checking this polyhedron is manifold. Use --enable=trust-manifold to skip and speed up.");

		size_t duplicatedNonManifoldVertexCount = 0;
		{
			SCOPED_PERFORMANCE_TIMER("Polyhedron(): manifoldness checks");
			duplicatedNonManifoldVertexCount = PMP::duplicate_non_manifold_vertices(*polyhedron);
		}

		if (duplicatedNonManifoldVertexCount) {
			LOG(message_group::Warning, Location::NONE, "",
					"PolySet had %1$lu non-manifold vertices. Converting CGALPolyhedron to nef polyhedron.",
					duplicatedNonManifoldVertexCount);
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
	if (auto pNef = boost::get<shared_ptr<nef_polyhedron_t>>(&data)) {
		return pNef->get();
	}
	return nullptr;
}

CGALPolyhedron::polyhedron_t *CGALPolyhedron::getPolyhedron() const
{
	if (auto pPoly = boost::get<shared_ptr<polyhedron_t>>(&data)) {
		return pPoly->get();
	}
	return nullptr;
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
	return false;
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
	return false;
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

shared_ptr<const Geometry> CGALPolyhedron::toGeometry() const
{
	if (auto poly = getPolyhedron()) {
		auto ps = make_shared<PolySet>(3, /* convex */ unknown, /* manifold */ false);
		auto err = CGALUtils::createPolySetFromPolyhedron(*poly, *ps);
		assert(!err);
		return ps;
	}
	else if (auto nef = getNefPolyhedron()) {
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

void CGALPolyhedron::clear()
{
	data = make_shared<polyhedron_t>();
	bboxes.clear();
}

void CGALPolyhedron::operator+=(CGALPolyhedron &other)
{
	if (!bboxes.intersects(other.bboxes)) {
		polyBinOp("fast union", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			CGALUtils::appendToPolyhedron(otherPoly, destinationPoly);
		});
	}
	else {
		if (!isManifold() || !other.isManifold() || sharesAnyVertexWith(other)) {
			nefPolyBinOp("union", other,
									 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
										 destinationNef += otherNef;
									 });
		}
		else {
			polyBinOp("union", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
				PMP::corefine_and_compute_union(destinationPoly, otherPoly, destinationPoly);
			});
		}
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

	if (!isManifold() || !other.isManifold() || sharesAnyVertexWith(other)) {
		nefPolyBinOp("intersection", other,
								 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
									 destinationNef *= otherNef;
								 });
	}
	else {
		polyBinOp("intersection", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			PMP::corefine_and_compute_intersection(destinationPoly, otherPoly, destinationPoly);
		});
	}
	bboxes *= other.bboxes;
}

void CGALPolyhedron::operator-=(CGALPolyhedron &other)
{
	if (!bboxes.intersects(other.bboxes)) {
		printf("Non intersecting difference!\n");
		return;
	}

	// Note: we don't need to change the bbox.
	if (!isManifold() || !other.isManifold() || sharesAnyVertexWith(other)) {
		nefPolyBinOp("intersection", other,
								 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
									 destinationNef -= otherNef;
								 });
	}
	else {
		polyBinOp("difference", other, [&](polyhedron_t &destinationPoly, polyhedron_t &otherPoly) {
			PMP::corefine_and_compute_difference(destinationPoly, otherPoly, destinationPoly);
		});
	}
}

void CGALPolyhedron::minkowski(CGALPolyhedron &other)
{
	nefPolyBinOp("minkowski", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 destinationNef = CGAL::minkowski_sum_3(destinationNef, otherNef);
							 });
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

	std::unordered_set<polyhedron_t::Point_3> vertices;
	foreachVertexUntilTrue([&](const auto &p) {
		vertices.insert(p);
		return false;
	});

	auto foundCollision = false;
	other.foreachVertexUntilTrue(
			[&](const auto &p) { return foundCollision = vertices.find(p) != vertices.end(); });

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
	SCOPED_PERFORMANCE_TIMER(std::string("mesh ") + opName);

	auto &destinationPoly = convertToPolyhedron();
	auto &otherPoly = other.convertToPolyhedron();

	operation(destinationPoly, otherPoly);
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

#endif // FAST_POLYHEDRON_AVAILABLE
