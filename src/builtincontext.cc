#define _USE_MATH_DEFINES
#include <cmath>

#include "builtincontext.h"
#include "builtin.h"
#include "expression.h"
#include "function.h"
#include "ModuleInstantiation.h"
#include "printutils.h"
#include "evalcontext.h"
#include "boost-utils.h"

BuiltinContext::BuiltinContext() : Context()
{
}

void BuiltinContext::init()
{
	for(const auto &assignment : Builtins::instance()->getAssignments()) {
		this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(shared_from_this()));
	}

	this->set_variable("PI", M_PI);
}

boost::optional<CallableFunction> BuiltinContext::lookup_local_function(const std::string &name) const
{
	const auto &search = Builtins::instance()->getFunctions().find(name);
	if (search != Builtins::instance()->getFunctions().end()) {
		BuiltinFunction *f = search->second;
		if (f->is_enabled()) {
			return CallableFunction{f};
		}
		
		LOG(message_group::Warning,Location::NONE,"","Experimental builtin function '%1$s' is not enabled",name);
	}
	return Context::lookup_local_function(name);
}

class AbstractNode *BuiltinContext::instantiate_module(const class ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	const std::string &name = inst.name();
	const auto &search = Builtins::instance()->getModules().find(name);
	if (search != Builtins::instance()->getModules().end()) {
		AbstractModule *m = search->second;
		if (!m->is_enabled()) {
			LOG(message_group::Warning,evalctx->loc,evalctx->documentRoot(),"Experimental builtin module '%1$s' is not enabled",name);
		}
		std::string replacement = Builtins::instance()->instance()->isDeprecated(name);
		if (!replacement.empty()) {
			LOG(message_group::Deprecated,evalctx->loc,evalctx->documentRoot(),"The %1$s() module will be removed in future releases. Use %2$s instead.", std::string(name),std::string(replacement));
		}
		return m->instantiate((const_cast<BuiltinContext *>(this))->get_shared_ptr(), &inst, evalctx);
	}
	return Context::instantiate_module(inst, evalctx);
}

