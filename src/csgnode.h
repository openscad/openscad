#ifndef CSGNODE_H_
#define CSGNODE_H_

#include "node.h"
#include "visitor.h"

enum csg_type_e {
	CSG_TYPE_UNION,
	CSG_TYPE_DIFFERENCE,
	CSG_TYPE_INTERSECTION
};

class CsgNode : public AbstractNode
{
public:
	csg_type_e type;
	CsgNode(const ModuleInstantiation *mi, csg_type_e type) : AbstractNode(mi), type(type) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const;
};

#endif
