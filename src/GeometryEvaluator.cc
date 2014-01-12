#include "GeometryEvaluator.h"
#include "traverser.h"
#include "Tree.h"
#include "GeometryCache.h"
#include "CGALCache.h"
#include "Polygon2d.h"
#include "module.h"
#include "state.h"
#include "transformnode.h"
#include "linearextrudenode.h"
#include "rotateextrudenode.h"
#include "csgnode.h"
#include "cgaladvnode.h"
#include "projectionnode.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgalutils.h"
#include "rendernode.h"
#include "clipper-utils.h"
#include "polyset-utils.h"
#include "polyset.h"
#include "calc.h"
#include "printutils.h"
#include "svg.h"
#include "dxfdata.h"

#include <algorithm>
#include <boost/foreach.hpp>

#include <CGAL/convex_hull_2.h>
#include <CGAL/Point_2.h>

GeometryEvaluator::GeometryEvaluator(const class Tree &tree):
	tree(tree)
{
}

bool GeometryEvaluator::isCached(const AbstractNode &node) const
{
	return GeometryCache::instance()->contains(this->tree.getIdString(node));
}

/*!
	Set allownef to false to force the result to _not_ be a Nef polyhedron
*/
shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode &node, 
																															 bool allownef)
{
	if (!isCached(node)) {
		shared_ptr<const CGAL_Nef_polyhedron> N;
		if (CGALCache::instance()->contains(this->tree.getIdString(node))) {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}

		// If not found in any caches, we need to evaluate the geometry
		if (N) {
			this->root = N;
		}	
    else {
			Traverser trav(*this, node, Traverser::PRE_AND_POSTFIX);
			trav.execute();
		}

		if (!allownef) {
			if (shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(this->root)) {
				this->root.reset(N->convertToPolyset());
				smartCache(node, this->root);
			}
		}
		return this->root;
	}
	return GeometryCache::instance()->get(this->tree.getIdString(node));
}

GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode &node, OpenSCADOperator op)
{
	unsigned int dim = 0;
	BOOST_FOREACH(const Geometry::ChildItem &item, this->visitedchildren[node.index()]) {
		if (item.second) {
			if (!dim) dim = item.second->getDimension();
			else if (dim != item.second->getDimension()) {
				PRINT("WARNING: Mixing 2D and 3D objects is not supported.");
				break;
			}
		}
	}
	if (dim == 2) return ResultObject(applyToChildren2D(node, op));
	else if (dim == 3) return applyToChildren3D(node, op);
	return ResultObject();
}

/*!
	Applies the operator to all child nodes of the given node.
	
	May return NULL or any 3D Geometry object (can be either PolySet or CGAL_Nef_polyhedron)
*/
GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren3D(const AbstractNode &node, OpenSCADOperator op)
{
	if (op == OPENSCAD_HULL) {
		return ResultObject(applyHull3D(node));
	}

	Geometry::ChildList children = collectChildren3D(node);

	if (children.size() == 0) return ResultObject();
	// Only one child -> this is a noop
	if (children.size() == 1) return ResultObject(children.front().second);

	CGAL_Nef_polyhedron *N = NULL;
	BOOST_FOREACH(const Geometry::ChildItem &item, children) {
		const shared_ptr<const Geometry> &chgeom = item.second;
		shared_ptr<const CGAL_Nef_polyhedron> chN;
		if (!chgeom) {
			chN.reset(new CGAL_Nef_polyhedron); // Create null polyhedron
		}
		else {
			chN = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
			if (!chN) {
				const PolySet *chps = dynamic_cast<const PolySet*>(chgeom.get());
				if (chps) chN.reset(createNefPolyhedronFromGeometry(*chps));
			}
		}

		if (N) CGALUtils::applyBinaryOperator(*N, *chN, op);
		// Initialize N on first iteration with first expected geometric object
		else if (chN) N = chN->copy();

		item.first->progress_report();
	}
	return ResultObject(N);
}


Polygon2d *GeometryEvaluator::applyHull2D(const AbstractNode &node)
{
	std::vector<const Polygon2d *> children = collectChildren2D(node);
	Polygon2d *geometry = NULL;

	typedef CGAL::Point_2<CGAL::Cartesian<double> > CGALPoint2;
	// Collect point cloud
	std::list<CGALPoint2> points;
	BOOST_FOREACH(const Polygon2d *p, children) {
		BOOST_FOREACH(const Outline2d &o, p->outlines()) {
			BOOST_FOREACH(const Vector2d &v, o.vertices) {
				points.push_back(CGALPoint2(v[0], v[1]));
			}
		}
	}
	if (points.size() > 0) {
		// Apply hull
		std::list<CGALPoint2> result;
		CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter(result));

		// Construct Polygon2d
		Outline2d outline;
		BOOST_FOREACH(const CGALPoint2 &p, result) {
			outline.vertices.push_back(Vector2d(p[0], p[1]));
		}
		geometry = new Polygon2d();
		geometry->addOutline(outline);
	}
	return geometry;
}

Geometry *GeometryEvaluator::applyHull3D(const AbstractNode &node)
{
	Geometry::ChildList children = collectChildren3D(node);

	CGAL_Polyhedron P;
	if (CGALUtils::applyHull(children, P)) {
		return new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(P));
	}
	return NULL;
}

void GeometryEvaluator::applyResize3D(CGAL_Nef_polyhedron &N, 
																			const Vector3d &newsize,
																			const Eigen::Matrix<bool,3,1> &autosize)
{
	// Based on resize() in Giles Bathgate's RapCAD (but not exactly)
	if (N.isEmpty()) return;

	CGAL_Iso_cuboid_3 bb = CGALUtils::boundingBox(*N.p3);

	std::vector<NT3> scale, bbox_size;
	for (int i=0;i<3;i++) {
		scale.push_back(NT3(1));
		bbox_size.push_back(bb.max_coord(i) - bb.min_coord(i));
	}
	int newsizemax_index = 0;
	for (int i=0;i<N.getDimension();i++) {
		if (newsize[i]) {
			if (bbox_size[i] == NT3(0)) {
				PRINT("WARNING: Resize in direction normal to flat object is not implemented");
				return;
			}
			else {
				scale[i] = NT3(newsize[i]) / bbox_size[i];
			}
			if (newsize[i] > newsize[newsizemax_index]) newsizemax_index = i;
		}
	}

	NT3 autoscale = NT3(1);
	if (newsize[newsizemax_index] != 0) {
		autoscale = NT3(newsize[newsizemax_index]) / bbox_size[newsizemax_index];
	}
	for (int i=0;i<N.getDimension();i++) {
		if (autosize[i] && newsize[i]==0) scale[i] = autoscale;
	}

	Eigen::Matrix4d t;
	t << CGAL::to_double(scale[0]),           0,        0,        0,
	     0,        CGAL::to_double(scale[1]),           0,        0,
	     0,        0,        CGAL::to_double(scale[2]),           0,
	     0,        0,        0,                                   1;

	N.transform(Transform3d(t));
	return;
}

// Helper functions for GeometryEvaluator::applyMinkowski2D()
namespace {
	void transform_path(ClipperLib::Path & path, ClipperLib::IntPoint delta) {
		BOOST_FOREACH(ClipperLib::IntPoint & point, path) {
			point.X += delta.X;
			point.Y += delta.Y;
		}
	}

	void transform_paths(ClipperLib::Paths & paths, ClipperLib::IntPoint delta) {
		BOOST_FOREACH(ClipperLib::Path & path, paths) {
			transform_path(path, delta);
		}
	}

	// Add the polygon a translated to an arbitrary point of each separate component of b
	void fill_minkowski_insides(ClipperLib::Paths const& a,
								ClipperLib::Paths const& b,
								std::vector<ClipperLib::Paths> & target) {
		// (or easier: one arbitrary point on each positive contour)
		BOOST_FOREACH (ClipperLib::Path const& b_path, b) {
			if (!b_path.empty() && ClipperLib::Orientation(b_path) == 1) {
				target.push_back(a);
				transform_paths(target.back(), b_path[0] /* arbitrary */);
			}
		}
	}
}

Polygon2d *GeometryEvaluator::applyMinkowski2D(const AbstractNode &node)
{
	std::vector<const Polygon2d *> children = collectChildren2D(node);
	if (!children.empty()) {
		ClipperLib::Paths lhs = ClipperUtils::fromPolygon2d(*children[0]);

		for (size_t i=1; i<children.size(); i++) {
			ClipperLib::Paths rhs = ClipperUtils::fromPolygon2d(*children[i]);

			std::vector<ClipperLib::Paths> minkowski_terms;

			// First, convolve each outline of lhs with the outlines of rhs
			BOOST_FOREACH(ClipperLib::Path const& rhs_path, rhs) {
				BOOST_FOREACH(ClipperLib::Path const& lhs_path, lhs) {
					ClipperLib::Paths result;
					ClipperLib::MinkowskiSum(lhs_path, rhs_path, result, true);
					minkowski_terms.push_back(result);
				}
			}

			// Then, fill the central parts
			fill_minkowski_insides(lhs, rhs, minkowski_terms);
			fill_minkowski_insides(rhs, lhs, minkowski_terms);

			// Finally, merge the Minkowski terms
			Polygon2d *p = ClipperUtils::apply(minkowski_terms, ClipperLib::ctUnion);
			lhs = ClipperUtils::fromPolygon2d(*p);
			delete p;
		}
		return ClipperUtils::toPolygon2d(ClipperUtils::sanitize(lhs));
	}
	return NULL;
}

/*!
	Returns a list of Polygon2d children of the given node.
	May return empty Polygon2d object, but not NULL objects
*/
std::vector<const class Polygon2d *> GeometryEvaluator::collectChildren2D(const AbstractNode &node)
{
	std::vector<const Polygon2d *> children;
	BOOST_FOREACH(const Geometry::ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const shared_ptr<const Geometry> &chgeom = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;

		// NB! We insert into the cache here to ensure that all children of
		// a node is a valid object. If we inserted as we created them, the 
		// cache could have been modified before we reach this point due to a large
		// sibling object. 
		smartCache(*chnode, chgeom);
		
		if (chgeom) {
			if (chgeom->getDimension() == 2) {
				const Polygon2d *polygons = dynamic_cast<const Polygon2d *>(chgeom.get());
				assert(polygons);
				children.push_back(polygons);
			}
			else {
				PRINT("WARNING: Ignoring 3D child object for 2D operation");
			}
		}
	}
	return children;
}

/*!
	Since we can generate both Nef and non-Nef geometry, we need to insert it into
	the appropriate cache.
	This method inserts the geometry into the appropriate cache if it's not already cached.
*/
void GeometryEvaluator::smartCache(const AbstractNode &node, 
																	 const shared_ptr<const Geometry> &geom)
{
	shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
	if (N) {
		if (!CGALCache::instance()->contains(this->tree.getIdString(node))) {
			CGALCache::instance()->insert(this->tree.getIdString(node), N);
		}
	}
	else {
		if (!isCached(node)) {
			if (!GeometryCache::instance()->insert(this->tree.getIdString(node), geom)) {
				PRINT("WARNING: GeometryEvaluator: Root node didn't fit into cache");
			}
		}
	}
}

/*!
	Returns a list of 3D Geometry children of the given node.
	May return empty geometries, but not NULL objects
*/
Geometry::ChildList GeometryEvaluator::collectChildren3D(const AbstractNode &node)
{
	Geometry::ChildList children;
	BOOST_FOREACH(const Geometry::ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const shared_ptr<const Geometry> &chgeom = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;

		// NB! We insert into the cache here to ensure that all children of
		// a node is a valid object. If we inserted as we created them, the 
		// cache could have been modified before we reach this point due to a large
		// sibling object. 
		smartCache(*chnode, chgeom);
		
		if (chgeom) {
			if (chgeom->isEmpty() || chgeom->getDimension() == 3) {
				children.push_back(item);
			}
			else {
				PRINT("WARNING: Ignoring 2D child object for 3D operation");
			}
		}
	}
	return children;
}

/*!
	
*/
Polygon2d *GeometryEvaluator::applyToChildren2D(const AbstractNode &node, OpenSCADOperator op)
{
	if (op == OPENSCAD_MINKOWSKI) {
		return applyMinkowski2D(node);
	}
	else if (op == OPENSCAD_HULL) {
		return applyHull2D(node);
	}

	std::vector<const Polygon2d *> children = collectChildren2D(node);

	ClipperLib::ClipType clipType;
	switch (op) {
	case OPENSCAD_UNION:
		clipType = ClipperLib::ctUnion;
		break;
	case OPENSCAD_INTERSECTION:
		clipType = ClipperLib::ctIntersection;
		break;
	case OPENSCAD_DIFFERENCE:
		clipType = ClipperLib::ctDifference;
		break;
	default:
		PRINTB("Error: Unknown boolean operation %d", int(op));
		return NULL;
		break;
	}

	return ClipperUtils::apply(children, clipType);
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during traversal.
	Usually, this should be called from the postfix stage, but for some nodes, 
	we defer traversal letting other components (e.g. CGAL) render the subgraph, 
	and we'll then call this from prefix and prune further traversal.

	The added geometry can be NULL if it wasn't possible to evaluate it.
*/
void GeometryEvaluator::addToParent(const State &state, 
																		const AbstractNode &node, 
																		const shared_ptr<const Geometry> &geom)
{
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, geom));
	}
	else {
		// Root node, insert into cache
		smartCache(node, geom);
		this->root = geom;
	}
}

/*!
   Custom nodes, as well as RenderNode are handled here => implicit union
*/
Response GeometryEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {
			geom = applyToChildren(node, OPENSCAD_UNION).constptr();
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

/*!
   RenderNodes just pass on convexity
*/
Response GeometryEvaluator::visit(State &state, const RenderNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {
			ResultObject res = applyToChildren(node, OPENSCAD_UNION);

			geom = res.constptr();
			if (shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom)) {
				// If we got a const object, make a copy
				shared_ptr<PolySet> newps;
				if (res.isConst()) newps.reset(new PolySet(*ps));
				else newps = dynamic_pointer_cast<PolySet>(res.ptr());
				newps->setConvexity(node.convexity);
				geom = newps;
			}
			else if (shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
				// If we got a const object, make a copy
				shared_ptr<CGAL_Nef_polyhedron> newN;
				if (res.isConst()) newN.reset(new CGAL_Nef_polyhedron(*N));
				else newN = dynamic_pointer_cast<CGAL_Nef_polyhedron>(res.ptr());
				newN->setConvexity(node.convexity);
				geom = newN;
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

/*!
	Leaf nodes can create their own geometry, so let them do that

	input: None
	output: PolySet or Polygon2d
*/
Response GeometryEvaluator::visit(State &state, const LeafNode &node)
{
	if (state.isPrefix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) {
			const Geometry *geometry = node.createGeometry();
			if (const Polygon2d *polygon = dynamic_cast<const Polygon2d*>(geometry)) {
				if (!polygon->isSanitized()) {
					Polygon2d *p = ClipperUtils::sanitize(*polygon);
					delete geometry;
					geometry = p;
				}
			}
            geom.reset(geometry);
		}
		else geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, geom);
	}
	return PruneTraversal;
}

/*!
	input: List of 2D or 3D objects (not mixed)
	output: Polygon2d or 3D PolySet
	operation:
	  o Perform csg op on children
 */			
Response GeometryEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) {
			geom = applyToChildren(node, node.type).constptr();
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

/*!
	input: List of 2D or 3D objects (not mixed)
	output: Polygon2d or 3D PolySet
	operation:
	  o Union all children
	  o Perform transform
 */			
Response GeometryEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {
			if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
				// due to the way parse/eval works we can't currently distinguish between NaN and Inf
				PRINT("Warning: Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
			}
			else {
				// First union all children
				ResultObject res = applyToChildren(node, OPENSCAD_UNION);
				if ((geom = res.constptr())) {
					if (geom->getDimension() == 2) {
						shared_ptr<const Polygon2d> polygons = dynamic_pointer_cast<const Polygon2d>(geom);
						assert(polygons);
						
						// If we got a const object, make a copy
						shared_ptr<Polygon2d> newpoly;
						if (res.isConst()) newpoly.reset(new Polygon2d(*polygons));
						else newpoly = dynamic_pointer_cast<Polygon2d>(res.ptr());
						
						Transform2d mat2;
						mat2.matrix() << 
							node.matrix(0,0), node.matrix(0,1), node.matrix(0,3),
							node.matrix(1,0), node.matrix(1,1), node.matrix(1,3),
							node.matrix(3,0), node.matrix(3,1), node.matrix(3,3);
						newpoly->transform(mat2);
						geom = newpoly;
					}
					else if (geom->getDimension() == 3) {
						shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);
						if (ps) {
							// If we got a const object, make a copy
							shared_ptr<PolySet> newps;
							if (res.isConst()) newps.reset(new PolySet(*ps));
							else newps = dynamic_pointer_cast<PolySet>(res.ptr());
							newps->transform(node.matrix);
							geom = newps;
						}
						else {
							shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
							assert(N);
							// If we got a const object, make a copy
							shared_ptr<CGAL_Nef_polyhedron> newN;
							if (res.isConst()) newN.reset(new CGAL_Nef_polyhedron(*N));
							else newN = dynamic_pointer_cast<CGAL_Nef_polyhedron>(res.ptr());
							newN->transform(node.matrix);
							geom = newN;
						}
					}
				}
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

static void translate_PolySet(PolySet &ps, const Vector3d &translation)
{
	BOOST_FOREACH(PolySet::Polygon &p, ps.polygons) {
		BOOST_FOREACH(Vector3d &v, p) {
			v += translation;
		}
	}
}

static void add_slice(PolySet *ps, const Polygon2d &poly, 
											double rot1, double rot2, 
											double h1, double h2, 
											const Vector2d &scale1,
											const Vector2d &scale2)
{
	Eigen::Affine2d trans1(Eigen::Scaling(scale1) * Eigen::Rotation2D<double>(-rot1*M_PI/180));
	Eigen::Affine2d trans2(Eigen::Scaling(scale2) * Eigen::Rotation2D<double>(-rot2*M_PI/180));
	
	// FIXME: If scale2 == 0 we need to handle tessellation separately
	bool splitfirst = sin(rot1 - rot2) >= 0.0;
	BOOST_FOREACH(const Outline2d &o, poly.outlines()) {
		Vector2d prev1 = trans1 * o.vertices[0];
		Vector2d prev2 = trans2 * o.vertices[0];
		for (size_t i=1;i<=o.vertices.size();i++) {
			Vector2d curr1 = trans1 * o.vertices[i % o.vertices.size()];
			Vector2d curr2 = trans2 * o.vertices[i % o.vertices.size()];
			ps->append_poly();
			
			// Make sure to split negative outlines correctly
			if (splitfirst xor !o.positive) {
				ps->insert_vertex(prev1[0], prev1[1], h1);
				ps->insert_vertex(curr2[0], curr2[1], h2);
				ps->insert_vertex(curr1[0], curr1[1], h1);
				if (scale2[0] > 0 || scale2[1] > 0) {
					ps->append_poly();
					ps->insert_vertex(curr2[0], curr2[1], h2);
					ps->insert_vertex(prev1[0], prev1[1], h1);
					ps->insert_vertex(prev2[0], prev2[1], h2);
				}
			}
			else {
				ps->insert_vertex(prev1[0], prev1[1], h1);
				ps->insert_vertex(prev2[0], prev2[1], h2);
				ps->insert_vertex(curr1[0], curr1[1], h1);
				if (scale2[0] > 0 || scale2[1] > 0) {
					ps->append_poly();
					ps->insert_vertex(prev2[0], prev2[1], h2);
					ps->insert_vertex(curr2[0], curr2[1], h2);
					ps->insert_vertex(curr1[0], curr1[1], h1);
				}
			}
			prev1 = curr1;
			prev2 = curr2;
		}
	}
}

/*!
	Input to extrude should be sanitized. This means non-intersecting, correct winding order
	etc., the input coming from a library like Clipper.
*/
static Geometry *extrudePolygon(const LinearExtrudeNode &node, const Polygon2d &poly)
{
	PolySet *ps = new PolySet(3);
	ps->setConvexity(node.convexity);
	if (node.height <= 0) return ps;

	double h1, h2;

	if (node.center) {
		h1 = -node.height/2.0;
		h2 = +node.height/2.0;
	} else {
		h1 = 0;
		h2 = node.height;
	}

	PolySet *ps_bottom = poly.tessellate(); // bottom
	
	// Flip vertex ordering for bottom polygon
	BOOST_FOREACH(PolySet::Polygon &p, ps_bottom->polygons) {
		std::reverse(p.begin(), p.end());
	}
	translate_PolySet(*ps_bottom, Vector3d(0,0,h1));

	ps->append(*ps_bottom);
	delete ps_bottom;
	if (node.scale_x > 0 || node.scale_y > 0) {
		Polygon2d top_poly(poly);
		Eigen::Affine2d trans(Eigen::Scaling(node.scale_x, node.scale_y) *
													 Eigen::Rotation2D<double>(-node.twist*M_PI/180));
		top_poly.transform(trans); // top
		PolySet *ps_top = top_poly.tessellate();
		translate_PolySet(*ps_top, Vector3d(0,0,h2));
		ps->append(*ps_top);
		delete ps_top;
	}
    size_t slices = node.has_twist ? node.slices : 1;

	for (int j = 0; j < slices; j++) {
		double rot1 = node.twist*j / slices;
		double rot2 = node.twist*(j+1) / slices;
		double height1 = h1 + (h2-h1)*j / slices;
		double height2 = h1 + (h2-h1)*(j+1) / slices;
		Vector2d scale1(1 - (1-node.scale_x)*j / slices,
										1 - (1-node.scale_y)*j / slices);
		Vector2d scale2(1 - (1-node.scale_x)*(j+1) / slices,
										1 - (1-node.scale_y)*(j+1) / slices);
		add_slice(ps, poly, rot1, rot2, height1, height2, scale1, scale2);
	}

	return ps;
}

/*!
	input: List of 2D objects
	output: 3D PolySet
	operation:
	  o Union all children
	  o Perform extrude
 */			
Response GeometryEvaluator::visit(State &state, const LinearExtrudeNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) {
			const Geometry *geometry = NULL;
			if (!node.filename.empty()) {
				DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale_x);

				Polygon2d *p2d = dxf.toPolygon2d();
				if (p2d) geometry = ClipperUtils::sanitize(*p2d);
			}
			else {
				geometry = applyToChildren2D(node, OPENSCAD_UNION);
			}
			if (geometry) {
				const Polygon2d *polygons = dynamic_cast<const Polygon2d*>(geometry);
				Geometry *extruded = extrudePolygon(node, *polygons);
				assert(extruded);
				geom.reset(extruded);
				delete geometry;
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

static void fill_ring(std::vector<Vector3d> &ring, const Outline2d &o, double a)
{
	for (int i=0;i<o.vertices.size();i++) {
		ring[i][0] = o.vertices[i][0] * sin(a);
		ring[i][1] = o.vertices[i][0] * cos(a);
		ring[i][2] = o.vertices[i][1];
	}
}

/*!
	Input to extrude should be clean. This means non-intersecting, correct winding order
	etc., the input coming from a library like Clipper.
*/
static Geometry *rotatePolygon(const RotateExtrudeNode &node, const Polygon2d &poly)
{
	PolySet *ps = new PolySet(3);
	ps->setConvexity(node.convexity);

	BOOST_FOREACH(const Outline2d &o, poly.outlines()) {
		double min_x = 0;
		double max_x = 0;
		BOOST_FOREACH(const Vector2d &v, o.vertices) {
			min_x = fmin(min_x, v[0]);
			max_x = fmax(max_x, v[0]);

			if ((max_x - min_x) > max_x && (max_x - min_x) > fabs(min_x)) {
				PRINTB("ERROR: all points for rotate_extrude() must have the same X coordinate sign (range is %.2f -> %.2f)", min_x % max_x);
				delete ps;
				return NULL;
			}
		}
		int fragments = Calc::get_fragments_from_r(max_x - min_x, node.fn, node.fs, node.fa);

		std::vector<Vector3d> rings[2];
		rings[0].resize(o.vertices.size());
		rings[1].resize(o.vertices.size());

		fill_ring(rings[0], o, -M_PI/2); // first ring
		for (int j = 0; j < fragments; j++) {
			double a = ((j+1)%fragments*2*M_PI) / fragments - M_PI/2; // start on the X axis
			fill_ring(rings[(j+1)%2], o, a);

			for (size_t i=0;i<o.vertices.size();i++) {
				ps->append_poly();
				ps->insert_vertex(rings[j%2][i]);
				ps->insert_vertex(rings[(j+1)%2][(i+1)%o.vertices.size()]);
				ps->insert_vertex(rings[j%2][(i+1)%o.vertices.size()]);
				ps->append_poly();
				ps->insert_vertex(rings[j%2][i]);
				ps->insert_vertex(rings[(j+1)%2][i]);
				ps->insert_vertex(rings[(j+1)%2][(i+1)%o.vertices.size()]);
			}
		}
	}
	return ps;
}

/*!
	input: List of 2D objects
	output: 3D PolySet
	operation:
	  o Union all children
	  o Perform extrude
 */			
Response GeometryEvaluator::visit(State &state, const RotateExtrudeNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) {
			const Geometry *geometry = NULL;
			if (!node.filename.empty()) {
				DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale);
				Polygon2d *p2d = dxf.toPolygon2d();
				if (p2d) geometry = ClipperUtils::sanitize(*p2d);
			}
			else {
				geometry = applyToChildren2D(node, OPENSCAD_UNION);
			}
			if (geometry) {
				const Polygon2d *polygons = dynamic_cast<const Polygon2d*>(geometry);
				Geometry *rotated = rotatePolygon(node, *polygons);
				geom.reset(rotated);
				delete geometry;
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

/*!
	Handles non-leaf PolyNodes; projection
*/
Response GeometryEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	assert(false);
	return AbortTraversal;
}

/*!
	input: List of 3D objects
	output: Polygon2d
	operation:
	  o Union all children
		o Perform projection
 */			
Response GeometryEvaluator::visit(State &state, const ProjectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {

			if (!node.cut_mode) {
				ClipperLib::Clipper sumclipper;
				BOOST_FOREACH(const Geometry::ChildItem &item, this->visitedchildren[node.index()]) {
					const AbstractNode *chnode = item.first;
					const shared_ptr<const Geometry> &chgeom = item.second;
					// FIXME: Don't use deep access to modinst members
					if (chnode->modinst->isBackground()) continue;

					const Polygon2d *poly = NULL;

// CGAL version of Geometry projection
// Causes crashes in createNefPolyhedronFromGeometry() for this model:
// projection(cut=false) {
//    cube(10);
//    difference() {
//      sphere(10);
//      cylinder(h=30, r=5, center=true);
//    }
// }
#if 0
					shared_ptr<const PolySet> chPS = dynamic_pointer_cast<const PolySet>(chgeom);
					const PolySet *ps2d = NULL;
					shared_ptr<const CGAL_Nef_polyhedron> chN = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
					if (chN) chPS.reset(chN->convertToPolyset());
					if (chPS) ps2d = PolysetUtils::flatten(*chPS);
					if (ps2d) {
						CGAL_Nef_polyhedron *N2d = createNefPolyhedronFromGeometry(*ps2d);
						poly = N2d->convertToPolygon2d();
					}
#endif

// Clipper version of Geometry projection
// Clipper doesn't handle meshes very well.
// It's better in V6 but not quite there. FIXME: stand-alone example.
#if 1
					// project chgeom -> polygon2d
					shared_ptr<const PolySet> chPS = dynamic_pointer_cast<const PolySet>(chgeom);
					if (!chPS) {
						shared_ptr<const CGAL_Nef_polyhedron> chN = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
						if (chN) {
							chPS.reset(chN->convertToPolyset());
						}
					}
					if (chPS) poly = PolysetUtils::project(*chPS);
#endif

					if (poly) {
						ClipperLib::Paths result = ClipperUtils::fromPolygon2d(*poly);
						// Using NonZero ensures that we don't create holes from polygons sharing
						// edges since we're unioning a mesh
						result = ClipperUtils::process(result, 
																					 ClipperLib::ctUnion, 
																					 ClipperLib::pftNonZero);
						// Add correctly winded polygons to the main clipper
						sumclipper.AddPaths(result, ClipperLib::ptSubject, true);
					}
				}
				ClipperLib::PolyTree sumresult;
				// This is key - without StrictlySimple, we tend to get self-intersecting results
				sumclipper.StrictlySimple(true);
				sumclipper.Execute(ClipperLib::ctUnion, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
				if (sumresult.Total() > 0) geom.reset(ClipperUtils::toPolygon2d(sumresult));
			}
			else {
				shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OPENSCAD_UNION).constptr();
				if (newgeom) {
					shared_ptr<const CGAL_Nef_polyhedron> Nptr = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(newgeom);
					if (!Nptr) {
						Nptr.reset(createNefPolyhedronFromGeometry(*newgeom));
					}
					if (!Nptr->isEmpty()) {
						Polygon2d *poly = CGALUtils::project(*Nptr, node.cut_mode);
						assert(poly);
						poly->setConvexity(node.convexity);
						geom.reset(poly);
					}
				}
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}		

/*!
	input: List of 2D or 3D objects (not mixed)
	output: any Geometry
	operation:
	  o Perform cgal operation
 */			
Response GeometryEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) {
			switch (node.type) {
			case MINKOWSKI: {
				geom = applyToChildren(node, OPENSCAD_MINKOWSKI).constptr();
				break;
			}
			case HULL: {
				geom = applyToChildren(node, OPENSCAD_HULL).constptr();
				break;
			}
			case RESIZE: {
				ResultObject res = applyToChildren(node, OPENSCAD_UNION);
				geom = res.constptr();

				shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(res.constptr());
				if (N) {
					// If we got a const object, make a copy
					shared_ptr<CGAL_Nef_polyhedron> newN;
					if (res.isConst()) newN.reset(new CGAL_Nef_polyhedron(*N));
					else newN = dynamic_pointer_cast<CGAL_Nef_polyhedron>(res.ptr());
					applyResize3D(*newN, node.newsize, node.autosize);
					geom = newN;
				}
				else {
					shared_ptr<const Polygon2d> poly = dynamic_pointer_cast<const Polygon2d>(res.constptr());
					if (poly) {
						// If we got a const object, make a copy
						shared_ptr<Polygon2d> newpoly;
						if (res.isConst()) newpoly.reset(new Polygon2d(*poly));
						else newpoly = dynamic_pointer_cast<Polygon2d>(res.ptr());

						newpoly->resize(Vector2d(node.newsize[0], node.newsize[1]),
														Eigen::Matrix<bool,2,1>(node.autosize[0], node.autosize[1]));
					}
					else {
						shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(res.constptr());
						if (ps) {
							// If we got a const object, make a copy
							shared_ptr<PolySet> newps;
							if (res.isConst()) newps.reset(new PolySet(*ps));
							else newps = dynamic_pointer_cast<PolySet>(res.ptr());

							newps->resize(node.newsize, node.autosize);
							geom = newps;
						}
						else {
							assert(false);
						}
					}
				}
				break;
			}
			default:
				assert(false && "not implemented");
			}
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

Response GeometryEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {
			geom = applyToChildren(node, OPENSCAD_INTERSECTION).constptr();
		}
		else {
			geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}
