#ifndef POLYSETEVALUATOR_H_
#define POLYSETEVALUATOR_H_

#include "memory.h"

class PolySetEvaluator
{
public:
	PolySetEvaluator(const class Tree &tree) : tree(tree) {}
	virtual ~PolySetEvaluator() {}

	const Tree &getTree() const { return this->tree; }

	virtual shared_ptr<class Geometry> getGeometry(const class AbstractNode &, bool cache);

	virtual Geometry *evaluateGeometry(const class ProjectionNode &) { return NULL; }
	virtual Geometry *evaluateGeometry(const class LinearExtrudeNode &) { return NULL; }
	virtual Geometry *evaluateGeometry(const class RotateExtrudeNode &) { return NULL; }
	virtual Geometry *evaluateGeometry(const class CgaladvNode &) { return NULL; }
	virtual Geometry *evaluateGeometry(const class RenderNode &) { return NULL; }

private:
	const Tree &tree;
};

#endif
