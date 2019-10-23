#pragma once

#include "node.h"
#include "value.h"

class ExtrudeNode : public AbstractPolyNode
{
public:
	VISITABLE();
	ExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
	}
	std::string toString() const override;
	std::string name() const override { return "extrude"; }

	int convexity;
};
