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

	~EvalContext() {}

	size_t numArgs() const { return this->eval_arguments.size(); }
	const std::string &getArgName(size_t i) const;
	Value getArgValue(size_t i, const std::shared_ptr<Context> ctx = std::shared_ptr<Context>()) const;
	const AssignmentList & getArgs() const { return this->eval_arguments; }

	AssignmentMap resolveArguments(const AssignmentList &args, const AssignmentList &optargs, bool silent) const;

	size_t numChildren() const;
	shared_ptr<ModuleInstantiation> getChild(size_t i) const;

	void assignTo(std::shared_ptr<Context> target) const;

#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif

	const Location loc;

protected:
	EvalContext(const std::shared_ptr<Context> parent, const AssignmentList &args, const Location &loc, const class LocalScope *const scope = nullptr);

private:
	const AssignmentList &eval_arguments;
	const LocalScope *const scope;

	friend class Context;
};

std::ostream &operator<<(std::ostream &stream, const EvalContext &ec);
