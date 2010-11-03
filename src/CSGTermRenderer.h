#ifndef CSGTERMRENDERER_H_
#define CSGTERMRENDERER_H_

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

class CSGTermRenderer : public Visitor
{
public:
	CSGTermRenderer(const Tree &tree, class PolySetRenderer *psrenderer = NULL)
		: highlights(NULL), background(NULL), tree(tree), psrenderer(psrenderer) {
	}
  virtual ~CSGTermRenderer() {}

  virtual Response visit(State &state, const AbstractNode &node);
 	virtual Response visit(State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const AbstractPolyNode &node);
 	virtual Response visit(State &state, const CsgNode &node);
 	virtual Response visit(State &state, const TransformNode &node);
 	virtual Response visit(State &state, const RenderNode &node);

	class CSGTerm *renderCSGTerm(const AbstractNode &node,
															 vector<CSGTerm*> *highlights, vector<CSGTerm*> *background);

private:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
	void applyToChildren(const AbstractNode &node, CSGTermRenderer::CsgOp op);

  const AbstractNode *root;
  typedef list<const AbstractNode *> ChildList;
  map<int, ChildList> visitedchildren;

public:
  map<int, class CSGTerm*> stored_term; // The term rendered from each node index

	vector<CSGTerm*> *highlights;
	vector<CSGTerm*> *background;
	const Tree &tree;
	class PolySetRenderer *psrenderer;
};

#endif
