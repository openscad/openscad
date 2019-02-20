#pragma once

#include "node.h"
#include "linalg.h"

class TransformNode : public AbstractNode
{
public:
	VISITABLE();
	TransformNode(const ModuleInstantiation *mi);
	std::string toString() const override;
	std::string name() const override;

	Transform3d matrix;
};
