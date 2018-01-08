#pragma once

#include "node.h"
#include "value.h"
#include "linalg.h"

enum class CgaladvType {
	MINKOWSKI,
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
	~CgaladvNode() { }
	std::string toString() const override;
	std::string name() const override;

	ValuePtr path;
	unsigned int convexity;
	Vector3d newsize;
	Eigen::Matrix<bool,3,1> autosize;
	CgaladvType type;
};
