#include "GeometryEvaluator.h"
#include "traverser.h"
#include "tree.h"
#include "GeometryCache.h"
#include "Polygon2d.h"
#include "module.h"
#include "state.h"
#include "transformnode.h"
#include "clipper-utils.h"
#include "CGALEvaluator.h"
#include "CGALCache.h"
#include "PolySet.h"

#include <boost/foreach.hpp>

GeometryEvaluator::GeometryEvaluator(const class Tree &tree):
	tree(tree)
{
	this->cgalevaluator = new CGALEvaluator(tree, *this);
}

/*!
  Factory method returning a Geometry from the given node. If the
  node is already cached, the cached Geometry will be returned
  otherwise a new Geometry will be created from the node. If cache is
  true, the newly created Geometry will be cached.

	FIXME: This looks redundant
 */
shared_ptr<const Geometry> GeometryEvaluator::getGeometry(const AbstractNode &node, bool cache)
{
	std::string cacheid = this->tree.getIdString(node);

	if (GeometryCache::instance()->contains(cacheid)) {
#ifdef DEBUG
    // For cache debugging
		PRINTB("GeometryCache hit: %s", cacheid.substr(0, 40));
#endif
		return GeometryCache::instance()->get(cacheid);
	}

	shared_ptr<const Geometry> geom(this->evaluateGeometry(node));

	if (cache) GeometryCache::instance()->insert(cacheid, geom);
	return geom;
}

bool GeometryEvaluator::isCached(const AbstractNode &node) const
{
	return GeometryCache::instance()->contains(this->tree.getIdString(node));
}

// FIXME: This doesn't insert into cache. Fix this here or in client code
shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode &node)
{
	if (!isCached(node)) {
		Traverser trav(*this, node, Traverser::PRE_AND_POSTFIX);
		trav.execute();
		return this->root;
	}
	return GeometryCache::instance()->get(this->tree.getIdString(node));
}

/*!
*/
Geometry *GeometryEvaluator::applyToChildren(const AbstractNode &node, OpenSCADOperator op)
{
	// FIXME: Support other operators than UNION
	
	Polygon2d sum;
	BOOST_FOREACH(const ChildItem &item, this->visitedchildren[node.index()]) {
		const AbstractNode *chnode = item.first;
		const shared_ptr<const Geometry> &chgeom = item.second;
		// FIXME: Don't use deep access to modinst members
		if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the 
    // cache could have been modified before we reach this point due to a large
    // sibling object. 
		if (!isCached(*chnode)) {
			GeometryCache::instance()->insert(this->tree.getIdString(*chnode), chgeom);
		}

		assert(chgeom->getDimension() == 2);
		shared_ptr<const Polygon2d> polygons = dynamic_pointer_cast<const Polygon2d>(chgeom);
		assert(polygons);
		BOOST_FOREACH(const Outline2d &o, polygons->outlines()) {
			sum.addOutline(o);
		}
		chnode->progress_report();
	}

	ClipperLib::Clipper clipper;
	clipper.AddPolygons(ClipperUtils::fromPolygon2d(sum), ClipperLib::ptSubject);
	ClipperLib::Polygons result;
	clipper.Execute(ClipperLib::ctUnion, result);

	if (result.size() == 0) return NULL;
	return ClipperUtils::toPolygon2d(result);
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during traversal.
	Usually, this should be called from the postfix stage, but for some nodes, 
	we defer traversal letting other components (e.g. CGAL) render the subgraph, 
	and we'll then call this from prefix and prune further traversal.
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
		if (!isCached(node)) {
			if (!GeometryCache::instance()->insert(this->tree.getIdString(node), geom)) {
				PRINT("WARNING: GeometryEvaluator: Root node didn't fit into cache");
			}
		}
		this->root = geom;
	}
}

/*!
	Fallback: If we don't know how to handle a node, send it to CGAL
 */
Response GeometryEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		CGAL_Nef_polyhedron N = this->cgalevaluator->evaluateCGALMesh(node);
		CGALCache::instance()->insert(this->tree.getIdString(node), N);

		PolySet *ps = NULL;
		if (!N.isNull()) ps = N.convertToPolyset();
		shared_ptr<const Geometry> geom(ps);
		GeometryCache::instance()->insert(this->tree.getIdString(node), geom);
		addToParent(state, node, geom);
	}
	return ContinueTraversal;
}

/*!
	Leaf nodes can create their own geometry, so let them do that
*/
Response GeometryEvaluator::visit(State &state, const LeafNode &node)
{
	if (state.isPrefix()) {
		shared_ptr<const Geometry> geom;
		if (!isCached(node)) geom.reset(node.createGeometry());
		else geom = GeometryCache::instance()->get(this->tree.getIdString(node));
		addToParent(state, node, geom);
	}
	return PruneTraversal;
}

Response GeometryEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isCached(node)) {
			// First union all children
			geom.reset(applyToChildren(node, CGE_UNION));
			if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
				// due to the way parse/eval works we can't currently distinguish between NaN and Inf
				PRINT("Warning: Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
				geom.reset();
			}
			//FIXME: Handle 2D vs. 3D
			shared_ptr<const Polygon2d> polygons = dynamic_pointer_cast<const Polygon2d>(geom);
			if (polygons) {
// FIXME: Convert from mat3 to mat2:			Transform2d mat2(node.matrix);
				Transform2d mat2;
//				polygons->transform(mat2);
			}
			else {
				// FIXME: Handle 3D transfer
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
	Handles non-leaf PolyNodes; extrusions, projection
*/
Response GeometryEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPrefix()) {
		/* 
		shared_ptr<CSGTerm> t1;
		if (this->psevaluator) {
			shared_ptr<const Geometry> geom = this->psevaluator->getGeometry(node, true);
			if (geom) {
				t1 = evaluate_csg_term_from_geometry(state, this->highlights, this->background, 
																						 geom, node.modinst, node);
				node.progress_report();
			}
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
		*/
 		return PruneTraversal;
	}
	return ContinueTraversal;
}

