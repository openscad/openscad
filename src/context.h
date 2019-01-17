#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "value.h"
#include "Assignment.h"
#include "memory.h"

class Context
{
public:
	typedef std::vector<const Context*> Stack;

	Context(const Context *parent = nullptr);
	virtual ~Context();

	const Context *getParent() const { return this->parent; }
	virtual ValuePtr evaluate_function(const std::string &name, const class EvalContext *evalctx) const;
	virtual class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, EvalContext *evalctx) const;

	void setVariables(const class EvalContext *evalctx, const AssignmentList &args, const AssignmentList &optargs={}, bool usermodule=false);

	void set_variable(const std::string &name, const ValuePtr &value);
	void set_variable(const std::string &name, const Value &value);
	void set_constant(const std::string &name, const ValuePtr &value);
	void set_constant(const std::string &name, const Value &value);

	void apply_variables(const Context &other);
	ValuePtr lookup_variable(const std::string &name, bool silent = false, const Location &loc=Location::NONE) const;
	double lookup_variable_with_default(const std::string &variable, const double &def, const Location &loc=Location::NONE) const;
	std::string lookup_variable_with_default(const std::string &variable, const std::string &def, const Location &loc=Location::NONE) const;

	bool has_local_variable(const std::string &name) const;

	void setDocumentPath(const std::string &path) { this->document_path = path; }
	const std::string &documentPath() const { return this->document_path; }
	std::string getAbsolutePath(const std::string &filename) const;
        
public:

protected:
	const Context *parent;
	Stack *ctx_stack;

	typedef std::unordered_map<std::string, ValuePtr> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;

	std::string document_path;

public:
#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
};
