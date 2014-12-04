#pragma once

#include "node.h"
#include "visitor.h"
#include "value.h"
#include "linalg.h"

enum cgaladv_type_e {
	MINKOWSKI,
	GLIDE,
	SUBDIV,
	HULL,
	RESIZE
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

	ValuePtr path;
	std::string subdiv_type;
	int convexity, level;
	Vector3d newsize;
	Eigen::Matrix<bool,3,1> autosize;
	cgaladv_type_e type;
};
