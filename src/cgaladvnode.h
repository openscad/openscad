#pragma once

#include "node.h"
#include "value.h"
#include "linalg.h"

enum class CgaladvType {
	MINKOWSKI,
	GLIDE,
	SUBDIV,
	HULL,
	RESIZE
};

class CgaladvNode : public AbstractNode
{
public:
	VISITABLE();
	CgaladvNode(const ModuleInstantiation *mi, CgaladvType type) : AbstractNode(mi), type(type) {
		convexity = 1;
	}
	virtual ~CgaladvNode() { }
	virtual std::string toString() const;
	virtual std::string name() const;

	ValuePtr path;
	std::string subdiv_type;
	int convexity, level;
	Vector3d newsize;
	Eigen::Matrix<bool,3,1> autosize;
	CgaladvType type;
};
