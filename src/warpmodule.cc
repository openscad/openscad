// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include <sstream>

#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "fileutils.h"
#include "builtin.h"
#include "warpnode.h"

class WarpModule : public AbstractModule
{
public:
	WarpModule() { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *WarpModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new WarpNode(inst, evalctx);

	AssignmentList args{assignment("grid_x"), assignment("grid_y"), assignment("grid_z"), assignment("R"), assignment("r")};

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args);
	inst->scope.apply(evalctx);

	node->grid_x = std::max(0.0, c->lookup_variable_with_default("grid_x", 0.0));
	node->grid_y = std::max(0.0, c->lookup_variable_with_default("grid_y", 0.0));
	node->grid_z = std::max(0.0, c->lookup_variable_with_default("grid_z", 0.0));
	
        node->R = c->lookup_variable_with_default("R", 10);
        node->r = c->lookup_variable_with_default("r", 4);

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string WarpNode::toString() const
{
	std::stringstream stream;

	stream  << "warp(grid_x = " << this->grid_x
		<< ", grid_y = " << this->grid_y
		<< ", grid_z = " << this->grid_z
		<< ", R = " << this->R
		<< ", r = " << this->r
		<< ")";

	return stream.str();
}

void register_builtin_warp()
{
	Builtins::init("warp", new WarpModule(), {
			"warp(grid_x = 0, grid_y = 0, grid_z = 0, R = 10, r = 4)"
	});
}
