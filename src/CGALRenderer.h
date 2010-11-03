#ifndef CGALRENDERER_H_
#define CGALRENDERER_H_

#include "myqhash.h"
#include "visitor.h"
#include "Tree.h"
#include "cgal.h"
#include "PolySetCGALRenderer.h"

#include <string>
#include <map>
#include <list>

extern CGAL_Nef_polyhedron3 minkowski3(CGAL_Nef_polyhedron3 a, CGAL_Nef_polyhedron3 b);
extern CGAL_Nef_polyhedron2 minkowski2(CGAL_Nef_polyhedron2 a, CGAL_Nef_polyhedron2 b);

using std::string;
using std::map;
using std::list;
using std::pair;

class CGALRenderer : public Visitor
{
public:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
	// FIXME: If a cache is not given, we need to fix this ourselves
	CGALRenderer(QHash<string, CGAL_Nef_polyhedron> &cache, const Tree &tree) : cache(cache), tree(tree), psrenderer(*this) {}
  virtual ~CGALRenderer() {}

  virtual Response visit(State &state, const AbstractNode &node);
 	virtual Response visit(State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const CsgNode &node);
 	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);

 	CGAL_Nef_polyhedron renderCGALMesh(const AbstractNode &node);
	CGAL_Nef_polyhedron renderCGALMesh(const PolySet &polyset);

	const Tree &getTree() const { return this->tree; }

private:
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node) const;
	void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CGALRenderer::CsgOp op);

  string currindent;
  typedef list<pair<const AbstractNode *, string> > ChildList;
  map<int, ChildList> visitedchildren;

	QHash<string, CGAL_Nef_polyhedron> &cache;
	const Tree &tree;
	PolySetCGALRenderer psrenderer;
};

#endif
