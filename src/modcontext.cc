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
	this->setVariables(evalctx, module.definition_arguments, {}, true);
	// FIXME: Don't access module members directly
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	for (const auto &assignment : module.scope.assignments) {
		if (assignment->getExpr()->isLiteral() && this->variables.find(assignment->getName()) != this->variables.end()) {
			LOG(message_group::Warning,assignment->location(),this->documentPath(),"Module %1$s: Parameter %2$s is overwritten with a literal",module.name,assignment->getName());
		}
		this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
	}

// Experimental code. See issue #399
//	evaluateAssignments(module.scope.assignments);
}

shared_ptr<const UserFunction> ModuleContext::findLocalFunction(const std::string &name) const
{
 	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end()) {
		auto f = this->functions_p->find(name)->second;
		if (!f->is_enabled()) {
			LOG(message_group::Warning,Location::NONE,"","Experimental builtin function '%1$s' is not enabled.",name);
			return nullptr;
		}
		return f;
	}
	return nullptr;
}

shared_ptr<const UserModule> ModuleContext::findLocalModule(const std::string &name) const
{
	if (this->modules_p && this->modules_p->find(name) != this->modules_p->end()) {
		auto m = this->modules_p->find(name)->second;
		if (!m->is_enabled()) {
			LOG(message_group::Warning,Location::NONE,"","Experimental builtin module '%1$s' is not enabled.",name);
			return nullptr;
		}
		auto replacement = Builtins::instance()->isDeprecated(name);
		if (!replacement.empty()) {
			LOG(message_group::Deprecated,Location::NONE,"","The %1$s() module will be removed in future releases. Use %2$s instead.",std::string(name),std::string(replacement));
		}
		return m;
	}
	return nullptr;
}

Value ModuleContext::evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx) const
{
	const auto foundf = findLocalFunction(name);
	std::shared_ptr<Context> self = (const_cast<ModuleContext *>(this))->get_shared_ptr();
	if (foundf) return foundf->evaluate(self, evalctx);

	return Context::evaluate_function(name, evalctx);
}

AbstractNode *ModuleContext::instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	const auto foundm = this->findLocalModule(inst.name());
	std::shared_ptr<Context> self = (const_cast<ModuleContext *>(this))->get_shared_ptr();
	if (foundm) return foundm->instantiate(self, &inst, evalctx);

	return Context::instantiate_module(inst, evalctx);
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
	s << boost::format("  document path: %s") % *this->document_path;
	if (mod) {
		const UserModule *m = dynamic_cast<const UserModule*>(mod);
		if (m) {
			s << "  module args:";
			for(const auto &arg : m->definition_arguments) {
				s << boost::format("    %s = %s") % arg->getName() % variables.get(arg->getName());
			}
		}
	}
	s << "  vars:";
	for(const auto &v : constants) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	for(const auto &v : variables) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	for(const auto &v : config_variables) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	return s.str();
}
#endif

Value FileContext::sub_evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx, FileModule *usedmod) const
{
	ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent)};
	ctx->initializeModule(*usedmod);
	// FIXME: Set document path
#ifdef DEBUG
	PRINTDB("New lib Context for %s func:", name);
	PRINTDB("%s",ctx->dump(nullptr, nullptr));
#endif
	return usedmod->scope.functions[name]->evaluate(ctx.ctx, evalctx);
}

Value FileContext::evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx) const
{
	const auto foundf = findLocalFunction(name);
	std::shared_ptr<Context> self = (const_cast<FileContext *>(this))->get_shared_ptr();
	if (foundf) return foundf->evaluate(self, evalctx);

	for (const auto &m : *this->usedlibs_p) {
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		auto usedmod = ModuleCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.functions.find(name) != usedmod->scope.functions.end())
			return sub_evaluate_function(name, evalctx, usedmod);
	}

	return ModuleContext::evaluate_function(name, evalctx);
}

AbstractNode *FileContext::instantiate_module(const ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	const auto foundm = this->findLocalModule(inst.name());
	std::shared_ptr<Context> self = (const_cast<FileContext *>(this))->get_shared_ptr();
	if (foundm) return foundm->instantiate(self, &inst, evalctx);

	for (const auto &m : *this->usedlibs_p) {
		auto usedmod = ModuleCache::instance()->lookup(m);
		// usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
		if (usedmod && usedmod->scope.modules.find(inst.name()) != usedmod->scope.modules.end()) {
			ContextHandle<FileContext> ctx{Context::create<FileContext>(this->parent)};
			ctx->initializeModule(*usedmod);
			// FIXME: Set document path
#ifdef DEBUG
			PRINTD("New file Context:");
			PRINTDB("%s",ctx->dump(nullptr, &inst));
#endif
			return usedmod->scope.modules[inst.name()]->instantiate(ctx.ctx, &inst, evalctx);
		}
	}

	return ModuleContext::instantiate_module(inst, evalctx);
}

void FileContext::initializeModule(const class FileModule &module)
{
	if (!module.modulePath().empty()) this->document_path = std::make_shared<std::string>(module.modulePath());
	// FIXME: Don't access module members directly
	this->usedlibs_p = &module.usedlibs;
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	for (const auto &assignment : module.scope.assignments) {
		this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
	}
}
