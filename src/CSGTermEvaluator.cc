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
#include "PolySetEvaluator.h"

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

	A visitor responsible for creating a tree of CSGTerm nodes used for rendering
	with OpenCSG.
*/

shared_ptr<CSGTerm> CSGTermEvaluator::evaluateCSGTerm(const AbstractNode &node, 
																					 std::vector<shared_ptr<CSGTerm> > &highlights, 
																					 std::vector<shared_ptr<CSGTerm> > &background)
{
	Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
	highlights = this->highlights;
	background = this->background;
	return this->stored_term[node.index()];
}

void CSGTermEvaluator::applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op)
{
	shared_ptr<CSGTerm> t1;
	BOOST_FOREACH(const AbstractNode *chnode, this->visitedchildren[node.index()]) {
		shared_ptr<CSGTerm> t2(this->stored_term[chnode->index()]);
		this->stored_term.erase(chnode->index());
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (op == CSGT_UNION) {
				t1 = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, t1, t2);
			} else if (op == CSGT_DIFFERENCE) {
				t1 = CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, t1, t2);
			} else if (op == CSGT_INTERSECTION) {
				t1 = CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			}
		}
	}
	if (t1 && node.modinst->isHighlight()) {
		t1->flag = CSGTerm::FLAG_HIGHLIGHT;
		this->highlights.push_back(t1);
	}
	if (t1 && node.modinst->isBackground()) {
		this->background.push_back(t1);
		t1.reset(); // don't propagate background tagged nodes
	}
	this->stored_term[node.index()] = t1;
}

Response CSGTermEvaluator::visit(State &state, const AbstractNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

static shared_ptr<CSGTerm> evaluate_csg_term_from_ps(const State &state, 
																					std::vector<shared_ptr<CSGTerm> > &highlights, 
																					std::vector<shared_ptr<CSGTerm> > &background, 
																					const shared_ptr<PolySet> &ps, 
																					const ModuleInstantiation *modinst, 
																					const AbstractNode &node)
{
	std::stringstream stream;
	stream << node.name() << node.index();
	shared_ptr<CSGTerm> t(new CSGTerm(ps, state.matrix(), state.color(), stream.str()));
	if (modinst->isHighlight()) {
		t->flag = CSGTerm::FLAG_HIGHLIGHT;
		highlights.push_back(t);
	}
	if (modinst->isBackground()) {
		background.push_back(t);
		t.reset();
	}
	return t;
}

Response CSGTermEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGTerm> t1;
		if (this->psevaluator) {
			shared_ptr<PolySet> ps = this->psevaluator->getPolySet(node, true);
			if (ps) {
				t1 = evaluate_csg_term_from_ps(state, this->highlights, this->background, 
																			 ps, node.modinst, node);
				node.progress_report();
			}
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
		case CSG_TYPE_UNION:
			op = CSGT_UNION;
			break;
		case CSG_TYPE_DIFFERENCE:
			op = CSGT_DIFFERENCE;
			break;
		case CSG_TYPE_INTERSECTION:
			op = CSGT_INTERSECTION;
			break;
		default:
			assert(false);
		}
		applyToChildren(node, op);
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
		applyToChildren(node, CSGT_UNION);
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
		applyToChildren(node, CSGT_UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: If we've got CGAL support, render this node as a CGAL union into a PolySet
Response CSGTermEvaluator::visit(State &state, const RenderNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGTerm> t1;
		shared_ptr<PolySet> ps;
		if (this->psevaluator) {
			ps = this->psevaluator->getPolySet(node, true);
			node.progress_report();
		}
		if (ps) {
			t1 = evaluate_csg_term_from_ps(state, this->highlights, this->background, 
																		 ps, node.modinst, node);
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPostfix()) {
		shared_ptr<CSGTerm> t1;
    // FIXME: Calling evaluator directly since we're not a PolyNode. Generalize this.
		shared_ptr<PolySet> ps;
		if (this->psevaluator) {
			ps = this->psevaluator->getPolySet(node, true);
		}
		if (ps) {
			t1 = evaluate_csg_term_from_ps(state, this->highlights, this->background, 
																		 ps, node.modinst, node);
			node.progress_report();
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CSGTermEvaluator::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(&node);
	}
}
