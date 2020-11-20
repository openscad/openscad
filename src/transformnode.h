#pragma once

#include "node.h"
#include "linalg.h"

class TransformNode : public AbstractNode
{
public:
	VISITABLE();
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	TransformNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, const std::string &verbose_name);
	std::string toString() const override;
	std::string name() const override;
	std::string verbose_name() const override;
	Transform3d matrix;

private:
	const std::string _name;
};
