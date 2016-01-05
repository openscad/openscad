#pragma once

#include "node.h"
#include "visitor.h"
#include "enums.h"

class CsgOpNode : public AbstractNode
{
public:
	OpenSCADOperator type;
	CsgOpNode(const ModuleInstantiation *mi, OpenSCADOperator type) : AbstractNode(mi), type(type) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const;
};
