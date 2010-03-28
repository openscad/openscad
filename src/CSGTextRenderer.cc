#include "CSGTextRenderer.h"
#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "state.h"
#include "nodecache.h"
#include "module.h" // FIXME: Temporarily for ModuleInstantiation

#include "csgnode.h"
#include "transformnode.h"

#include <sstream>
#include <iostream>
#include <assert.h>
#include <QRegExp>

string CSGTextRenderer::getCGALMesh() const
{
	assert(this->root); 
	// FIXME: assert that cache contains root
	return this->cache[mk_cache_id(*this->root)];
}

// CGAL_Nef_polyhedron CSGTextRenderer::getCGALMesh() const
// {
// 	assert(this->root); 
// // FIXME: assert that cache contains root
// 	return this->cache[*this->root];
// }

bool CSGTextRenderer::isCached(const AbstractNode &node)
{
	return this->cache.contains(mk_cache_id(node));
}

/*!
	Modifies target by applying op to target and src:
	target = target [op] src
 */
void
CSGTextRenderer::process(string &target, const string &src, CSGTextRenderer::CsgOp op)
{
// 	if (target.dim != 2 && target.dim != 3) {
// 		assert(false && "Dimension of Nef polyhedron must be 2 or 3");
// 	}

	switch (op) {
	case UNION:
		target += "+" + src;
		break;
	case INTERSECTION:
		target += "*" + src;
		break;
	case DIFFERENCE:
		target += "-" + src;
		break;
	case MINKOWSKI:
		target += "M" + src;
		break;
	}
}

// /*!
// 	Modifies target by applying op to target and src:
// 	target = target [op] src
//  */
// void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CsgOp op)
// {
// 	if (target.dim == 2) {
// 		switch (op) {
// 		case UNION:
// 			target.p2 += src.p2;
// 			break;
// 		case INTERSECTION:
// 			target.p2 *= src.p2;
// 			break;
// 		case DIFFERENCE:
// 			target.p2 -= src.p2;
// 			break;
// 		case MINKOWSKI:
// 			target.p2 = minkowski2(target.p2, src.p2);
// 			break;
// 		}
// 	}
// 	else if (target.dim == 3) {
// 		switch (op) {
// 		case UNION:
// 			target.p3 += src.p3;
// 			break;
// 		case INTERSECTION:
// 			target.p3 *= src.p3;
// 			break;
// 		case DIFFERENCE:
// 			target.p3 -= src.p3;
// 			break;
// 		case MINKOWSKI:
// 			target.p3 = minkowski3(target.p3, src.p3);
// 			break;
// 		}
// 	}
// 	else {
// 		assert(false && "Dimention of Nef polyhedron must be 2 or 3");
// 	}
// }

void CSGTextRenderer::applyToChildren(const AbstractNode &node, CSGTextRenderer::CsgOp op)
{
	std::stringstream stream;
	stream << typeid(node).name();
	stream << "<" << node.index() << ">";
	string N = stream.str();
	if (this->visitedchildren[node.index()].size() > 0) {
		// FIXME: assert that cache contains nodes in code below
		bool first = true;
//		CGAL_Nef_polyhedron N;
		for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
				 iter != this->visitedchildren[node.index()].end();
				 iter++) {
			const AbstractNode *chnode = iter->first;
			const QString &chcacheid = iter->second;
			// FIXME: Don't use deep access to modinst members
			if (chnode->modinst->tag_background) continue;
			if (first) {
				N += "(" + this->cache[chcacheid];
// 				if (N.dim != 0) first = false; // FIXME: when can this happen?
				first = false;
			} else {
				process(N, this->cache[chcacheid], op);
			}
			chnode->progress_report();
		}
		N += ")";
	}
	QString cacheid = mk_cache_id(node);
	this->cache.insert(cacheid, N);
}

/*
	Typical visitor behavior:
	o In prefix: Check if we're cached -> prune
	o In postfix: Check if we're cached -> don't apply operator to children
	o In postfix: addToParent()
 */

Response CSGTextRenderer::visit(const State &state, const AbstractNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) applyToChildren(node, UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTextRenderer::visit(const State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) applyToChildren(node, INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTextRenderer::visit(const State &state, const CsgNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			CsgOp op;
			switch (node.type) {
			case CSG_TYPE_UNION:
				op = UNION;
				break;
			case CSG_TYPE_DIFFERENCE:
				op = DIFFERENCE;
				break;
			case CSG_TYPE_INTERSECTION:
				op = INTERSECTION;
				break;
			}
			applyToChildren(node, op);
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTextRenderer::visit(const State &state, const TransformNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {
			// First union all children
			applyToChildren(node, UNION);
			// FIXME: Then apply transform
		}
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: RenderNode: Union over children + some magic
// FIXME: CgaladvNode: Iterate over children. Special operation

// FIXME: Subtypes of AbstractPolyNode:
// ProjectionNode
// DxfLinearExtrudeNode
// DxfRotateExtrudeNode
// (SurfaceNode)
// (PrimitiveNode)
Response CSGTextRenderer::visit(const State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix() && isCached(node)) return PruneTraversal;
	if (state.isPostfix()) {
		if (!isCached(node)) {

	// FIXME: Manage caching
	// FIXME: Will generate one single Nef polyhedron (no csg ops necessary)
 
// 	PolySet *ps = render_polyset(RENDER_CGAL);
// 	try {
// 		CGAL_Nef_polyhedron N = ps->renderCSGMesh();
// 		cgal_nef_cache.insert(cache_id, new cgal_nef_cache_entry(N), N.weight());
// 		print_messages_pop();
// 		progress_report();
		
// 		ps->unlink();
// 		return N;
// 	}
// 	catch (...) { // Don't leak the PolySet on ProgressCancelException
// 		ps->unlink();
// 		throw;
// 	}

			string N = typeid(node).name();
			QString cacheid = mk_cache_id(node);
			this->cache.insert(cacheid, N);
		
// 		std::cout << "Insert: " << N << "\n";
// 		std::cout << "Node: " << cacheid.toStdString() << "\n\n";
		}
		addToParent(state, node);
	}

	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CSGTextRenderer::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	QString cacheid = mk_cache_id(node);
	this->visitedchildren.erase(node.index());
	if (!state.parent()) {
		this->root = &node;
	}
	else {
		this->visitedchildren[state.parent()->index()].push_back(std::make_pair(&node, cacheid));
	}
}

/*!
  Create a cache id of the entire tree under this node. This cache id
	is a non-whitespace plaintext of the evaluated scad tree and is used
	for lookup in cgal_nef_cache.
*/
QString CSGTextRenderer::mk_cache_id(const AbstractNode &node) const
{
	// FIXME: should we keep a cache of cache_id's to avoid recalculating this?
	// -> check how often we recalculate it.

	// FIXME: Get dump from dump cache
	// FIXME: assert that cache contains node
	QString cache_id = QString::fromStdString(this->dumpcache[node]);
	// Remove all node indices and whitespace
	cache_id.remove(QRegExp("[a-zA-Z_][a-zA-Z_0-9]*:"));
	cache_id.remove(' ');
	cache_id.remove('\t');
	cache_id.remove('\n');
	return cache_id;
}
