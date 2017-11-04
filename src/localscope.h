#pragma once

#include "Assignment.h"
#include <unordered_map>

class LocalScope
{
public:
	LocalScope();
	~LocalScope();

	size_t numElements() const { return assignments.size() + children.size(); }
	std::string dump(const std::string &indent) const;
	std::vector<class AbstractNode *> instantiateChildren(const class Context *evalctx) const;
	void addChild(class ModuleInstantiation *astnode);
	void addModule(const std::string &name, class UserModule *module);
	void addFunction(class UserFunction *function);
	void addAssignment(const class Assignment &ass);
	void apply(Context &ctx) const;

	AssignmentList assignments;
	std::vector<ModuleInstantiation *> children;

	// Modules and functions are stored twice; once for lookup and once for AST serialization
	typedef std::unordered_map<std::string, class AbstractFunction *> FunctionContainer;
	FunctionContainer functions;
	std::vector<std::pair<std::string, AbstractFunction *>> astFunctions;

	typedef std::unordered_map<std::string, class AbstractModule *> AbstractModuleContainer;
	AbstractModuleContainer modules;
	std::vector<std::pair<std::string, AbstractModule *>> astModules;
};
