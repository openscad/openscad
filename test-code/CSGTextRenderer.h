#ifndef CSGTEXTRENDERER_H_
#define CSGTEXTRENDERER_H_

#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "nodecache.h"

using std::string;
using std::map;
using std::list;
using std::pair;

class CSGTextRenderer : public Visitor
{
public:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
	CSGTextRenderer(const NodeCache<string> &dumpcache) : root(NULL), dumpcache(dumpcache) {}
  virtual ~CSGTextRenderer() {}

  virtual Response visit(const State &state, const AbstractNode &node);
	virtual Response visit(const State &state, const AbstractIntersectionNode &node);
	virtual Response visit(const State &state, const CsgNode &node);
	virtual Response visit(const State &state, const TransformNode &node);
	virtual Response visit(const State &state, const AbstractPolyNode &node);

	string getCSGString() const;
private:
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node);
	QString mk_cache_id(const AbstractNode &node) const;
	void process(string &target, const string &src, CSGTextRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CSGTextRenderer::CsgOp op);

  string currindent;
  const AbstractNode *root;
  typedef list<pair<const AbstractNode *, QString> > ChildList;
  map<int, ChildList> visitedchildren;

	QHash<QString, string> cache;
	const NodeCache<string> &dumpcache;
};

#endif
