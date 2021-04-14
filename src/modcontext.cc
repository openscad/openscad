#include "modcontext.h"
#include "ModuleInstantiation.h"
#include "expression.h"
#include "parameters.h"
#include "printutils.h"
#include "builtin.h"
#include "SourceFileCache.h"
#include <cmath>
#include "boost-utils.h"
#ifdef DEBUG
#include <boost/format.hpp>
#endif

// Experimental code. See issue #399
#if 0
void ScopeContext::evaluateAssignments(const AssignmentList &assignments)
{
	// First, assign all simple variables
	std::list<std::string> undefined_vars;
 	for (const auto &ass : assignments) {
		Value tmpval = ass.second->evaluate(this);
		if (tmpval.isUndefined()) undefined_vars.push_back(ass.first);
 		else this->set_variable(ass.first, tmpval);
 	}

	// Variables which couldn't be evaluated in the first pass is attempted again,
  // to allow for initialization out of order

	std::unordered_map<std::string, Expression *> tmpass;
	for(const auto &ass : assignments) {
		tmpass[ass.first] = ass.second;
	}
		
	bool changed = true;
	while (changed) {
		changed = false;
		std::list<std::string>::iterator iter = undefined_vars.begin();
		while (iter != undefined_vars.end()) {
			std::list<std::string>::iterator curr = iter++;
			std::unordered_map<std::string, Expression *>::iterator found = tmpass.find(*curr);
			if (found != tmpass.end()) {
				const Expression *expr = found->second;
				Value tmpval = expr->evaluate(this);
				// FIXME: it's not enough to check for undefined;
				// we need to check for any undefined variable in the subexpression
				// For now, ignore this and revisit the validity and order of variable
				// assignments later
				if (!tmpval.isUndefined()) {
					changed = true;
					this->set_variable(*curr, tmpval);
					undefined_vars.erase(curr);
				}
			}
		}+-
	}
}
#endif

void ScopeContext::init()
{
	for (const auto &assignment : scope->assignments) {
		if (assignment->getExpr()->isLiteral() && lookup_local_variable(assignment->getName())) {
			LOG(message_group::Warning,assignment->location(),this->documentRoot(),"Parameter %1$s is overwritten with a literal",assignment->getName());
		}
		set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
	}

// Experimental code. See issue #399
//	evaluateAssignments(module.scope.assignments);
}

boost::optional<CallableFunction> ScopeContext::lookup_local_function(const std::string &name, const Location &loc) const
{
	const auto& search = scope->functions.find(name);
	if (search != scope->functions.end()) {
		return CallableFunction{CallableUserFunction{get_shared_ptr(), search->second.get()}};
	}
	return Context::lookup_local_function(name, loc);
}

boost::optional<InstantiableModule> ScopeContext::lookup_local_module(const std::string &name, const Location &loc) const
{
	const auto& search = scope->modules.find(name);
	if (search != scope->modules.end()) {
		return InstantiableModule{get_shared_ptr(), search->second.get()};
	}
	return Context::lookup_local_module(name, loc);
}

#ifdef DEBUG
std::string ScopeContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{

	std::ostringstream s;
	if (inst) {
		s << boost::format("UserModuleContext %p (%p) for %s inst (%p) ") % this % this->parent % inst->name() % inst;
	}
	else {
		s << boost::format("UserModuleContext: %p (%p)") % this % this->parent;
	}
	if (mod) {
		const UserModule *m = dynamic_cast<const UserModule*>(mod);
		if (m) {
			s << "  module parameters:";
			for(const auto &parameter : m->parameters) {
				s << boost::format("    %s = %s") % parameter->getName() % lexical_variables.get(parameter->getName());
			}
		}
	}
	s << ContextFrame::dump();
	return s.str();
}
#endif

UserModuleContext::UserModuleContext(const std::shared_ptr<Context> parent, const UserModule* module, const Location &loc, Arguments arguments, Children children):
	ScopeContext(parent, &module->body),
	children(std::move(children))
{
	set_variable("$children", Value(double(this->children.size())));
	set_variable("$parent_modules", Value(double(StaticModuleNameStack::size())));
	apply_variables(Parameters::parse(std::move(arguments), loc, module->parameters, parent).to_context_frame());
}

boost::optional<CallableFunction> FileContext::lookup_local_function(const std::string &name, const Location &loc) const
{
	auto result = ScopeContext::lookup_local_function(name, loc);
	if (result) {
		return result;
	}
	
	for (const auto &m : source_file->usedlibs) {
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		auto usedmod = SourceFileCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.functions.find(name) != usedmod->scope.functions.end()) {
			ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
			PRINTDB("New lib Context for %s func:", name);
			PRINTDB("%s",ctx->dump(nullptr, nullptr));
#endif
			return CallableFunction{CallableUserFunction{ctx.ctx, usedmod->scope.functions[name].get()}};
		}
	}
	return boost::none;
}

boost::optional<InstantiableModule> FileContext::lookup_local_module(const std::string &name, const Location &loc) const
{
	auto result = ScopeContext::lookup_local_module(name, loc);
	if (result) {
		return result;
	}
	
	for (const auto &m : source_file->usedlibs) {
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		auto usedmod = SourceFileCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.modules.find(name) != usedmod->scope.modules.end()) {
			ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
			PRINTDB("New lib Context for %s module:", name);
			PRINTDB("%s",ctx->dump(nullptr, nullptr));
#endif
			return InstantiableModule{ctx.ctx, usedmod->scope.modules[name].get()};
		}
	}
	return boost::none;
}
