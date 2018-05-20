#pragma once

#include <string>
#include <deque>

#include "module.h"
#include "localscope.h"

class UserModule : public AbstractModule, public ASTNode
{
public:
	UserModule(const char *name, const Location &loc) : ASTNode(loc), name(name) { }
	UserModule(const char *name, const class Feature& feature, const Location &loc) : AbstractModule(feature), ASTNode(loc), name(name) { }
	~UserModule() {}

	AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = nullptr) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	static const std::string& stack_element(int n) { return module_stack[n]; };
	static int stack_size() { return module_stack.size(); };

	std::string name;
	AssignmentList definition_arguments;
	LocalScope scope;

private:
	static std::deque<std::string> module_stack;
};
