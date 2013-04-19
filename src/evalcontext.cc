#include "evalcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "localscope.h"

#include <boost/foreach.hpp>

const std::string &EvalContext::getArgName(size_t i) const
{
	assert(i < this->eval_arguments.size());
	return this->eval_arguments[i].first;
}

Value EvalContext::getArgValue(size_t i, const Context *ctx) const
{
	assert(i < this->eval_arguments.size());
	const Assignment &arg = this->eval_arguments[i];
	return arg.second ? arg.second->evaluate(ctx ? ctx : this) : Value();
}

size_t EvalContext::numChildren() const
{
	return this->scope ? this->scope->children.size() : 0;
}

ModuleInstantiation *EvalContext::getChild(size_t i) const
{
	return this->scope ? this->scope->children[i] : NULL; 
}

#ifdef DEBUG
void EvalContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	if (inst) 
		PRINTB("EvalContext %p (%p) for %s inst (%p)", this % this->parent % inst->name() % inst);
	else 
		PRINTB("Context: %p (%p)", this % this->parent);
	PRINTB("  document path: %s", this->document_path);

	PRINT("  eval args:");
	for (int i=0;i<this->eval_arguments.size();i++) {
		PRINTB("    %s = %s", this->eval_arguments[i].first % this->eval_arguments[i].second);
	}
	if (this->scope && this->scope->children.size() > 0) {
		PRINT("    children:");
		BOOST_FOREACH(const ModuleInstantiation *ch, this->scope->children) {
			PRINTB("      %s", ch->name());
		}
	}
		
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			PRINT("  module args:");
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				PRINTB("    %s = %s", arg.first % variables[arg.first]);
			}
		}
	}
}
#endif
