#ifndef POLYSETEVALUATOR_H_
#define POLYSETEVALUATOR_H_

#include "memory.h"

class PolySetEvaluator
{
public:
	PolySetEvaluator(const class Tree &tree) : tree(tree) {}
	virtual ~PolySetEvaluator() {}

	const Tree &getTree() const { return this->tree; }

	virtual shared_ptr<class PolySet> getPolySet(const class AbstractNode &, bool cache);

	virtual PolySet *evaluatePolySet(const class ProjectionNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class LinearExtrudeNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class RotateExtrudeNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class CgaladvNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class RenderNode &) { return NULL; }

private:
	const Tree &tree;
};

#endif
