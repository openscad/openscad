// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include "roofnode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "fileutils.h"
#include "builtin.h"
#include "polyset.h"

#include <sstream>
#include "boost-utils.h"
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

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

	node->fn = c->lookup_variable("$fn")->toDouble();

	node->method = c->lookup_variable_with_default("method", "voronoi diagram");

	if (node->method != "voronoi diagram" && node->method != "straight skeleton") {
		node->method = "voronoi diagram";
	}

	std::cout << "SET METHOD: " << node->method << "\n" << std::flush;

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}



std::string RoofNode::toString() const
{
	std::stringstream stream;

	stream  << "roof(method = \"" << this->method << "\")";

	return stream.str();
}



void register_builtin_roof()
{
	Builtins::init("roof", new RoofModule(), {
			"roof(method = \"voronoi diagram\")"
	});
}
