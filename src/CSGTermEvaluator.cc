#include "CSGTermEvaluator.h"
#include "visitor.h"
#include "state.h"
#include "csgterm.h"
#include "module.h"
#include "csgnode.h"
#include "transformnode.h"
#include "colornode.h"
#include "rendernode.h"
#include "printutils.h"

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>

/*!
	\class CSGTermEvaluator

	A visitor responsible for creating a tree of CSGTerm nodes used for rendering
	with OpenCSG.
*/

CSGTerm *CSGTermEvaluator::evaluateCSGTerm(const AbstractNode &node, 
																					 std::vector<CSGTerm*> &highlights, 
																					 std::vector<CSGTerm*> &background)
{
	Traverser evaluate(*this, node, Traverser::PRE_AND_POSTFIX);
	evaluate.execute();
	highlights = this->highlights;
	background = this->background;
	return this->stored_term[node.index()];
}

void CSGTermEvaluator::applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op)
{
	CSGTerm *t1 = NULL;
	for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
			 iter != this->visitedchildren[node.index()].end();
			 iter++) {
		const AbstractNode *chnode = *iter;
		CSGTerm *t2 = this->stored_term[chnode->index()];
		this->stored_term.erase(chnode->index());
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (op == CSGT_UNION) {
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
			} else if (op == CSGT_DIFFERENCE) {
				t1 = new CSGTerm(CSGTerm::TYPE_DIFFERENCE, t1, t2);
			} else if (op == CSGT_INTERSECTION) {
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			}
		}
	}
	if (t1 && node.modinst->tag_highlight) {
		this->highlights.push_back(t1->link());
	}
	if (t1 && node.modinst->tag_background) {
		this->background.push_back(t1);
		t1 = NULL; // don't propagate background tagged nodes
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

static CSGTerm *evaluate_csg_term_from_ps(const State &state, 
																				vector<CSGTerm*> &highlights, 
																				vector<CSGTerm*> &background, 
																				PolySet *ps, 
																				const ModuleInstantiation *modinst, 
																				const AbstractPolyNode &node)
{
	CSGTerm *t = new CSGTerm(ps, state.matrix(), state.color(), QString("%1%2").arg(node.name().c_str()).arg(node.index()));
	if (modinst->tag_highlight)
		highlights.push_back(t->link());
	if (modinst->tag_background) {
		background.push_back(t);
		return NULL;
	}
	return t;
}

Response CSGTermEvaluator::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPostfix()) {
		CSGTerm *t1 = NULL;
		PolySet *ps = node.evaluate_polyset(AbstractPolyNode::RENDER_OPENCSG, this->psevaluator);
		if (ps) {
			t1 = evaluate_csg_term_from_ps(state, this->highlights, this->background, 
																	 ps, node.modinst, node);
		}
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const CsgNode &node)
{
	if (state.isPostfix()) {
		CsgOp op;
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
		}
		applyToChildren(node, op);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix()) {
		double m[16];
		
		for (int i = 0; i < 16; i++)
		{
			int c_row = i%4;
			int m_col = i/4;
			m[i] = 0;
			for (int j = 0; j < 4; j++) {
				m[i] += state.matrix()[c_row + j*4] * node.matrix[m_col*4 + j];
			}
		}
		state.setMatrix(m);
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
		state.setColor(node.color);
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
	PRINT("WARNING: render() statement not implemented");
	if (state.isPostfix()) {
		applyToChildren(node, CSGT_UNION);
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
