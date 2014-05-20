#pragma once

#include "node.h"
#include "visitor.h"
#include <string>

class RenderNode : public AbstractNode
{
public:
	RenderNode(const ModuleInstantiation *mi) : AbstractNode(mi), convexity(1) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "render"; }

	int convexity;
};
