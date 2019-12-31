#pragma once

#include "AST.h"
#include "Assignment.h"
#include <unordered_map>

class LocalScope : public ASTNode
{
public:
	LocalScope(const Location &loc);
	~LocalScope();

	void print(std::ostream &stream, const std::string &indent) const override;

	size_t numElements() const { return assignments.size() + children.size(); }
	std::vector<class AbstractNode*> instantiateChildren(const std::shared_ptr<Context> evalctx) const;
	void addChild(ASTNode *astnode);
	void addModuleInst(class ModuleInstantiation *astnode);
	void addModule(const std::string &name, class UserModule *module);
	void addFunction(class UserFunction *function);
	void addAssignment(class Assignment *assignment);
	void apply(const std::shared_ptr<Context> ctx) const;
	bool hasChildren() const {return !(children.empty());};
	void resolveAssignments();

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
