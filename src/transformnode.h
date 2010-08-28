#ifndef TRANSFORMNODE_H_
#define TRANSFORMNODE_H_

#include "node.h"
#include "visitor.h"

class TransformNode : public AbstractNode
{
public:
	TransformNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

	double matrix[20];
};

#endif
