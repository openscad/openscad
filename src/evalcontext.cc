#include "evalcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "localscope.h"
#include "exceptions.h"

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

void EvalContext::assignTo(Context &target) const
{
	for(const auto &assignment : this->eval_arguments) {
		if (assignment.first.empty()) {
			continue;
		}
		ValuePtr v;
		if (assignment.second) v = assignment.second->evaluate(&target);
		if (target.has_local_variable(assignment.first)) {
			PRINTB("WARNING: Ignoring duplicate variable assignment %s = %s", assignment.first % v->toString());
		} else {
			target.set_variable(assignment.first, v);
		}
	}
}

std::ostream &operator<<(std::ostream &stream, const EvalContext &ec)
{
	for (size_t i = 0; i < ec.numArgs(); i++) {
		if (i > 0) stream << ", ";
		if (!ec.getArgName(i).empty()) stream << ec.getArgName(i) << " = ";
		ValuePtr val = ec.getArgValue(i);
		if (val->type() == Value::STRING) {
			stream << '"' << val->toString() << '"';
		} else {
			stream << val->toString();
		}
	}
	return stream;
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
		for(const auto &ch : this->scope->children) {
			s << boost::format("      %s") % ch->name();
		}
	}
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			s << boost::format("  module args:");
			for(const auto &arg : m->definition_arguments) {
				s << boost::format("    %s = %s") % arg.first % *(variables[arg.first]);
			}
		}
	}
	return s.str();
}
#endif

