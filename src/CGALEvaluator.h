#ifndef CGALEVALUATOR_H_
#define CGALEVALUATOR_H_

#include "myqhash.h"
#include "visitor.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"
#include "PolySetCGALEvaluator.h"

#include <string>
#include <map>
#include <list>

using std::string;
using std::map;
using std::list;
using std::pair;

class CGALEvaluator : public Visitor
{
public:
	enum CsgOp {CGE_UNION, CGE_INTERSECTION, CGE_DIFFERENCE, CGE_MINKOWSKI};
	// FIXME: If a cache is not given, we need to fix this ourselves
	CGALEvaluator(QHash<string, CGAL_Nef_polyhedron> &cache, const Tree &tree) : cache(cache), tree(tree), psevaluator(*this) {}
  virtual ~CGALEvaluator() {}

  virtual Response visit(State &state, const AbstractNode &node);
 	virtual Response visit(State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const CsgNode &node);
 	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);
	virtual Response visit(State &state, const CgaladvNode &node);

 	CGAL_Nef_polyhedron evaluateCGALMesh(const AbstractNode &node);
	CGAL_Nef_polyhedron evaluateCGALMesh(const PolySet &polyset);

	const Tree &getTree() const { return this->tree; }

private:
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node) const;
	void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALEvaluator::CsgOp op);
	void applyToChildren(const AbstractNode &node, CGALEvaluator::CsgOp op);
	void applyHull(const CgaladvNode &node);

  string currindent;
  typedef list<pair<const AbstractNode *, string> > ChildList;
  map<int, ChildList> visitedchildren;

	QHash<string, CGAL_Nef_polyhedron> &cache;
	const Tree &tree;
public:
	// FIXME: Do we need to make this visible? Used for cache management
 // Note: psevaluator constructor needs this->tree to be initialized first
	PolySetCGALEvaluator psevaluator;
};

#endif
