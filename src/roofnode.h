#pragma once

#include "node.h"
#include "value.h"

class RoofNode : public AbstractPolyNode
{
public:
	VISITABLE();
	RoofNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx) : AbstractPolyNode(mi, ctx) {
	}
	std::string toString() const override;
	std::string name() const override { return "roof"; }
};
