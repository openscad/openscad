#pragma once

#include <string>
#include <memory>

#include "context.h"

class BuiltinContext : public Context
{
public:
	~BuiltinContext() {}

	void init() override;
	boost::optional<CallableFunction> lookup_local_function(const std::string &name, const Location &loc) const override;
	boost::optional<InstantiableModule> lookup_local_module(const std::string &name, const Location &loc) const override;

protected:
	BuiltinContext(EvaluationSession* session);

private:
	std::shared_ptr<BuiltinContext> self;

	friend class Context;
};
