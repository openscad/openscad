#include "CSGTermEvaluator.h"
#include "visitor.h"
#include "state.h"
#include "csgterm.h"
#include "module.h"
#include "csgnode.h"
#include "transformnode.h"
#include "colornode.h"
#include "rendernode.h"
#include "cgaladvnode.h"
#include "printutils.h"
#include "GeometryEvaluator.h"
#include "polyset.h"
#include "polyset-utils.h"

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <cstddef>
#include <boost/foreach.hpp>

/*!
	\class CSGTermEvaluator

	A visitor responsible for creating a binary tree of CSGNode nodes used for rendering
	with OpenCSG.
*/

shared_ptr<CSGNode> CSGTermEvaluator::evaluateCSGTerm(const AbstractNode &node)
{
	Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
	this->root_term = this->stored_term[node.index()];
	
	return this->root_term;
}

void CSGTermEvaluator::applyToChildren(State &state, const AbstractNode &node, CSGTermEvaluator::CsgOp op)
{
	shared_ptr<CSGNode> t1;
	BOOST_FOREACH(const AbstractNode *chnode, this->visitedchildren[node.index()]) {
		shared_ptr<CSGNode> t2(this->stored_term[chnode->index()]);
		this->stored_term.erase(chnode->index());
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (op == CSGT_UNION) {
				t1 = CSGOperation::createCSGNode(CSGOperation::TYPE_UNION, t1, t2);
			} else if (op == CSGT_DIFFERENCE) {
				CSGNode::Flag flag = t1->flag;
				t1 = CSGOperation::createCSGNode(CSGOperation::TYPE_DIFFERENCE, t1, t2);
				t1->flag = CSGNode::Flag(t1->flag | flag);
			} else if (op == CSGT_INTERSECTION) {
				t1 = CSGOperation::createCSGNode(CSGOperation::TYPE_INTERSECTION, t1, t2);
			}
		}
	}

	if (t1 && ((t1->flag & CSGNode::FLAG_HIGHLIGHT) || node.modinst->isHighlight())) {
		t1->flag = CSGNode::FLAG_HIGHLIGHT;
		if (!state.isHighlight()) {
			this->highlight_terms.push_back(t1);
			state.setHighlight(true);

			// FIXME: If we remove the positive part of a difference, we cannot properly render the negative
			t1.reset();
		}
	}
	if (t1 && node.modinst->isBackground()) {
		t1->flag = CSGNode::FLAG_BACKGROUND;
		state.setBackground(true);
	}
	this->stored_term[node.index()] = t1;
}

Response CSGTermEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(state, node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(state, node, CSGT_INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

shared_ptr<CSGNode> CSGTermEvaluator::evaluateCSGTermFromGeometry(
	State &state, const shared_ptr<const Geometry> &geom,
	const ModuleInstantiation *modinst, const AbstractNode &node)
{
	std::stringstream stream;
	stream << node.name() << node.index();

	// We cannot render Polygon2d directly, so we preprocess (tessellate) it here
	shared_ptr<const Geometry> g = geom;
	if (!g->isEmpty()) {
		shared_ptr<const Polygon2d> p2d = dynamic_pointer_cast<const Polygon2d>(geom);
		if (p2d) {
			g.reset(p2d->tessellate());
		}
		else {
			// We cannot render concave polygons, so tessellate any 3D PolySets
			shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);
			// Since is_convex() doesn't handle non-planar faces, we need to tessellate
			// also in the indeterminate state so we cannot just use a boolean comparison. See #1061
			bool convex = ps->convexValue();
			if (ps && !convex) {
				assert(ps->getDimension() == 3);
				PolySet *ps_tri = new PolySet(3, ps->convexValue());
				ps_tri->setConvexity(ps->getConvexity());
				PolysetUtils::tessellate_faces(*ps, *ps_tri);
				g.reset(ps_tri);
			}
		}
	}

	shared_ptr<CSGNode> t(new CSGLeaf(g, state.matrix(), state.color(), stream.str()));
	if (modinst->isHighlight()) {
		t->flag = CSGNode::FLAG_HIGHLIGHT;
		if (!state.isHighlight()) {
			this->highlight_terms.push_back(t);
			state.setHighlight(true);
		}
	}
	else if (modinst->isBackground()) {
		t->flag = CSGNode::FLAG_BACKGROUND;
	}
	return t;
}

Response CSGTermEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGNode> t1;
		if (this->geomevaluator) {
			shared_ptr<const Geometry> geom = this->geomevaluator->evaluateGeometry(node, false);
			if (geom) {
				t1 = evaluateCSGTermFromGeometry(state, geom, node.modinst, node);
			}
			node.progress_report();
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPostfix()) {
		CsgOp op = CSGT_UNION;
		switch (node.type) {
		case OPENSCAD_UNION:
			op = CSGT_UNION;
			break;
		case OPENSCAD_DIFFERENCE:
			op = CSGT_DIFFERENCE;
			break;
		case OPENSCAD_INTERSECTION:
			op = CSGT_INTERSECTION;
			break;
		default:
			assert(false);
		}
		applyToChildren(state, node, op);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix()) {
		state.setMatrix(state.matrix() * node.matrix);
	}
	if (state.isPostfix()) {
		applyToChildren(state, node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const ColorNode &node)
{
	if (state.isPrefix()) {
		if (!state.color().isValid()) state.setColor(node.color);
	}
	if (state.isPostfix()) {
		applyToChildren(state, node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: If we've got CGAL support, render this node as a CGAL union into a PolySet
Response CSGTermEvaluator::visit(State &state, const RenderNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGNode> t1;
		shared_ptr<const Geometry> geom;
		if (this->geomevaluator) {
			geom = this->geomevaluator->evaluateGeometry(node, false);
			if (geom) {
				t1 = evaluateCSGTermFromGeometry(state, geom, node.modinst, node);
			}
			node.progress_report();
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGNode> t1;
    // FIXME: Calling evaluator directly since we're not a PolyNode. Generalize this.
		shared_ptr<const Geometry> geom;
		if (this->geomevaluator) {
			geom = this->geomevaluator->evaluateGeometry(node, false);
			if (geom) {
				t1 = evaluateCSGTermFromGeometry(state, geom, node.modinst, node);
			}
			node.progress_report();
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during traversal.
    Usually, this should be called from the postfix stage, but for some nodes, we defer traversal letting other components (e.g. CGAL) render the subgraph, and we'll then call this from prefix and prune further traversal.
*/
void CSGTermEvaluator::addToParent(const State &state, const AbstractNode &node)
{
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(&node);
	}
}
