#include "evalcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"

#include <boost/foreach.hpp>

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
	if (this->children.size() > 0) {
		PRINT("    children:");
		BOOST_FOREACH(const ModuleInstantiation *ch, this->children) {
			PRINTB("      %s", ch->name());
		}
	}
		
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			PRINT("  module args:");
			BOOST_FOREACH(const std::string &arg, m->argnames) {
				PRINTB("    %s = %s", arg % variables[arg]);
			}
		}
	}
}
#endif
