#pragma once

#include "node.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
	VISITABLE();
	ColorNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
	virtual std::string toString() const;
	virtual std::string name() const;

	Color4f color;
};
