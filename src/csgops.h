#pragma once

#include "node.h"
#include "enums.h"

class CsgOpNode : public AbstractNode
{
public:
	VISITABLE();
	OpenSCADOperator type;
	CsgOpNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, OpenSCADOperator type) : AbstractNode(mi, ctx), type(type) { }
	std::string toString() const override;
	std::string name() const override;
};
