#include "modcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "ModuleCache.h"

#include <boost/foreach.hpp>

ModuleContext::ModuleContext(const Context *parent, const EvalContext *evalctx)
	: Context(parent), functions_p(NULL), modules_p(NULL), evalctx(evalctx)
{
}

ModuleContext::~ModuleContext()
{
}

void ModuleContext::initializeModule(const class Module &module)
{
	this->setVariables(module.definition_arguments, evalctx);
	// FIXME: Don't access module members directly
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	BOOST_FOREACH(const Assignment &ass, module.scope.assignments) {
		this->set_variable(ass.first, ass.second->evaluate(this));
	}
}

/*!
	Only used to initialize builtins for the top-level root context
*/
void ModuleContext::registerBuiltin()
{
	const LocalScope &scope = Builtins::instance()->getGlobalScope();

	// FIXME: Don't access module members directly
	this->functions_p = &scope.functions;
	this->modules_p = &scope.modules;
	BOOST_FOREACH(const Assignment &ass, scope.assignments) {
		this->set_variable(ass.first, ass.second->evaluate(this));
	}

	this->set_constant("PI",Value(M_PI));
}

const AbstractFunction *ModuleContext::findLocalFunction(const std::string &name) const
{
	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end()) {
		return this->functions_p->find(name)->second;
	}
	return NULL;
}

const AbstractModule *ModuleContext::findLocalModule(const std::string &name) const
{
	if (this->modules_p && this->modules_p->find(name) != this->modules_p->end()) {
		AbstractModule *m = this->modules_p->find(name)->second;
		std::string replacement = Builtins::instance()->isDeprecated(name);
		if (!replacement.empty()) {
			PRINTB("DEPRECATED: The %s() module will be removed in future releases. Use %s() instead.", name % replacement);
		}
		return m;
	}
	return NULL;
}

Value ModuleContext::evaluate_function(const std::string &name, const EvalContext *evalctx) const
{
	const AbstractFunction *foundf = findLocalFunction(name);
	if (foundf) return foundf->evaluate(this, evalctx);

	return Context::evaluate_function(name, evalctx);
}

AbstractNode *ModuleContext::instantiate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const
{
	const AbstractModule *foundm = this->findLocalModule(inst.name());
	if (foundm) return foundm->instantiate(this, &inst, evalctx);

	return Context::instantiate_module(inst, evalctx);
}

#ifdef DEBUG
void ModuleContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	if (inst) 
		PRINTB("ModuleContext %p (%p) for %s inst (%p) ", this % this->parent % inst->name() % inst);
	else 
		PRINTB("ModuleContext: %p (%p)", this % this->parent);
	PRINTB("  document path: %s", this->document_path);
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			PRINT("  module args:");
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				PRINTB("    %s = %s", arg.first % variables[arg.first]);
			}
		}
	}
	typedef std::pair<std::string, Value> ValueMapType;
	PRINT("  vars:");
  BOOST_FOREACH(const ValueMapType &v, constants) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, config_variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		

}
#endif

FileContext::FileContext(const class FileModule &module, const Context *parent)
	: usedlibs(module.usedlibs), ModuleContext(parent)
{
	if (!module.modulePath().empty()) this->document_path = module.modulePath();
}

Value FileContext::evaluate_function(const std::string &name, const EvalContext *evalctx) const
{
	const AbstractFunction *foundf = findLocalFunction(name);
	if (foundf) return foundf->evaluate(this, evalctx);
	
	BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, this->usedlibs) {
		// usedmod is NULL if the library wasn't be compiled (error or file-not-found)
		FileModule *usedmod = ModuleCache::instance()->lookup(m);
		if (usedmod && 
				usedmod->scope.functions.find(name) != usedmod->scope.functions.end()) {
			FileContext ctx(*usedmod, this->parent);
			ctx.initializeModule(*usedmod);
			// FIXME: Set document path
#if 0 && DEBUG
			PRINTB("New lib Context for %s func:", name);
			ctx.dump(NULL, NULL);
#endif
			return usedmod->scope.functions[name]->evaluate(&ctx, evalctx);
		}
	}

	return ModuleContext::evaluate_function(name, evalctx);
}

AbstractNode *FileContext::instantiate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const
{
	const AbstractModule *foundm = this->findLocalModule(inst.name());
	if (foundm) return foundm->instantiate(this, &inst, evalctx);

	BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, this->usedlibs) {
		FileModule *usedmod = ModuleCache::instance()->lookup(m);
		// usedmod is NULL if the library wasn't be compiled (error or file-not-found)
		if (usedmod && 
				usedmod->scope.modules.find(inst.name()) != usedmod->scope.modules.end()) {
			FileContext ctx(*usedmod, this->parent);
			ctx.initializeModule(*usedmod);
			// FIXME: Set document path
#if 0 && DEBUG
			PRINT("New file Context:");
			ctx.dump(NULL, &inst);
#endif
			return usedmod->scope.modules[inst.name()]->instantiate(&ctx, &inst, evalctx);
		}
	}

	return ModuleContext::instantiate_module(inst, evalctx);
}
