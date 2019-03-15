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
	Value evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const override;

	shared_ptr<const UserModule> findLocalModule(const std::string &name) const;
	shared_ptr<const UserFunction> findLocalFunction(const std::string &name) const;

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
	Value evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const override;

protected:
	FileContext(const std::shared_ptr<Context> parent) : ModuleContext(parent), usedlibs_p(nullptr) {}

private:
	const FileModule::ModuleContainer *usedlibs_p;

	// This sub_* method is needed to minimize stack usage only.
	Value sub_evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx, FileModule *usedmod) const;

	friend class Context;
};
