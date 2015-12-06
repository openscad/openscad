#pragma once

#include <map>
#include <list>
#include <vector>
#include <cstddef>
#include "visitor.h"
#include "memory.h"
#include "stl-utils.h"

class CSGTermEvaluator : public Visitor
{
public:
	CSGTermEvaluator(const class Tree &tree, class GeometryEvaluator *geomevaluator = NULL)
		: tree(tree), geomevaluator(geomevaluator) {
	}
	virtual ~CSGTermEvaluator();

	virtual Response visit(State &state, const class AbstractNode &node);
 	virtual Response visit(State &state, const class AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const class AbstractPolyNode &node);
 	virtual Response visit(State &state, const class CsgNode &node);
 	virtual Response visit(State &state, const class TransformNode &node);
	virtual Response visit(State &state, const class ColorNode &node);
 	virtual Response visit(State &state, const class RenderNode &node);
 	virtual Response visit(State &state, const class CgaladvNode &node);

	shared_ptr<class CSGTerm> evaluateCSGTerm(const AbstractNode &node,
						  std::vector<shared_ptr<CSGTerm> > &highlights,
						  std::vector<shared_ptr<CSGTerm> > &background);

private:
	enum CsgOp {CSGT_UNION, CSGT_INTERSECTION, CSGT_DIFFERENCE, CSGT_MINKOWSKI};
	void addToParent(const State &state, const AbstractNode &node);
	void applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op);

	const AbstractNode *root;
	typedef std::list<const AbstractNode *> ChildList;
	std::map<int, ChildList> visitedchildren;

public:
	std::map<int, shared_ptr<CSGTerm> > stored_term; // The term evaluated from each node index

	std::vector<shared_ptr<CSGTerm> > highlights;
	std::vector<shared_ptr<CSGTerm> > background;
	const Tree &tree;
	class GeometryEvaluator *geomevaluator;
};
