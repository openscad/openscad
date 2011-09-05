#ifndef CGALADVNODE_H_
#define CGALADVNODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"

enum cgaladv_type_e {
	MINKOWSKI,
	GLIDE,
	SUBDIV,
	HULL
};

class CgaladvNode : public AbstractNode
{
public:
	CgaladvNode(const ModuleInstantiation *mi, cgaladv_type_e type) : AbstractNode(mi), type(type) {
		convexity = 1;
	}
	virtual ~CgaladvNode() { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const;

	Value path;
	std::string subdiv_type;
	int convexity, level;
	cgaladv_type_e type;
};

#endif
