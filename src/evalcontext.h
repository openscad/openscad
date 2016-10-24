#pragma once

#include "context.h"
#include "Assignment.h"

/*!
  This hold the evaluation context (the parameters actually sent
	when calling a module or function, including the children).
*/
class EvalContext : public Context
{
public:
	typedef std::vector<class ModuleInstantiation *> InstanceList;

	EvalContext(const Context *parent, 
							const AssignmentList &args, const class LocalScope *const scope = NULL);
	virtual ~EvalContext() {}

	size_t numArgs() const { return this->eval_arguments.size(); }
	const std::string &getArgName(size_t i) const;
	ValuePtr getArgValue(size_t i, const Context *ctx = NULL) const;
	const AssignmentList & getArgs() const { return this->eval_arguments; }

	size_t numChildren() const;
	ModuleInstantiation *getChild(size_t i) const;

	void assignTo(Context &target) const;

#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif

private:
	const AssignmentList &eval_arguments;
	const LocalScope *const scope;
};
