#ifndef CSGTERMEVALUATOR_H_
#define CSGTERMEVALUATOR_H_

#include <string>
#include <map>
#include <list>
#include <vector>
#include "Tree.h"
#include "visitor.h"
#include "node.h"

using std::string;
using std::map;
using std::list;
using std::vector;

class CSGTermEvaluator : public Visitor
{
public:
	CSGTermEvaluator(const Tree &tree, class PolySetEvaluator *psevaluator = NULL)
		: tree(tree), psevaluator(psevaluator) {
	}
  virtual ~CSGTermEvaluator() {}

  virtual Response visit(State &state, const AbstractNode &node);
 	virtual Response visit(State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const AbstractPolyNode &node);
 	virtual Response visit(State &state, const CsgNode &node);
 	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const ColorNode &node);
 	virtual Response visit(State &state, const RenderNode &node);
 	virtual Response visit(State &state, const CgaladvNode &node);

	class CSGTerm *evaluateCSGTerm(const AbstractNode &node,
																 vector<CSGTerm*> &highlights, 
																 vector<CSGTerm*> &background);

private:
	enum CsgOp {CSGT_UNION, CSGT_INTERSECTION, CSGT_DIFFERENCE, CSGT_MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
	void applyToChildren(const AbstractNode &node, CSGTermEvaluator::CsgOp op);

  const AbstractNode *root;
  typedef list<const AbstractNode *> ChildList;
  map<int, ChildList> visitedchildren;

public:
  map<int, class CSGTerm*> stored_term; // The term evaluated from each node index

	vector<CSGTerm*> highlights;
	vector<CSGTerm*> background;
	const Tree &tree;
	class PolySetEvaluator *psevaluator;
};

#endif
