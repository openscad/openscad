#ifndef PROJECTIONNODE_H_
#define PROJECTIONNODE_H_

#include "node.h"
#include "visitor.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
	ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		cut_mode = false;
	}
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "projection"; }

	int convexity;
	bool cut_mode;
	virtual PolySet *evaluate_polyset(class PolySetEvaluator *evaluator) const;
};

#endif
