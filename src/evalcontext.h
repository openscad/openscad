#ifndef EVALCONTEXT_H_
#define EVALCONTEXT_H_

#include "context.h"

/*!
  This hold the evaluation context (the parameters actually sent
	when calling a module or function, including the children).
*/
class EvalContext : public Context
{
public:
	typedef std::vector<class ModuleInstantiation *> InstanceList;

	EvalContext(const Context *parent, 
							const AssignmentList &args, const class LocalScope *const scope = NULL)
		: Context(parent), eval_arguments(args), scope(scope) {}
	virtual ~EvalContext() {}

	size_t numArgs() const { return this->eval_arguments.size(); }
	const std::string &getArgName(size_t i) const;
	Value getArgValue(size_t i, const Context *ctx = NULL) const;

	size_t numChildren() const;
	ModuleInstantiation *getChild(size_t i) const;

#ifdef DEBUG
	virtual void dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif

private:
	const AssignmentList &eval_arguments;
	std::vector<std::pair<std::string, Value> > eval_values;
	const LocalScope *const scope;
};

#endif
