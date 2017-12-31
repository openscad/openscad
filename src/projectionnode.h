#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
	VISITABLE();
	ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		cut_mode = false;
	}
	std::string toString() const override;
	std::string name() const override { return "projection"; }

	int convexity;
	bool cut_mode;
};
