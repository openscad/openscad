#ifndef GEOMETRYEVALUATOR_H_
#define GEOMETRYEVALUATOR_H_

#include "visitor.h"
#include "enums.h"
#include "memory.h"

#include <utility>
#include <list>
#include <map>

class GeometryEvaluator : public Visitor
{
public:
	GeometryEvaluator(const class Tree &tree);
	virtual ~GeometryEvaluator() {}

	shared_ptr<const class Geometry> getGeometry(const AbstractNode &node, bool cache);
	shared_ptr<const class Geometry> evaluateGeometry(const AbstractNode &node);

	virtual Response visit(State &state, const AbstractNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);
	virtual Response visit(State &state, const LeafNode &node);
	virtual Response visit(State &state, const TransformNode &node);

	const Tree &getTree() const { return this->tree; }

private:
	bool isCached(const AbstractNode &node) const;
	Geometry *applyToChildren(const AbstractNode &node, OpenSCADOperator op);
	void addToParent(const State &state, const AbstractNode &node, const shared_ptr<const Geometry> &geom);

  typedef std::pair<const AbstractNode *, shared_ptr<const Geometry> > ChildItem;
  typedef std::list<ChildItem> ChildList;
	std::map<int, ChildList> visitedchildren;
	const Tree &tree;
	shared_ptr<const Geometry> root;

public:
// FIXME: Deal with visibility
	class CGALEvaluator *cgalevaluator;
};


#endif
