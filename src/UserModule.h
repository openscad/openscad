#pragma once

#include <string>
#include <deque>

#include "module.h"
#include "localscope.h"

class UserModule : public AbstractModule, public ASTNode
{
public:
	UserModule(const Location &loc) : ASTNode(loc) { }
	UserModule(const class Feature& feature, const Location &loc) : AbstractModule(feature), ASTNode(loc) { }
	virtual ~UserModule() {}

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = nullptr) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
	static const std::string& stack_element(int n) { return module_stack[n]; };
	static int stack_size() { return module_stack.size(); };

	AssignmentList definition_arguments;

	LocalScope scope;

private:
	static std::deque<std::string> module_stack;
};
