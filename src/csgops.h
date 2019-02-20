#pragma once

#include "node.h"
#include "enums.h"

class CsgOpNode : public AbstractNode
{
public:
	VISITABLE();
	OpenSCADOperator type;
	CsgOpNode(const ModuleInstantiation *mi, OpenSCADOperator type) : AbstractNode(mi), type(type) { }
	std::string toString() const override;
	std::string name() const override;
};
