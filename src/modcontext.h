#pragma once

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
	ModuleContext(const Context *parent = nullptr, const EvalContext *evalctx = nullptr);
	~ModuleContext();

	void initializeModule(const class UserModule &m);
	void registerBuiltin();
	ValuePtr evaluate_function(const std::string &name, 
																										const EvalContext *evalctx) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, 
																					 EvalContext *evalctx) const override;

	const AbstractModule *findLocalModule(const std::string &name) const;
	const AbstractFunction *findLocalFunction(const std::string &name) const;

	const LocalScope::FunctionContainer *functions_p;
	const LocalScope::AbstractModuleContainer *modules_p;

  // FIXME: Points to the eval context for the call to this module. Not sure where it belongs
	const class EvalContext *evalctx;

#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
private:
// Experimental code. See issue #399
//	void evaluateAssignments(const AssignmentList &assignments);
};

class FileContext : public ModuleContext
{
public:
	FileContext(const Context *parent);
	~FileContext() {}
	void initializeModule(const FileModule &module);
	ValuePtr evaluate_function(const std::string &name, 
																		 const EvalContext *evalctx) const override;
	AbstractNode *instantiate_module(const ModuleInstantiation &inst, 
																					 EvalContext *evalctx) const override;

private:
	const FileModule::ModuleContainer *usedlibs_p;

	// This sub_* method is needed to minimize stack usage only.
	ValuePtr sub_evaluate_function(const std::string &name, 
																 const EvalContext *evalctx, 
																 FileModule *usedmod) const;
};
