#pragma once

#include <memory>
#include "context.h"
#include "FileModule.h"

/*!
	This holds the context for a UserModule definition; keeps track of
	global variables, submodules and functions defined inside a module.

	NB! every .scad file defines a FileModule holding the contents of the file.
*/
class ModuleContext : public Context
{
public:
	~ModuleContext();

	void initializeModule(const class UserModule &m);
	boost::optional<CallableFunction> lookup_local_function(const std::string &name) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const override;

	shared_ptr<const UserModule> findLocalModule(const std::string &name) const;

	const LocalScope::FunctionContainer *functions_p;
	const LocalScope::ModuleContainer *modules_p;

  // FIXME: Points to the eval context for the call to this module. Not sure where it belongs
	std::shared_ptr<EvalContext> evalctx;

#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst) override;
#endif

protected:
	ModuleContext(const std::shared_ptr<Context> parent, const std::shared_ptr<EvalContext> evalctx = {});

private:
// Experimental code. See issue #399
//	void evaluateAssignments(const AssignmentList &assignments);

	friend class Context;
};

class FileContext : public ModuleContext
{
public:
	~FileContext() {}
	void initializeModule(const FileModule &module);
	boost::optional<CallableFunction> lookup_local_function(const std::string &name) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const override;

protected:
	FileContext(const std::shared_ptr<Context> parent) : ModuleContext(parent), usedlibs_p(nullptr) {}

private:
	const FileModule::ModuleContainer *usedlibs_p;

	friend class Context;
};
