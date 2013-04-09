#include "modcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"

#include <boost/foreach.hpp>

ModuleContext::ModuleContext(const class Module *module, const Context *parent, const EvalContext *evalctx)
	: Context(parent)
{
	if (module) {
		setModule(*module, evalctx);
	}
	else {
		this->functions_p = NULL;
		this->modules_p = NULL;
		this->usedlibs_p = NULL;
	}
}

ModuleContext::~ModuleContext()
{
}

void ModuleContext::setModule(const Module &module, const EvalContext *evalctx)
{
	this->setVariables(module.argnames, module.argexpr, evalctx);
	this->evalctx = evalctx;

	// FIXME: Don't access module members directly
	this->functions_p = &module.functions;
	this->modules_p = &module.modules;
	this->usedlibs_p = &module.usedlibs;
	BOOST_FOREACH(const std::string &var, module.assignments_var) {
		this->set_variable(var, module.assignments.at(var)->evaluate(this));
	}
	
	if (!module.modulePath().empty()) this->document_path = module.modulePath();
}

/*!
	Only used to initialize builtins for the top-level root context
*/
void ModuleContext::registerBuiltin()
{
	this->setModule(Builtins::instance()->getRootModule());
	this->set_constant("PI",Value(M_PI));
}

Value ModuleContext::evaluate_function(const std::string &name, const EvalContext *evalctx) const
{
	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end()) {
		return this->functions_p->find(name)->second->evaluate(this, evalctx);
	}
	
	if (this->usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *this->usedlibs_p) {
			if (m.second->functions.find(name) != m.second->functions.end()) {
				ModuleContext ctx(m.second, this->parent);
				// FIXME: Set document path
#if 0 && DEBUG
				PRINTB("New lib Context for %s func:", name);
				ctx.dump(NULL, NULL);
#endif
				return m.second->functions[name]->evaluate(&ctx, evalctx);
			}
		}
	}
	return Context::evaluate_function(name, evalctx);
}

AbstractNode *ModuleContext::evaluate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const
{
	if (this->modules_p && this->modules_p->find(inst.name()) != this->modules_p->end()) {
		AbstractModule *m = this->modules_p->find(inst.name())->second;
		std::string replacement = Builtins::instance()->isDeprecated(inst.name());
		if (!replacement.empty()) {
			PRINTB("DEPRECATED: The %s() module will be removed in future releases. Use %s() instead.", inst.name() % replacement);
		}
		return m->evaluate(this, &inst, evalctx);
	}

	if (this->usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *this->usedlibs_p) {
			assert(m.second);
			if (m.second->modules.find(inst.name()) != m.second->modules.end()) {
				ModuleContext ctx(m.second, this->parent);
				// FIXME: Set document path
#if 0 && DEBUG
				PRINT("New lib Context:");
				ctx.dump(NULL, &inst);
#endif
				return m.second->modules[inst.name()]->evaluate(&ctx, &inst, evalctx);
			}
		}
	}

	return Context::evaluate_module(inst, evalctx);
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
			BOOST_FOREACH(const std::string &arg, m->argnames) {
				PRINTB("    %s = %s", arg % variables[arg]);
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
