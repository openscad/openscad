#ifndef CSGTEXTRENDERER_H_
#define CSGTEXTRENDERER_H_

#include "visitor.h"
#include "CSGTextCache.h"

#include <map>
#include <list>

using std::string;
using std::map;
using std::list;

class CSGTextRenderer : public Visitor
{
public:
	CSGTextRenderer(CSGTextCache &cache) : cache(cache) {}
  virtual ~CSGTextRenderer() {}

  virtual Response visit(State &state, const AbstractNode &node);
	virtual Response visit(State &state, const AbstractIntersectionNode &node);
	virtual Response visit(State &state, const CsgNode &node);
	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);

private:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node);
	void process(string &target, const string &src, CSGTextRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CSGTextRenderer::CsgOp op);

  string currindent;
  typedef list<const AbstractNode *> ChildList;
  map<int, ChildList> visitedchildren;

	CSGTextCache &cache;
};

#endif
