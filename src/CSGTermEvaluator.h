#ifndef CSGTERMEVALUATOR_H_
#define CSGTERMEVALUATOR_H_

#include <map>
#include <list>
#include <vector>
#include "visitor.h"

class CSGTermEvaluator : public Visitor
{
public:
	CSGTermEvaluator(const class Tree &tree, class PolySetEvaluator *psevaluator = NULL)
		: tree(tree), psevaluator(psevaluator) {
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

	class CSGTerm *evaluateCSGTerm(const AbstractNode &node,
																 std::vector<CSGTerm*> &highlights, 
																 std::vector<CSGTerm*> &background);

private:
	enum CsgOp {CSGT_UNION, CSGT_INTERSECTION, CSGT_DIFFERENCE, CSGT_MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
	void applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op);

  const AbstractNode *root;
  typedef std::list<const AbstractNode *> ChildList;
	std::map<int, ChildList> visitedchildren;

public:
	std::map<int, class CSGTerm*> stored_term; // The term evaluated from each node index

	std::vector<CSGTerm*> highlights;
	std::vector<CSGTerm*> background;
	const Tree &tree;
	class PolySetEvaluator *psevaluator;
};

#endif
