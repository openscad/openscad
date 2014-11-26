#pragma once

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "value.h"
#include "typedefs.h"
#include "memory.h"

class Context
{
public:
	typedef std::vector<const Context*> Stack;
	Context(const Context *parent = NULL);
	virtual ~Context();

	const Context *getParent() const { return this->parent; }
	virtual ValuePtr evaluate_function(const std::string &name, const class EvalContext *evalctx) const;
	virtual class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, EvalContext *evalctx) const;

	void setVariables(const AssignmentList &args,
										const class EvalContext *evalctx = NULL);

	void set_variable(const std::string &name, const ValuePtr &value);
	void set_variable(const std::string &name, const Value &value);
	void set_constant(const std::string &name, const ValuePtr &value);
	void set_constant(const std::string &name, const Value &value);

        void apply_variables(const Context &other);
	ValuePtr lookup_variable(const std::string &name, bool silent = false) const;
	bool has_local_variable(const std::string &name) const;

	void setDocumentPath(const std::string &path) { this->document_path = path; }
	const std::string &documentPath() const { return this->document_path; }
	std::string getAbsolutePath(const std::string &filename) const;
        
public:

protected:
	const Context *parent;
	Stack *ctx_stack;

	typedef boost::unordered_map<std::string, ValuePtr> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;

	std::string document_path; // FIXME: This is a remnant only needed by dxfdim

public:
#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
};
