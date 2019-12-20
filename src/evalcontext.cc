#include "evalcontext.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "localscope.h"
#include "exceptions.h"

EvalContext::EvalContext(const std::shared_ptr<Context> parent, const AssignmentList &args, const Location &loc, const class LocalScope *const scope)
	: Context(parent), loc(loc), eval_arguments(args), scope(scope)
{
}

const std::string &EvalContext::getArgName(size_t i) const
{
	assert(i < this->eval_arguments.size());
	return this->eval_arguments[i].name;
}

ValuePtr EvalContext::getArgValue(size_t i, const std::shared_ptr<Context> ctx) const
{
	assert(i < this->eval_arguments.size());
	const auto &arg = this->eval_arguments[i];
	ValuePtr v;
	if (arg.expr) {
		v = arg.expr->evaluate(ctx ? ctx : (const_cast<EvalContext *>(this))->get_shared_ptr());
	}
	return v;
}

/*!
  Resolves arguments specified by evalctx, using args to lookup positional arguments.
  optargs is for optional arguments that are not positional arguments.

  Returns an AssignmentList (vector<Assignment>) with arguments in same order as "args", from definition
*/
AssignmentMap EvalContext::resolveArguments(const AssignmentList &args, const AssignmentList &optargs, bool silent) const
{
  size_t argc = this->numArgs();
  AssignmentMap resolvedArgs(args.size(), std::make_pair(false, Assignment("",nullptr)));

  typedef enum ArgStatus {
    UNSET = 0,
    NAMED = 1,
    POSITIONAL = 2,
  } ArgStatus;
  std::vector<ArgStatus> arg_status(args.size(), UNSET);

	// count of positional arguments provided
  size_t posarg_count = 0;
	// count of all other arguments provided: optargs, special vars, or variables whose name aren't in args(with warning).
	size_t extra_count = 0;

  for (size_t i=0; i<argc; i++) {
    const auto &name = this->getArgName(i); // name is optional
    if (name.empty()) {
			if (posarg_count < args.size()) {
				// If positional, find name of arg with this position 
				const auto &name = args[posarg_count].name; // name is optional
				if (arg_status[posarg_count] == NAMED) {
					PRINTB("WARNING: positional argument overrides argument \"%s\", %s", name % this->loc.toRelativeString(this->documentPath()));
				}
				arg_status[posarg_count] = POSITIONAL;
				resolvedArgs[posarg_count] = std::make_pair(false, Assignment(name, this->getArgs()[i]));
			}
			++posarg_count;
		} else {				

			bool found = false;
			size_t i_new;

			if (name.at(0) != '$') {
				for (size_t j = 0; j < args.size(); ++j) {
					if(args[j].name == name) { 
						found = true; 
						i_new = j;
						break;
					}
				}
				if (!found) {
					for (const auto &opt : optargs) {
						if (opt.name == name) {
							found = true;
							i_new = args.size() + extra_count;
							extra_count++;
							break; 
						}
					}
				}
			}

			// argument is special var or not named in definition, check for duplicates anyways
			if (!found) {
				for (size_t j = args.size(); j < resolvedArgs.size(); ++j) {
					if (resolvedArgs[j].second.name == name) {
						found = true;
						i_new = j;
						break;
					}
				}
			}

			if (found) {
				if (i_new < arg_status.size()) {
					if (arg_status[i_new] == NAMED) {
						PRINTB("WARNING: argument \"%s\" supplied more than once, %s", name % this->loc.toRelativeString(this->documentPath()));
					} else if (arg_status[i_new] == POSITIONAL) {
						PRINTB("WARNING: argument \"%s\" overrides positional argument, %s", name % this->loc.toRelativeString(this->documentPath()));
					}
				}
			} else {
				if (name.at(0) != '$' && !silent) {
					PRINTB("WARNING: variable \"%s\" not specified as parameter, %s", name % this->loc.toRelativeString(this->documentPath()));
				}
				i_new = args.size() + extra_count;
				extra_count++;
			}

			if (i_new < resolvedArgs.size()) {
				resolvedArgs[i_new] = std::make_pair(false, this->getArgs()[i]);
				arg_status[i_new] = NAMED;
			} else {
				resolvedArgs.emplace_back(false, this->getArgs()[i]);
				arg_status.push_back(NAMED);
			}
		}
  }

  if (posarg_count > args.size()) {
		if (!silent) PRINTB("WARNING: Too many unnamed arguments supplied, %s", this->loc.toRelativeString(this->documentPath()));
  } else {
		for(size_t i = posarg_count; i < args.size(); ++i) {
			// resolvedArgs initialized to assignment with empty string name, indicating default param not yet set
			if (resolvedArgs[i].second.name.empty()) {
				resolvedArgs[i] = std::make_pair(true, args[i]);
			}
		}
	}
  return resolvedArgs;
}

size_t EvalContext::numChildren() const
{
	return this->scope ? this->scope->children.size() : 0;
}

ModuleInstantiation *EvalContext::getChild(size_t i) const
{
	return this->scope ? this->scope->children[i] : nullptr; 
}

void EvalContext::assignTo(std::shared_ptr<Context> target) const
{
	for (const auto &assignment : this->getArgs()) {
		ValuePtr v;
		if (assignment.expr) v = assignment.expr->evaluate(target);
		
		if(assignment.name.empty()){
			PRINTB("WARNING: Assignment without variable name %s, %s", v->toEchoString() % this->loc.toRelativeString(target->documentPath()));
		}else if (target->has_local_variable(assignment.name)) {
			PRINTB("WARNING: Ignoring duplicate variable assignment %s = %s, %s", assignment.name % v->toEchoString() % this->loc.toRelativeString(target->documentPath()));
		} else {
			target->set_variable(assignment.name, v);
		}
	}
}

std::ostream &operator<<(std::ostream &stream, const EvalContext &ec)
{
	for (size_t i = 0; i < ec.numArgs(); i++) {
		if (i > 0) stream << ", ";
		if (!ec.getArgName(i).empty()) stream << ec.getArgName(i) << " = ";
		auto val = ec.getArgValue(i);
		stream << val->toEchoString();
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
	s << boost::format("  document path: %s") % *this->document_path;

	s << boost::format("  eval args:");
	for (size_t i=0;i<this->eval_arguments.size();i++) {
		s << boost::format("    %s = %s") % this->eval_arguments[i].name % this->eval_arguments[i].expr;
	}
	if (this->scope && this->scope->children.size() > 0) {
		s << boost::format("    children:");
		for(const auto &ch : this->scope->children) {
			s << boost::format("      %s") % ch->name();
		}
	}
	if (mod) {
		const UserModule *m = dynamic_cast<const UserModule*>(mod);
		if (m) {
			s << boost::format("  module args:");
			for(const auto &arg : m->definition_arguments) {
				s << boost::format("    %s = %s") % arg.name % *(variables[arg.name]);
			}
		}
	}
	return s.str();
}
#endif

