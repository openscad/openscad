#include "CGALCache.h"
#include "CGALEvaluator.h"
#include "GeometryEvaluator.h"
#include "traverser.h"
#include "visitor.h"
#include "state.h"
#include "module.h" // FIXME: Temporarily for ModuleInstantiation
#include "printutils.h"

#include "csgnode.h"
#include "cgaladvnode.h"
#include "transformnode.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "Tree.h"

#include "CGALCache.h"
#include "cgal.h"
#include "cgalutils.h"

#ifdef NDEBUG
#define PREV_NDEBUG NDEBUG
#undef NDEBUG
#endif
#include <CGAL/convex_hull_3.h>
#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <map>

CGAL_Nef_polyhedron CGALEvaluator::evaluateCGALMesh(const AbstractNode &node)
{
	if (!isCached(node)) {
		Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
		evaluate.execute();
		return this->root;
	}
	return CGALCache::instance()->get(this->tree.getIdString(node));
}

bool CGALEvaluator::isCached(const AbstractNode &node) const
{
	return CGALCache::instance()->contains(this->tree.getIdString(node));
}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
 */
void CGALEvaluator::process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALEvaluator::CsgOp op)
{
 	if (target.dim != 2 && target.dim != 3) {
 		assert(false && "Dimension of Nef polyhedron must be 2 or 3");
 	}
	if (src.isEmpty()) return; // Empty polyhedron. This can happen for e.g. square([0,0])
	if (target.isEmpty() && op != CGE_UNION) return; // empty op <something> => empty
	if (target.dim != src.dim) return; // If someone tries to e.g. union 2d and 3d objects

	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try {
		switch (op) {
		case CGE_UNION:
			if (target.isEmpty()) target = src.copy();
			else target += src;
			break;
		case CGE_INTERSECTION:
			target *= src;
			break;
		case CGE_DIFFERENCE:
			target -= src;
			break;
		case CGE_MINKOWSKI:
			target.minkowski(src);
			break;
		}
	}
	catch (const CGAL::Failure_exception &e) {
		// union && difference assert triggered by testdata/scad/bugs/rotate-diff-nonmanifold-crash.scad and testdata/scad/bugs/issue204.scad
		std::string opstr = op == CGE_UNION ? "union" : op == CGE_INTERSECTION ? "intersection" : op == CGE_DIFFERENCE ? "difference" : op == CGE_MINKOWSKI ? "minkowski" : "UNKNOWN";
		PRINTB("CGAL error in CGAL_Nef_polyhedron's %s operator: %s", opstr % e.what());

		// Errors can result in corrupt polyhedrons, so put back the old one
		target = src;
	}
	CGAL::set_error_behaviour(old_behaviour);
}

/*!
*/
CGAL_Nef_polyhedron CGALEvaluator::applyToChildren(const AbstractNode &node, CGALEvaluator::CsgOp op)
{
	CGAL_Nef_polyhedron N;
	BOOST_FOREACH(const ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const CGAL_Nef_polyhedron &chN = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the 
    // cache could have been modified before we reach this point due to a large
    // sibling object. 
		if (!isCached(*chnode)) {
			CGALCache::instance()->insert(this->tree.getIdString(*chnode), chN);
		}
		// Initialize N on first iteration with first expected geometric object
		if (N.isNull() && !N.isEmpty()) N = chN.copy();
		else process(N, chN, op);

		chnode->progress_report();
	}
	return N;
}

CGAL_Nef_polyhedron CGALEvaluator::applyHull(const CgaladvNode &node)
{
	CGAL_Nef_polyhedron N;
	std::list<CGAL_Nef_polyhedron2*> polys;
	std::list<CGAL_Nef_polyhedron2::Point> points2d;
	std::list<CGAL_Polyhedron::Vertex::Point_3> points3d;
	int dim = 0;
	BOOST_FOREACH(const ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const CGAL_Nef_polyhedron &chN = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;
		if (chN.dim == 0) continue; // Ignore object with dimension 0 (e.g. echo)
		if (dim == 0) {
			dim = chN.dim;
		}
		else if (dim != chN.dim) {
			PRINT("WARNING: hull() does not support mixing 2D and 3D objects.");
			continue;
		}
		if (chN.isNull()) { // If one of the children evaluated to a null object
			continue;
		}		
		if (dim == 2) {
			CGAL_Nef_polyhedron2::Explorer explorer = chN.p2->explorer();
			BOOST_FOREACH(const CGAL_Nef_polyhedron2::Explorer::Vertex &vh, 
										std::make_pair(explorer.vertices_begin(), explorer.vertices_end())) {
				if (explorer.is_standard(&vh)) {
					points2d.push_back(explorer.point(&vh));
				}
			}
		}
		else if (dim == 3) {
			CGAL_Polyhedron P;
			if (!chN.p3->is_simple()) {
				PRINT("Hull() currently requires a valid 2-manifold. Please modify your design. See http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
			}
			else {
				bool err = false;
				std::string errmsg("");
				try {
					err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(chN.p3), P );
					//chN.p3->convert_to_Polyhedron(P);
				} catch (const CGAL::Failure_exception &e) {
					err = true;
					errmsg = std::string(e.what());
				}
				if (err) {
					PRINTB("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed. %s", errmsg);
				} else {
					std::transform(P.vertices_begin(), P.vertices_end(), std::back_inserter(points3d), 
										 boost::bind(static_cast<const CGAL_Polyhedron::Vertex::Point_3&(CGAL_Polyhedron::Vertex::*)() const>(&CGAL_Polyhedron::Vertex::point), _1));
				}
			}
		}
		chnode->progress_report();
	}
	
	if (dim == 2) {
		std::list<CGAL_Nef_polyhedron2::Point> result;
		CGAL::convex_hull_2(points2d.begin(), points2d.end(),std:: back_inserter(result));
		N = CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron2(result.begin(), result.end(), 
																										 CGAL_Nef_polyhedron2::INCLUDED));
	}
	else if (dim == 3) {
		CGAL_Polyhedron P;
		if (points3d.size()>3)
			CGAL::convex_hull_3(points3d.begin(), points3d.end(), P);
		N = CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3(P));
	}
	return N;
}

CGAL_Nef_polyhedron CGALEvaluator::applyResize(const CgaladvNode &node)
{
	// Based on resize() in Giles Bathgate's RapCAD (but not exactly)
	CGAL_Nef_polyhedron N;
	N = applyToChildren(node, CGE_UNION);

	if ( N.isNull() || N.isEmpty() ) return N;

	for (int i=0;i<3;i++) {
		if (node.newsize[i]<0) {
			PRINT("WARNING: Cannot resize to sizes less than 0.");
			return N;
		}
	}

	CGAL_Iso_cuboid_3 bb;

	if ( N.dim == 2 ) {
		CGAL_Iso_rectangle_2e bbox = bounding_box( *N.p2 );
		CGAL_Point_2e min2(bbox.min()), max2(bbox.max());
		CGAL_Point_3 min3(CGAL::to_double(min2.x()), CGAL::to_double(min2.y()), 0),
			max3(CGAL::to_double(max2.x()), CGAL::to_double(max2.y()), 0);
		bb = CGAL_Iso_cuboid_3( min3, max3 );
	}
	else {
		bb = bounding_box( *N.p3 );
	}

	std::vector<NT3> scale, bbox_size;
	for (int i=0;i<3;i++) scale.push_back( NT3(1) );
	bbox_size.push_back( bb.xmax()-bb.xmin() );
	bbox_size.push_back( bb.ymax()-bb.ymin() );
	bbox_size.push_back( bb.zmax()-bb.zmin() );
	int newsizemax_index = 0;
	for (int i=0;i<N.dim;i++) {
		if (node.newsize[i]) {
			if (bbox_size[i]==NT3(0)) {
				PRINT("WARNING: Resize in direction normal to flat object is not implemented");
				return N;
			}
			else {
				scale[i] = NT3(node.newsize[i]) / bbox_size[i];
			}
			if ( node.newsize[i] > node.newsize[newsizemax_index] )
				newsizemax_index = i;
		}
	}

	NT3 autoscale = NT3( 1 );
	if ( node.newsize[ newsizemax_index ] != 0 )
		autoscale = NT3( node.newsize[ newsizemax_index ] ) / bbox_size[ newsizemax_index ];
	for (int i=0;i<N.dim;i++) {
		if (node.autosize[i] && node.newsize[i]==0)
			scale[i] = autoscale;
	}

	Eigen::Matrix4d t;
	t << CGAL::to_double(scale[0]),           0,        0,        0,
	     0,        CGAL::to_double(scale[1]),           0,        0,
	     0,        0,        CGAL::to_double(scale[2]),           0,
	     0,        0,        0,                                   1;

	N.transform( Transform3d( t ) );
	return N;
}



/*
	Typical visitor behavior:
	o In prefix: Check if we're cached -> prune
	o In postfix: Check if we're cached -> don't apply operator to children
	o In postfix: addToParent()
 */

Response CGALEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) N = applyToChildren(node, CGE_UNION);
		else N = CGALCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) N = applyToChildren(node, CGE_INTERSECTION);
		else N = CGALCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			CGALEvaluator::CsgOp op = CGE_UNION;
			switch (node.type) {
			case CSG_TYPE_UNION:
				op = CGE_UNION;
				break;
			case CSG_TYPE_DIFFERENCE:
				op = CGE_DIFFERENCE;
				break;
			case CSG_TYPE_INTERSECTION:
				op = CGE_INTERSECTION;
				break;
			default:
				assert(false);
			}
			N = applyToChildren(node, op);
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			// First union all children
			N = applyToChildren(node, CGE_UNION);
			if ( matrix_contains_infinity( node.matrix ) || matrix_contains_nan( node.matrix ) ) {
				// due to the way parse/eval works we can't currently distinguish between NaN and Inf
				PRINT("Warning: Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
				N.reset();
			}
			N.transform( node.matrix );
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

/*!
	Leaf nodes can create their own geometry, so let them do that
*/
Response CGALEvaluator::visit(State &state, const LeafNode &node)
{
	if (state.isPrefix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			shared_ptr<const Geometry> geom(node.createGeometry());
			if (geom) N = createNefPolyhedronFromGeometry(*geom);
			node.progress_report();
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return PruneTraversal;
}

/*!
	Handles non-leaf PolyNodes; extrudes, projection
*/
Response CGALEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			// Apply polyset operation
			shared_ptr<const Geometry> geom = this->geomevaluator.evaluateGeometry(node);
			if (geom) N = createNefPolyhedronFromGeometry(*geom);
			node.progress_report();
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

Response CGALEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N;
		if (!isCached(node)) {
			CGALEvaluator::CsgOp op;
			switch (node.type) {
			case MINKOWSKI:
				op = CGE_MINKOWSKI;
				N = applyToChildren(node, op);
				break;
			case GLIDE:
				PRINT("WARNING: glide() is not implemented yet!");
				return PruneTraversal;
				break;
			case SUBDIV:
				PRINT("WARNING: subdiv() is not implemented yet!");
				return PruneTraversal;
				break;
			case HULL:
				N = applyHull(node);
				break;
			case RESIZE:
				N = applyResize(node);
				break;
			}
		}
		else {
			N = CGALCache::instance()->get(this->tree.getIdString(node));
		}
		addToParent(state, node, N);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CGALEvaluator::addToParent(const State &state, const AbstractNode &node, const CGAL_Nef_polyhedron &N)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, N));
	}
	else {
		// Root node, insert into cache
		if (!isCached(node)) {
			if (!CGALCache::instance()->insert(this->tree.getIdString(node), N)) {
				PRINT("WARNING: CGAL Evaluator: Root node didn't fit into cache");
			}
		}
		this->root = N;
	}
}
