#ifndef CGALRENDERER_H_
#define CGALRENDERER_H_

#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "nodecache.h"

using std::string;
using std::map;
using std::list;
using std::pair;

class CGALRenderer : public Visitor
{
public:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
	CGALRenderer(const NodeCache<string> &dumpcache) : root(NULL), dumpcache(dumpcache) {}
  virtual ~CGALRenderer() {}

  virtual Response visit(const State &state, const AbstractNode &node);
	virtual Response visit(const State &state, const AbstractIntersectionNode &node);
	virtual Response visit(const State &state, const CsgNode &node);
	virtual Response visit(const State &state, const TransformNode &node);
	virtual Response visit(const State &state, const AbstractPolyNode &node);

	string getCGALMesh() const;
// 	CGAL_Nef_polyhedron getCGALMesh() const;
private:
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node);
	QString mk_cache_id(const AbstractNode &node) const;
	void process(string &target, const string &src, CGALRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CGALRenderer::CsgOp op);

  string currindent;
  const AbstractNode *root;
  typedef list<pair<const AbstractNode *, QString> > ChildList;
  map<int, ChildList> visitedchildren;
//	hashmap<string, CGAL_Nef_polyhedron> cache;

  // For now use strings instead of Nef polyhedrons for testing caching
	QHash<QString, string> cache;
	const NodeCache<string> &dumpcache;
};

#endif
