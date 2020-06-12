#pragma once

#include "AST.h"
#include "Assignment.h"
#include <unordered_map>

class LocalScope
{
public:
	LocalScope();
	~LocalScope();

	size_t numElements() const { return assignments.size() + children.size(); }
	void print(std::ostream &stream, const std::string &indent, const bool inlined = false) const;
	std::vector<class AbstractNode*> instantiateChildren(const std::shared_ptr<Context> &evalctx) const;
	void addChild(shared_ptr<ASTNode> astnode);
private:
	void addModuleInst(shared_ptr<class ModuleInstantiation> &&astnode);
	void addModule(shared_ptr<class UserModule> &&module);
	void addFunction(shared_ptr<class UserFunction> &&function);
	void addAssignment(shared_ptr<class Assignment> &&assignment);
public:
	void apply(const std::shared_ptr<Context> &ctx) const;
	bool hasChildren() const {return !(children.empty());};

	std::vector<shared_ptr<ASTNode>> children;

	AssignmentList assignments;
	std::vector<shared_ptr<ModuleInstantiation>> children_inst;

	// Modules and functions are stored twice; once for lookup and once for AST serialization
	// FIXME: Should we split this class into an ASTNode and a run-time support class?
	typedef std::unordered_map<std::string, shared_ptr<UserFunction>> FunctionContainer;
	FunctionContainer functions;
	std::vector<std::pair<std::string, shared_ptr<UserFunction>>> astFunctions;

	typedef std::unordered_map<std::string, shared_ptr<UserModule>> ModuleContainer;
	ModuleContainer	modules;
	std::vector<std::pair<std::string, shared_ptr<UserModule>>> astModules;
};
