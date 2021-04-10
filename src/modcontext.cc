#include "modcontext.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "ModuleCache.h"
#include <cmath>
#include <memory>
#include "boost-utils.h"
#ifdef DEBUG
#include <boost/format.hpp>
#endif

ModuleContext::ModuleContext(const std::shared_ptr<Context> parent, const std::shared_ptr<EvalContext> evalctx)
	: Context(parent), functions_p(nullptr), modules_p(nullptr), evalctx(evalctx)
{
}

ModuleContext::~ModuleContext()
{
}

// Experimental code. See issue #399
#if 0
void ModuleContext::evaluateAssignments(const AssignmentList &assignments)
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

void ModuleContext::initializeModule(const UserModule &module)
{
	this->setVariables(evalctx, module.parameters, {}, true);
	// FIXME: Don't access module members directly
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	for (const auto &assignment : module.scope.assignments) {
		if (assignment->getExpr()->isLiteral() && lookup_local_variable(assignment->getName())) {
			LOG(message_group::Warning,assignment->location(),this->documentRoot(),"Module %1$s: Parameter %2$s is overwritten with a literal",module.name,assignment->getName());
		}
		this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
	}

// Experimental code. See issue #399
//	evaluateAssignments(module.scope.assignments);
}

boost::optional<CallableFunction> ModuleContext::lookup_local_function(const std::string &name, const Location &loc) const
{
	if (functions_p) {
		const auto& search = functions_p->find(name);
		if (search != functions_p->end()) {
			return CallableFunction{CallableUserFunction{get_shared_ptr(), search->second.get()}};
		}
	}
	return Context::lookup_local_function(name, loc);
}

boost::optional<InstantiableModule> ModuleContext::lookup_local_module(const std::string &name, const Location &loc) const
{
	if (this->modules_p && this->modules_p->find(name) != this->modules_p->end()) {
		auto m = this->modules_p->find(name)->second;
		return InstantiableModule{get_shared_ptr(), m.get()};
	}
	return Context::lookup_local_module(name, loc);
}

#ifdef DEBUG
std::string ModuleContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	std::ostringstream s;
	if (inst) {
		s << boost::format("ModuleContext %p (%p) for %s inst (%p) ") % this % this->parent % inst->name() % inst;
	}
	else {
		s << boost::format("ModuleContext: %p (%p)") % this % this->parent;
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

boost::optional<CallableFunction> FileContext::lookup_local_function(const std::string &name, const Location &loc) const
{
	auto result = ModuleContext::lookup_local_function(name, loc);
	if (result) {
		return result;
	}
	
	for (const auto &m : *this->usedlibs_p) {
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		auto usedmod = ModuleCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.functions.find(name) != usedmod->scope.functions.end()) {
			ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent)};
			ctx->initializeModule(*usedmod);
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
	auto result = ModuleContext::lookup_local_module(name, loc);
	if (result) {
		return result;
	}
	
	for (const auto &m : *this->usedlibs_p) {
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		auto usedmod = ModuleCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.modules.find(name) != usedmod->scope.modules.end()) {
			ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent)};
			ctx->initializeModule(*usedmod);
#ifdef DEBUG
			PRINTDB("New lib Context for %s module:", name);
			PRINTDB("%s",ctx->dump(nullptr, nullptr));
#endif
			return InstantiableModule{ctx.ctx, usedmod->scope.modules[name].get()};
		}
	}
	return boost::none;
}

void FileContext::initializeModule(const class FileModule &module)
{
	// FIXME: Don't access module members directly
	this->usedlibs_p = &module.usedlibs;
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	for (const auto &assignment : module.scope.assignments) {
		this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
	}
}
