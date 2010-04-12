#ifndef CSGTEXTRENDERER_H_
#define CSGTEXTRENDERER_H_

#include <qglobal.h>
#include <string>
extern uint qHash(const std::string &);

#include <map>
#include <list>
#include <QHash>
#include "visitor.h"
#include "Tree.h"

using std::string;
using std::map;
using std::list;
using std::pair;

class CSGTextRenderer : public Visitor
{
public:
	CSGTextRenderer(QHash<string, string> &cache, const Tree &tree) : 
		cache(cache), tree(tree) {}
  virtual ~CSGTextRenderer() {}

  virtual Response visit(const State &state, const AbstractNode &node);
	virtual Response visit(const State &state, const AbstractIntersectionNode &node);
	virtual Response visit(const State &state, const CsgNode &node);
	virtual Response visit(const State &state, const TransformNode &node);
	virtual Response visit(const State &state, const AbstractPolyNode &node);

private:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node);
	void process(string &target, const string &src, CSGTextRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CSGTextRenderer::CsgOp op);

  string currindent;
  typedef list<pair<const AbstractNode *, string> > ChildList;
  map<int, ChildList> visitedchildren;

	QHash<string, string> &cache;
	const Tree &tree;
};

#endif
