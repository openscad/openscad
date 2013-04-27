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
	EvalContext(const Context *parent = NULL) : Context(parent) {}
	virtual ~EvalContext() {}

	std::vector<std::pair<std::string, Value> > eval_arguments;
	std::vector<class ModuleInstantiation *> children;

#ifdef DEBUG
	virtual void dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
};

#endif
