#pragma once

#include <memory>
#include "context.h"
#include "SourceFile.h"
#include "UserModule.h"

class ScopeContext : public Context
{
public:
	void init() override;
	boost::optional<CallableFunction> lookup_local_function(const std::string &name, const Location &loc) const override;
	boost::optional<InstantiableModule> lookup_local_module(const std::string &name, const Location &loc) const override;

#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst) override;
#endif

protected:
	ScopeContext(const std::shared_ptr<Context> parent, const LocalScope* scope):
		Context(parent),
		scope(scope)
	{}

private:
// Experimental code. See issue #399
//	void evaluateAssignments(const AssignmentList &assignments);

	const LocalScope* scope;
};

class UserModuleContext : public ScopeContext
{
public:
	void init() override;

  // FIXME: Points to the eval context for the call to this module. Not sure where it belongs
	std::shared_ptr<EvalContext> evalctx;

protected:
	UserModuleContext(const std::shared_ptr<Context> parent, const UserModule* module, const std::shared_ptr<EvalContext> evalctx = {}):
		ScopeContext(parent, &module->scope),
		evalctx(evalctx),
		module(module)
	{}

private:
// Experimental code. See issue #399
//	void evaluateAssignments(const AssignmentList &assignments);

	const UserModule* module;

	friend class Context;
};

class FileContext : public ScopeContext
{
public:
	boost::optional<CallableFunction> lookup_local_function(const std::string &name, const Location &loc) const override;
	boost::optional<InstantiableModule> lookup_local_module(const std::string &name, const Location &loc) const override;

protected:
	FileContext(const std::shared_ptr<Context> parent, const SourceFile* source_file):
		ScopeContext(parent, &source_file->scope),
		source_file(source_file)
	{}

private:
	const SourceFile* source_file;

	friend class Context;
};
