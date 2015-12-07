#pragma once

#include <map>
#include <list>
#include <vector>
#include <cstddef>
#include "visitor.h"
#include "memory.h"
#include "CSGTerm.h"

class CSGTermEvaluator : public Visitor
{
public:
	CSGTermEvaluator(const class Tree &tree, class GeometryEvaluator *geomevaluator = NULL)
		: tree(tree), geomevaluator(geomevaluator) {
	}
  virtual ~CSGTermEvaluator() {}

  virtual Response visit(State &state, const class AbstractNode &node);
 	virtual Response visit(State &state, const class AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const class AbstractPolyNode &node);
 	virtual Response visit(State &state, const class CsgNode &node);
 	virtual Response visit(State &state, const class TransformNode &node);
	virtual Response visit(State &state, const class ColorNode &node);
 	virtual Response visit(State &state, const class RenderNode &node);
 	virtual Response visit(State &state, const class CgaladvNode &node);

	shared_ptr<class CSGNode> evaluateCSGTerm(const AbstractNode &node);

	const shared_ptr<CSGNode> &getRootTerm() const {
		return this->root_term;
	}
	const std::vector<shared_ptr<CSGNode> > &getHighlightTerms() const {
		return this->highlight_terms;
	}
	const std::vector<shared_ptr<CSGNode> > &getBackgroundTerms() const {
		return this->background_terms;
	}

private:
	enum CsgOp {CSGT_UNION, CSGT_INTERSECTION, CSGT_DIFFERENCE, CSGT_MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
	void applyToChildren(State &state, const AbstractNode &node, CSGTermEvaluator::CsgOp op);
	shared_ptr<CSGNode> evaluateCSGTermFromGeometry(State &state, 
																									const shared_ptr<const class Geometry> &geom,
																									const class ModuleInstantiation *modinst, 
																									const AbstractNode &node);

  const AbstractNode *root;
  typedef std::list<const AbstractNode *> ChildList;
	std::map<int, ChildList> visitedchildren;

public:
	std::map<int, shared_ptr<CSGNode> > stored_term; // The term evaluated from each node index

	shared_ptr<CSGNode> root_term;
	std::vector<shared_ptr<CSGNode> > highlight_terms;
	std::vector<shared_ptr<CSGNode> > background_terms;
	const Tree &tree;
	class GeometryEvaluator *geomevaluator;
};
