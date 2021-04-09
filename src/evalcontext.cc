#include "evalcontext.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "localscope.h"
#include "exceptions.h"
#include "boost-utils.h"

EvalContext::EvalContext(const std::shared_ptr<Context> parent, const AssignmentList &args, const Location &loc, const class LocalScope *const scope)
	: Context(parent), loc(loc), eval_arguments(args), scope(scope)
{
}

const std::string &EvalContext::getArgName(size_t i) const
{
	assert(i < this->eval_arguments.size());
	return this->eval_arguments[i]->getName();
}

Value EvalContext::getArgValue(size_t i, const std::shared_ptr<Context> ctx) const
{
	assert(i < this->eval_arguments.size());
	const auto &arg = this->eval_arguments[i];
	if (arg->getExpr()) {
		return arg->getExpr()->evaluate(ctx ? ctx : (const_cast<EvalContext *>(this))->get_shared_ptr());
	}
	return Value::undefined.clone();
}

/*!
  Matches arguments with parameters.
  Returns an AssignmentMap (string -> Expression*)
*/
AssignmentMap EvalContext::resolveArguments(const AssignmentList &parameters, const AssignmentList &optional_parameters, bool silent) const
{
  AssignmentMap resolvedArgs;
  size_t parameter_position = 0;
  bool tooManyWarned=false;

  for (size_t i=0; i<this->numArgs(); ++i) {
    const auto &name = this->getArgName(i); // name is optional
    const auto expr = this->getArgs()[i]->getExpr().get();
    if (!name.empty()) {
      if (name.at(0)!='$' && !silent) {
        bool found=false;
        for (auto const& parameter: parameters) {
          if (parameter->getName() == name) found = true;
        }
        for (auto const& parameter: optional_parameters) {
          if (parameter->getName() == name) found = true;
        }
        if (!found) {
		  LOG(message_group::Warning,this->loc,this->documentRoot(),"variable %1$s not specified as parameter",name);
        }
      }
      if (resolvedArgs.find(name) != resolvedArgs.end()) {
          LOG(message_group::Warning,this->loc,this->documentRoot(),"argument %1$s supplied more than once",name);
      }
      resolvedArgs[name] = expr;
    }
    // If positional, find name of parameter with this position
    else if (parameter_position < parameters.size()) resolvedArgs[parameters[parameter_position++]->getName()] = expr;
    else if (!silent && !tooManyWarned){
      LOG(message_group::Warning,this->loc,this->documentRoot(),"Too many unnamed arguments supplied");
      tooManyWarned=true;
    }
  }
  return resolvedArgs;
}

size_t EvalContext::numChildren() const
{
	return this->scope ? this->scope->children_inst.size() : 0;
}

shared_ptr<ModuleInstantiation> EvalContext::getChild(size_t i) const
{
	return this->scope ? this->scope->children_inst[i] : nullptr;
}

void EvalContext::assignTo(std::shared_ptr<Context> target) const
{
	std::set<std::string> seen;
	for (const auto &assignment : this->eval_arguments) {
		Value v = (assignment->getExpr()) ? assignment->getExpr()->evaluate(target) : Value::undefined.clone();
		
		if (assignment->getName().empty()) {
			LOG(message_group::Warning,this->loc,target->documentRoot(),"Assignment without variable name %1$s",v.toEchoString());
		} else if (seen.find(assignment->getName()) != seen.end()) {
			LOG(message_group::Warning,this->loc,target->documentRoot(),"Ignoring duplicate variable assignment %1$s = %2$s",assignment->getName(),v.toEchoString());
		} else {
			target->set_variable(assignment->getName(), std::move(v));
			seen.insert(assignment->getName());
		}
	}
}

std::ostream &operator<<(std::ostream &stream, const EvalContext &ec)
{
	for (size_t i = 0; i < ec.numArgs(); ++i) {
		if (i > 0) stream << ", ";
		if (!ec.getArgName(i).empty()) stream << ec.getArgName(i) << " = ";
		auto val = ec.getArgValue(i);
		stream << val.toEchoString();
	}
	return stream;
}

#ifdef DEBUG
std::string EvalContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	std::ostringstream s;
	if (inst)
		s << boost::format("EvalContext %p (%p) for %s inst (%p)") % this % this->parent % inst->name() % inst;
	else
		s << boost::format("Context: %p (%p)") % this % this->parent;

	s << boost::format("  eval args:");
	for (size_t i=0; i<this->eval_arguments.size(); ++i) {
		s << boost::format("    %s = %s") % this->eval_arguments[i]->getName() % this->eval_arguments[i]->getExpr();
	}
	if (this->scope && this->scope->children_inst.size() > 0) {
		s << boost::format("    children:");
		for(const auto &ch : this->scope->children_inst) {
			s << boost::format("      %s") % ch->name();
		}
	}
	if (mod) {
		const UserModule *m = dynamic_cast<const UserModule*>(mod);
		if (m) {
			s << boost::format("  module args:");
			for(const auto &parameter : m->parameters) {
				s << boost::format("    %s = %s") % parameter->getName() % variables.get(parameter->getName());
			}
		}
	}
	return s.str();
}
#endif
