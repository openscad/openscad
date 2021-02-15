// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include <sstream>

#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "fileutils.h"
#include "builtin.h"
#include "roofnode.h"

class RoofModule : public AbstractModule
{
public:
	RoofModule() { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *RoofModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new RoofNode(inst, evalctx);

	AssignmentList args{assignment("method")};

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args);
	inst->scope.apply(evalctx);

	node->fa = c->lookup_variable("$fa").toDouble();
	node->fs = c->lookup_variable("$fs").toDouble();
	node->fn = c->lookup_variable("$fn").toDouble();

	node->fa = std::max(node->fa, 0.01);
	node->fs = std::max(node->fs, 0.01);
	if (node->fn > 0) {
		node->fa = 360.0 / node->fn;
		node->fs = 0.0;
	}

	node->method = c->lookup_variable_with_default("method", "voronoi diagram");

	if (node->method != "voronoi diagram" && node->method != "straight skeleton") {
		node->method = "voronoi diagram";
	}

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string RoofNode::toString() const
{
	std::stringstream stream;

	stream  << "roof(method = \"" << this->method << "\""
		<< ", $fa = " << this->fa
		<< ", $fs = " << this->fs
		<< ", $fn = " << this->fn
		<< ")";

	return stream.str();
}

void register_builtin_roof()
{
	Builtins::init("roof", new RoofModule(), {
			"roof(method = \"voronoi diagram\")"
	});
}
