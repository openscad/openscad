#ifndef CGALEVALUATOR_H_
#define CGALEVALUATOR_H_

#include "visitor.h"
#include "enums.h"
#include "CGAL_Nef_polyhedron.h"

#include <string>
#include <map>
#include <list>

class CGALEvaluator : public Visitor
{
public:
	enum CsgOp {CGE_UNION, CGE_INTERSECTION, CGE_DIFFERENCE, CGE_MINKOWSKI};
	CGALEvaluator(const class Tree &tree, class GeometryEvaluator &geomevaluator) :
		tree(tree), geomevaluator(geomevaluator) {}
  virtual ~CGALEvaluator() {}

  virtual Response visit(State &state, const AbstractNode &node);
 	virtual Response visit(State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const CsgNode &node);
 	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);
	virtual Response visit(State &state, const CgaladvNode &node);

 	shared_ptr<const CGAL_Nef_polyhedron> evaluateCGALMesh(const AbstractNode &node);

	const Tree &getTree() const { return this->tree; }

private:
  void addToParent(const State &state, const AbstractNode &node, const shared_ptr<const CGAL_Nef_polyhedron> &N);
  bool isCached(const AbstractNode &node) const;
	void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, OpenSCADOperator op);
	CGAL_Nef_polyhedron *applyToChildren(const AbstractNode &node, OpenSCADOperator op);
	const CGAL_Nef_polyhedron *applyHull(const CgaladvNode &node);
	const CGAL_Nef_polyhedron *applyResize(const CgaladvNode &node);

  typedef std::pair<const AbstractNode *, shared_ptr<const CGAL_Nef_polyhedron> > ChildItem;
  typedef std::list<ChildItem> ChildList;
	std::map<int, ChildList> visitedchildren;

	const Tree &tree;
	shared_ptr<const CGAL_Nef_polyhedron> root;
public:
	// FIXME: Do we need to make this visible? Used for cache management
 // Note: psevaluator constructor needs this->tree to be initialized first
	class GeometryEvaluator &geomevaluator;
};

#endif
