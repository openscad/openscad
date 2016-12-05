#pragma once

#include <unordered_map>
#include "NodeVisitor.h"
#include "enums.h"
#include "csgops.h"

class CSGSubsetBuilder : public NodeVisitor
{
public:
	CSGSubsetBuilder(const class Tree &tree, class GeometryEvaluator *geomevaluator)
		: tree(tree), geomevaluator(geomevaluator), currentNode(nullptr) {
		assert(geomevaluator);
	}
  virtual ~CSGSubsetBuilder() {}

  virtual Response visit(State &state, const class AbstractNode &node);
 	virtual Response visit(State &state, const class AbstractIntersectionNode &node);
 	virtual Response visit(State &state, const class AbstractPolyNode &node);
 	virtual Response visit(State &state, const class CsgOpNode &node);
 	virtual Response visit(State &state, const class TransformNode &node);
	virtual Response visit(State &state, const class ColorNode &node);
 	virtual Response visit(State &state, const class RenderNode &node);
 	virtual Response visit(State &state, const class CgaladvNode &node);

	const AbstractNode *buildCSG(const AbstractNode &node);

private:
	void addTransformedNode(const State &state, const AbstractNode *node, AbstractNode *newnode);
	PrimitiveNode *evaluateGeometry(const State &state, const AbstractNode &node);

	const Tree &tree;
	class GeometryEvaluator *geomevaluator;
	AbstractNode *currentNode;
	std::unordered_map<const AbstractNode*, AbstractNode*> transformed;
};
