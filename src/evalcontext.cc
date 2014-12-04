#include "evalcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "localscope.h"
#include "exceptions.h"

#include <boost/foreach.hpp>

EvalContext::EvalContext(const Context *parent, 
												 const AssignmentList &args, const class LocalScope *const scope)
	: Context(parent), eval_arguments(args), scope(scope)
{
}

const std::string &EvalContext::getArgName(size_t i) const
{
	assert(i < this->eval_arguments.size());
	return this->eval_arguments[i].first;
}

ValuePtr EvalContext::getArgValue(size_t i, const Context *ctx) const
{
	assert(i < this->eval_arguments.size());
	const Assignment &arg = this->eval_arguments[i];
	ValuePtr v;
	if (arg.second) {
		v = arg.second->evaluate(ctx ? ctx : this);
	}
	return v;
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
std::string EvalContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	std::stringstream s;
	if (inst)
		s << boost::format("EvalContext %p (%p) for %s inst (%p)") % this % this->parent % inst->name() % inst;
	else
		s << boost::format("Context: %p (%p)") % this % this->parent;
	s << boost::format("  document path: %s") % this->document_path;

	s << boost::format("  eval args:");
	for (size_t i=0;i<this->eval_arguments.size();i++) {
		s << boost::format("    %s = %s") % this->eval_arguments[i].first % this->eval_arguments[i].second;
	}
	if (this->scope && this->scope->children.size() > 0) {
		s << boost::format("    children:");
		BOOST_FOREACH(const ModuleInstantiation *ch, this->scope->children) {
			s << boost::format("      %s") % ch->name();
		}
	}
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			s << boost::format("  module args:");
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				s << boost::format("    %s = %s") % arg.first % *(variables[arg.first]);
			}
		}
	}
	return s.str();
}
#endif

