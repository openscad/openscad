#include "builtincontext.h"
#include "builtin.h"
#include "expression.h"
#include "function.h"
#include "ModuleInstantiation.h"
#include "printutils.h"

BuiltinContext::BuiltinContext()
{
	for(const auto &ass : Builtins::instance()->getAssignments()) {
		this->set_variable(ass.name, ass.expr->evaluate(this));
	}
	
	this->set_constant("PI", ValuePtr(M_PI));
}

ValuePtr BuiltinContext::evaluate_function(const std::string &name, const class EvalContext *evalctx) const
{
	const auto &search = Builtins::instance()->getFunctions().find(name);
	if (search != Builtins::instance()->getFunctions().end()) {
		AbstractFunction *f = search->second;
		if (f->is_enabled()) return f->evaluate(this, evalctx);
		else PRINTB("WARNING: Experimental builtin function '%s' is not enabled.", name);
	}
	return Context::evaluate_function(name, evalctx);
}

class AbstractNode *BuiltinContext::instantiate_module(const class ModuleInstantiation &inst, EvalContext *evalctx) const
{
	const std::string &name = inst.name();
	const auto &search = Builtins::instance()->getModules().find(name);
	if (search != Builtins::instance()->getModules().end()) {
		AbstractModule *m = search->second;
		if (!m->is_enabled()) {
			PRINTB("WARNING: Experimental builtin module '%s' is not enabled.", name);
		}
		std::string replacement = Builtins::instance()->instance()->isDeprecated(name);
		if (!replacement.empty()) {
			PRINT_DEPRECATION("The %s() module will be removed in future releases. Use %s instead.", name % replacement);
		}
		return m->instantiate(this, &inst, evalctx);
	}
	return Context::instantiate_module(inst, evalctx);
}

