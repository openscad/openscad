#ifndef CGALEVALUATOR_H_
#define CGALEVALUATOR_H_

#include "visitor.h"
#include "CGAL_Nef_polyhedron.h"
#include "PolySetCGALEvaluator.h"

#include <string>
#include <map>
#include <list>

class CGALEvaluator : public Visitor
{
public:
	enum CsgOp {CGE_UNION, CGE_INTERSECTION, CGE_DIFFERENCE, CGE_MINKOWSKI};
	CGALEvaluator(const class Tree &tree) : tree(tree), psevaluator(*this) {}
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
  void addToParent(const State &state, const AbstractNode &node, const CGAL_Nef_polyhedron &N);
  bool isCached(const AbstractNode &node) const;
	void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALEvaluator::CsgOp op);
	CGAL_Nef_polyhedron applyToChildren(const AbstractNode &node, CGALEvaluator::CsgOp op);
	CGAL_Nef_polyhedron applyHull(const CgaladvNode &node);
	CGAL_Nef_polyhedron applyResize(const CgaladvNode &node);

	std::string currindent;
  typedef std::pair<const AbstractNode *, CGAL_Nef_polyhedron> ChildItem;
  typedef std::list<ChildItem> ChildList;
	std::map<int, ChildList> visitedchildren;

	const Tree &tree;
	CGAL_Nef_polyhedron root;
public:
	// FIXME: Do we need to make this visible? Used for cache management
 // Note: psevaluator constructor needs this->tree to be initialized first
	PolySetCGALEvaluator psevaluator;
};

#endif
