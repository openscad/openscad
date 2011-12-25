#ifndef COLORNODE_H_
#define COLORNODE_H_

#include "node.h"
#include "visitor.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
	ColorNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const;

	Color4f color;
};

#endif
