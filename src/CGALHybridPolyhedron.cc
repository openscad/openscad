// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include "cgalutils.h"
#include "hash.h"
#include "scoped_timer.h"
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/helpers.h>

#ifdef FAST_CSG_DEBUG_SERIALIZE_COREFINEMENT_OPERANDS
#include <sstream>
#include <fstream>
#endif

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
template <typename TriangleMesh>
void cleanupMesh(TriangleMesh &p, bool is_corefinement_result);

template <>
void cleanupMesh(CGALHybridPolyhedron::polyhedron_t &poly, bool is_corefinement_result)
{
	SCOPED_PERFORMANCE_TIMER("cleanup polyhedron");

#ifdef FAST_CSG_KERNEL_IS_LAZY
	// TODO(ochafik): Support ExperimentalFastCsgExactCallback for polyhedron?
	CGALHybridPolyhedron::polyhedron_t::Vertex_iterator vi;
	CGAL_forall_vertices(vi, poly)
	{
		auto &pt = vi->point();
		CGAL::exact(pt.x());
		CGAL::exact(pt.y());
		CGAL::exact(pt.z());
	}
#endif // FAST_CSG_KERNEL_IS_LAZY
}

template <>
void cleanupMesh(CGALHybridPolyhedron::mesh_t &mesh, bool is_corefinement_result)
{
	SCOPED_PERFORMANCE_TIMER("force exact numbers");

	mesh.collect_garbage();

#ifdef FAST_CSG_KERNEL_IS_LAZY
	// Coordinates of new vertices would have already been forced to exact in the
	// corefinement callbacks called whenever new faces are created.
	if (!Feature::ExperimentalFastCsgExactCallback.is_enabled() || !is_corefinement_result)
	{
		for (auto v : mesh.vertices()) {
			auto &pt = mesh.point(v);
			CGAL::exact(pt.x());
			CGAL::exact(pt.y());
			CGAL::exact(pt.z());
		}
	}
#endif // FAST_CSG_KERNEL_IS_LAZY
}

template <>
CGALHybridPolyhedron::nef_polyhedron_t &CGALHybridPolyhedron::convert()
{
	if (auto poly = getPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("polyhedron -> nef");

		auto nef = make_shared<nef_polyhedron_t>(*poly);
		data = nef;
		return *nef;
	}
	else if (auto mesh = getMesh()) {
		SCOPED_PERFORMANCE_TIMER("mesh -> nef");

		auto nef = make_shared<nef_polyhedron_t>(*mesh);
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

template <>
CGALHybridPolyhedron::polyhedron_t &CGALHybridPolyhedron::convert()
{
	if (auto poly = getPolyhedron()) {
		return *poly;
	}
	else if (auto mesh = getMesh()) {
		auto poly = make_shared<polyhedron_t>();
		CGAL::copy_face_graph(*mesh, *poly);
		data = poly;
		return *poly;
	}
	else if (auto nef = getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("nef -> polyhedron");

		auto poly = make_shared<polyhedron_t>();
		CGALUtils::convertNefToPolyhedron(*nef, *poly);
		cleanupMesh(*poly, /* is_corefinement_result */ false);
		data = poly;
		return *poly;
	}
	else {
		throw "Bad data state";
	}
}

template <>
CGALHybridPolyhedron::mesh_t &CGALHybridPolyhedron::convert()
{
	if (auto mesh = getMesh()) {
		return *mesh;
	}
	else if (auto poly = getPolyhedron()) {
		auto mesh = make_shared<mesh_t>();
		CGAL::copy_face_graph(*poly, *mesh);
		CGALUtils::triangulateFaces(*mesh);
		data = mesh;
		return *mesh;
	}
	else if (auto nef = getNefPolyhedron()) {
		SCOPED_PERFORMANCE_TIMER("nef -> polyhedron");

		auto mesh = make_shared<mesh_t>();
		CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, *mesh);
		cleanupMesh(*mesh, /* is_corefinement_result */ false);
		data = mesh;
		return *mesh;
	}
	else {
		throw "Bad data state";
	}
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t> &nef)
{
	assert(nef);
	data = nef;
	bboxes.push_back(CGALUtils::boundingBox(*nef));
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<mesh_t> &mesh)
{
	assert(mesh);
	data = mesh;
	bboxes.push_back(CGALUtils::boundingBox(*mesh));
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
	else if (auto mesh = other.getMesh()) {
		data = make_shared<mesh_t>(*mesh);
	}
	else {
		assert(!"Bad hybrid polyhedron state");
	}
}

CGALHybridPolyhedron::CGALHybridPolyhedron()
{
	if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
		data = make_shared<CGALHybridPolyhedron::mesh_t>();
	}
	else {
		data = make_shared<CGALHybridPolyhedron::polyhedron_t>();
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

CGALHybridPolyhedron::mesh_t *CGALHybridPolyhedron::getMesh() const
{
	auto pp = boost::get<shared_ptr<mesh_t>>(&data);
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
	else if (auto nef = getNefPolyhedron()) {
		return nef->number_of_facets();
	}
	else if (auto mesh = getMesh()) {
		return mesh->number_of_faces();
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
	else if (auto mesh = getMesh()) {
		return mesh->number_of_vertices();
	}
	assert(!"Bad hybrid polyhedron state");
	return 0;
}

bool CGALHybridPolyhedron::isManifold() const
{
	if (auto poly = getPolyhedron()) {
		return poly->is_closed();
	}
	else if (auto mesh = getMesh()) {
		// Note: haven't tried mesh->is_valid() but it could be too expensive.
		return CGAL::is_closed(*mesh);
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
	else if (auto mesh = getMesh()) {
		auto ps = make_shared<PolySet>(3, /* convex */ unknown);
		auto err = CGALUtils::createPolySetFromMesh(*mesh, *ps);
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
	if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
		data = make_shared<mesh_t>();
	}
	else {
		data = make_shared<polyhedron_t>();
	}
	bboxes.clear();
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron &other)
{
	auto bothManifold = isManifold() && other.isManifold();
	auto disjointUnion = bothManifold && Feature::ExperimentalFastCsgDisjointOpt.is_enabled() &&
											 !boundingBoxesIntersect(other);
	// Mesh doesn't play well with fast union yet, because `cube(1); translate([1, 1, 1]) cube(1);`
	// doesn't count as a intersection but means the two bodies share a vertex, which
	// makes it impossible to do a dumb concatenation (need to keep vertices unique
	// in a Surface_mesh).
	// TODO(ochafik): Needs more work.
	bboxes.insert(bboxes.end(), other.bboxes.begin(), other.bboxes.end());

	if (bothManifold) {
		if (disjointUnion) {
			if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
				polyBinOp<mesh_t>("fast mesh union", other, [&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
					CGALUtils::copyMesh(lhs, out);
					CGALUtils::copyMesh(rhs, out);
					return true;
				});
			}
			else {
				polyBinOp<polyhedron_t>("fast polyhedron union", other,
																[&](polyhedron_t &lhs, polyhedron_t &rhs, polyhedron_t &out) {
																	if (&lhs != &out) CGALUtils::copyPolyhedron(lhs, out);
																	if (&rhs != &out) CGALUtils::copyPolyhedron(rhs, out);
																	return true;
																});
			}
			return;
		}
		else {
			if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
				if (polyBinOp<mesh_t>("corefinement mesh union", other,
															[&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
																return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
															}))
					return;
			}
			else {
				if (polyBinOp<polyhedron_t>("corefinement polyhedron union", other,
																		[&](polyhedron_t &lhs, polyhedron_t &rhs, polyhedron_t &out) {
																			return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
																		}))
					return;
			}
		}
	}

	nefPolyBinOp("nef union", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron &other)
{
	if (Feature::ExperimentalFastCsgDisjointOpt.is_enabled()) {
		if (!boundingBoxesIntersect(other)) {
			LOG(message_group::Warning, Location::NONE, "", "[fast-csg] Empty intersection");
			clear();
			return;
		}

		std::vector<bbox_t> new_bboxes;
		for (auto &bbox : bboxes)
			if (other.boundingBoxesIntersect(bbox)) new_bboxes.push_back(bbox);
		bboxes = new_bboxes;
	}

	if (isManifold() && other.isManifold()) {
		if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
			if (polyBinOp<mesh_t>("corefinement mesh intersection", other,
														[&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
															return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
														}))
				return;
		}
		else {
			if (polyBinOp<polyhedron_t>("corefinement polyhedron intersection", other,
																	[&](polyhedron_t &lhs, polyhedron_t &rhs, polyhedron_t &out) {
																		return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
																	}))
				return;
		}
	}

	nefPolyBinOp("nef intersection", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron &other)
{
	if (Feature::ExperimentalFastCsgDisjointOpt.is_enabled() && !boundingBoxesIntersect(other)) {
		LOG(message_group::Warning, Location::NONE, "",
				"[fast-csg] Difference with non-intersecting geometry");
		return;
	}

	// Note: we don't need to change the bbox.

	if (isManifold() && other.isManifold()) {
		if (Feature::ExperimentalFastCsgMesh.is_enabled()) {
			if (polyBinOp<mesh_t>("corefinement mesh difference", other,
														[&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
															return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
														}))
				return;
		}
		else {
			if (polyBinOp<polyhedron_t>("corefinement polyhedron difference", other,
																	[&](polyhedron_t &lhs, polyhedron_t &rhs, polyhedron_t &out) {
																		return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
																	}))
				return;
		}
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
		for (auto &bbox : bboxes) bbox.transform(t);

		if (auto poly = getPolyhedron()) {
			SCOPED_PERFORMANCE_TIMER("polyhedron transform");
			CGALUtils::transform(*poly, mat);
			cleanupMesh(*poly, /* is_corefinement_result */ false);
		}
		else if (auto mesh = getMesh()) {
			SCOPED_PERFORMANCE_TIMER("mesh transform");
			CGALUtils::transform(*mesh, mat);
			cleanupMesh(*mesh, /* is_corefinement_result */ false);
		}
		else if (auto nef = getNefPolyhedron()) {
			SCOPED_PERFORMANCE_TIMER("nef transform");
			CGALUtils::transform(*nef, mat);
		}
		else {
			assert(!"Bad hybrid polyhedron state");
		}
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
	else if (auto mesh = getMesh()) {
		total += numFacets() * 3 * sizeof(size_t);
		total += numVertices() * sizeof(point_t);
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
	else if (auto mesh = getMesh()) {
		for (auto v : mesh->vertices()) {
			if (f(mesh->point(v))) return;
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

	operation(convert<nef_polyhedron_t>(), other.convert<nef_polyhedron_t>());
}

template <typename TriangleMesh>
bool CGALHybridPolyhedron::polyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<bool(TriangleMesh &lhs, TriangleMesh &rhs, TriangleMesh &out)> &operation)
{
	SCOPED_PERFORMANCE_TIMER(opName);

	LOG(message_group::Echo, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
			opName.c_str(), numFacets(), other.numFacets());

	auto previousData = data;
	auto previousOtherData = other.data;

	auto success = false;
#ifndef FAST_CSG_NO_INPLACE_UPDATES
	auto out = make_shared<TriangleMesh>();
#endif
	CGALUtils::CGALErrorBehaviour behaviour{CGAL::THROW_EXCEPTION};
	try {
		auto &lhs = convert<TriangleMesh>();
		auto &rhs = other.convert<TriangleMesh>();

#ifdef FAST_CSG_DEBUG_SERIALIZE_COREFINEMENT_OPERANDS
		static std::map<std::string, size_t> opCount;
		auto opNumber = opCount[opName]++;
		std::ofstream(opName + "_lhs.off") << lhs;
		std::ofstream(opName + "_rhs.off") << rhs;
#endif

#ifndef FAST_CSG_NO_INPLACE_UPDATES
		if ((success = operation(lhs, rhs, lhs))) {
			cleanupMesh(lhs, /* is_corefinement_result */ true);
		}
#else
		if ((success = operation(lhs, rhs, *out))) {
			cleanupMesh(*out, /* is_corefinement_result */ true);
			data = out;
		}
#endif
		else {
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

#endif // FAST_CSG_AVAILABLE
