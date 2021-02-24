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
void cleanupMesh(CGALHybridPolyhedron::mesh_t &mesh, bool is_corefinement_result)
{
	SCOPED_PERFORMANCE_TIMER("force exact numbers");

	mesh.collect_garbage();

#ifdef FAST_CSG_KERNEL_IS_LAZY
	// Coordinates of new vertices would have already been forced to exact in the
	// corefinement callbacks called whenever new faces are created.
	if (!Feature::ExperimentalFastCsgExactCallback.is_enabled() || !is_corefinement_result) {
		for (auto v : mesh.vertices()) {
			auto &pt = mesh.point(v);
			CGAL::exact(pt.x());
			CGAL::exact(pt.y());
			CGAL::exact(pt.z());
		}
	}
#endif // FAST_CSG_KERNEL_IS_LAZY
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t> &nef)
{
	assert(nef);
	data = nef;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<mesh_t> &mesh)
{
	assert(mesh);
	data = mesh;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const CGALHybridPolyhedron &other)
{
	if (auto nef = other.getNefPolyhedron()) {
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
	}
	else if (auto mesh = getMesh()) {
		return mesh->number_of_faces();
	}
	assert(!"Bad hybrid polyhedron state");
	return 0;
}

size_t CGALHybridPolyhedron::numVertices() const
{
	if (auto nef = getNefPolyhedron()) {
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
	if (auto mesh = getMesh()) {
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
	if (auto mesh = getMesh()) {
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
	data = make_shared<mesh_t>();
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron &other)
{
	if (isManifold() && other.isManifold()) {
		if (polyBinOp("corefinement mesh union", other, [&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
					return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
				}))
			return;
	}

	nefPolyBinOp("nef union", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron &other)
{
	if (isManifold() && other.isManifold()) {
		if (polyBinOp("corefinement mesh intersection", other,
									[&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
										return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
									}))
			return;
	}

	nefPolyBinOp("nef intersection", other,
							 [&](nef_polyhedron_t &destinationNef, nef_polyhedron_t &otherNef) {
								 CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
							 });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron &other)
{
	if (isManifold() && other.isManifold()) {
		if (polyBinOp("corefinement mesh difference", other,
									[&](mesh_t &lhs, mesh_t &rhs, mesh_t &out) {
										return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
									}))
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

		if (auto mesh = getMesh()) {
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

CGALHybridPolyhedron::bbox_t CGALHybridPolyhedron::getExactBoundingBox() const
{
	bbox_t result(0, 0, 0, 0, 0, 0);
	std::vector<point_t> points;
  // TODO(ochafik): Optimize this!
  foreachVertexUntilTrue([&](const auto &pt) {
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
	}
	else if (auto nef = getNefPolyhedron()) {
		total += nef->bytes();
	}
	return total;
}

void CGALHybridPolyhedron::foreachVertexUntilTrue(
		const std::function<bool(const point_t &pt)> &f) const
{
	if (auto mesh = getMesh()) {
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

	operation(convertToNef(), other.convertToNef());
}

bool CGALHybridPolyhedron::polyBinOp(
		const std::string &opName, CGALHybridPolyhedron &other,
		const std::function<bool(mesh_t &lhs, mesh_t &rhs, mesh_t &out)> &operation)
{
	SCOPED_PERFORMANCE_TIMER(opName);

	LOG(message_group::Echo, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
			opName.c_str(), numFacets(), other.numFacets());

	auto previousData = data;
	auto previousOtherData = other.data;

	auto success = false;
	CGALUtils::CGALErrorBehaviour behaviour{CGAL::THROW_EXCEPTION};
	try {
		auto &lhs = convertToMesh();
		auto &rhs = other.convertToMesh();

#ifdef FAST_CSG_DEBUG_SERIALIZE_COREFINEMENT_OPERANDS
		static std::map<std::string, size_t> opCount;
		auto opNumber = opCount[opName]++;
		std::ofstream(opName + "_lhs.off") << lhs;
		std::ofstream(opName + "_rhs.off") << rhs;
#endif

		if ((success = operation(lhs, rhs, lhs))) {
			cleanupMesh(lhs, /* is_corefinement_result */ true);
		}
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

CGALHybridPolyhedron::nef_polyhedron_t &CGALHybridPolyhedron::convertToNef()
{
	if (auto mesh = getMesh()) {
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

CGALHybridPolyhedron::mesh_t &CGALHybridPolyhedron::convertToMesh()
{
	if (auto mesh = getMesh()) {
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

#endif // FAST_CSG_AVAILABLE
