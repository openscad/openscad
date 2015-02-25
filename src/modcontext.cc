#include "modcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"
#include "ModuleCache.h"

#include <boost/foreach.hpp>

ModuleContext::ModuleContext(const Context *parent, const EvalContext *evalctx)
	: Context(parent), functions_p(NULL), modules_p(NULL), evalctx(evalctx)
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
 	BOOST_FOREACH(const Assignment &ass, assignments) {
		ValuePtr tmpval = ass.second->evaluate(this);
		if (tmpval->isUndefined()) undefined_vars.push_back(ass.first);
 		else this->set_variable(ass.first, tmpval);
 	}

	// Variables which couldn't be evaluated in the first pass is attempted again,
  // to allow for initialization out of order

	boost::unordered_map<std::string, Expression *> tmpass;
	BOOST_FOREACH (const Assignment &ass, assignments) {
		tmpass[ass.first] = ass.second;
	}
		
	bool changed = true;
	while (changed) {
		changed = false;
		std::list<std::string>::iterator iter = undefined_vars.begin();
		while (iter != undefined_vars.end()) {
			std::list<std::string>::iterator curr = iter++;
			boost::unordered_map<std::string, Expression *>::iterator found = tmpass.find(*curr);
			if (found != tmpass.end()) {
				const Expression *expr = found->second;
				ValuePtr tmpval = expr->evaluate(this);
				// FIXME: it's not enough to check for undefined;
				// we need to check for any undefined variable in the subexpression
				// For now, ignore this and revisit the validity and order of variable
				// assignments later
				if (!tmpval->isUndefined()) {
					changed = true;
					this->set_variable(*curr, tmpval);
					undefined_vars.erase(curr);
				}
			}
		}
	}
}
#endif

void ModuleContext::initializeModule(const class Module &module)
{
	this->setVariables(module.definition_arguments, evalctx);
	// FIXME: Don't access module members directly
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	BOOST_FOREACH(const Assignment &ass, module.scope.assignments) {
		this->set_variable(ass.first, ass.second->evaluate(this));
	}

// Experimental code. See issue #399
//	evaluateAssignments(module.scope.assignments);
}

/*!
	Only used to initialize builtins for the top-level root context
*/
void ModuleContext::registerBuiltin()
{
	const LocalScope &scope = Builtins::instance()->getGlobalScope();

	// FIXME: Don't access module members directly
	this->functions_p = &scope.functions;
	this->modules_p = &scope.modules;
	BOOST_FOREACH(const Assignment &ass, scope.assignments) {
		this->set_variable(ass.first, ass.second->evaluate(this));
	}

	this->set_constant("PI", ValuePtr(M_PI));
}

const AbstractFunction *ModuleContext::findLocalFunction(const std::string &name) const
{
	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end()) {
		AbstractFunction *f = this->functions_p->find(name)->second;
		if (!f->is_enabled()) {
			PRINTB("WARNING: Experimental builtin function '%s' is not enabled.", name);
			return NULL;
		}
		return f;
	}
	return NULL;
}

const AbstractModule *ModuleContext::findLocalModule(const std::string &name) const
{
	if (this->modules_p && this->modules_p->find(name) != this->modules_p->end()) {
		AbstractModule *m = this->modules_p->find(name)->second;
		if (!m->is_enabled()) {
			PRINTB("WARNING: Experimental builtin module '%s' is not enabled.", name);
			return NULL;
		}
		std::string replacement = Builtins::instance()->isDeprecated(name);
		if (!replacement.empty()) {
			PRINT_DEPRECATION("The %s() module will be removed in future releases. Use %s instead.", name % replacement);
		}
		return m;
	}
	return NULL;
}

ValuePtr ModuleContext::evaluate_function(const std::string &name, 
																												 const EvalContext *evalctx) const
{
	const AbstractFunction *foundf = findLocalFunction(name);
	if (foundf) return foundf->evaluate(this, evalctx);

	return Context::evaluate_function(name, evalctx);
}

AbstractNode *ModuleContext::instantiate_module(const ModuleInstantiation &inst, EvalContext *evalctx) const
{
	const AbstractModule *foundm = this->findLocalModule(inst.name());
	if (foundm) return foundm->instantiate(this, &inst, evalctx);

	return Context::instantiate_module(inst, evalctx);
}

#ifdef DEBUG
std::string ModuleContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	std::stringstream s;
	if (inst)
		s << boost::format("ModuleContext %p (%p) for %s inst (%p) ") % this % this->parent % inst->name() % inst;
	else
		s << boost::format("ModuleContext: %p (%p)") % this % this->parent;
	s << boost::format("  document path: %s") % this->document_path;
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			s << "  module args:";
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				s << boost::format("    %s = %s") % arg.first % variables[arg.first];
			}
		}
	}
	typedef std::pair<std::string, ValuePtr> ValueMapType;
	s << "  vars:";
	BOOST_FOREACH(const ValueMapType &v, constants) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	BOOST_FOREACH(const ValueMapType &v, variables) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	BOOST_FOREACH(const ValueMapType &v, config_variables) {
		s << boost::format("    %s = %s") % v.first % v.second;
	}
	return s.str();
}
#endif

FileContext::FileContext(const class FileModule &module, const Context *parent)
	: ModuleContext(parent), usedlibs(module.usedlibs)
{
	if (!module.modulePath().empty()) this->document_path = module.modulePath();
}

ValuePtr FileContext::sub_evaluate_function(const std::string &name, 
																													 const EvalContext *evalctx,
																													 FileModule *usedmod) const

{
	FileContext ctx(*usedmod, this->parent);
	ctx.initializeModule(*usedmod);
	// FIXME: Set document path
#ifdef DEBUG
	PRINTDB("New lib Context for %s func:", name);
	PRINTDB("%s",ctx.dump(NULL, NULL));
#endif
	return usedmod->scope.functions[name]->evaluate(&ctx, evalctx);
}

ValuePtr FileContext::evaluate_function(const std::string &name, 
																											 const EvalContext *evalctx) const
{
	const AbstractFunction *foundf = findLocalFunction(name);
	if (foundf) return foundf->evaluate(this, evalctx);

	BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, this->usedlibs) {
		// usedmod is NULL if the library wasn't be compiled (error or file-not-found)
		FileModule *usedmod = ModuleCache::instance()->lookup(m);
		if (usedmod && usedmod->scope.functions.find(name) != usedmod->scope.functions.end())
			return sub_evaluate_function(name, evalctx, usedmod);
	}

	return ModuleContext::evaluate_function(name, evalctx);
}

AbstractNode *FileContext::instantiate_module(const ModuleInstantiation &inst, EvalContext *evalctx) const
{
	const AbstractModule *foundm = this->findLocalModule(inst.name());
	if (foundm) return foundm->instantiate(this, &inst, evalctx);

	BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, this->usedlibs) {
		FileModule *usedmod = ModuleCache::instance()->lookup(m);
		// usedmod is NULL if the library wasn't be compiled (error or file-not-found)
		if (usedmod &&
				usedmod->scope.modules.find(inst.name()) != usedmod->scope.modules.end()) {
			FileContext ctx(*usedmod, this->parent);
			ctx.initializeModule(*usedmod);
			// FIXME: Set document path
#ifdef DEBUG
			PRINTD("New file Context:");
			PRINTDB("%s",ctx.dump(NULL, &inst));
#endif
			return usedmod->scope.modules[inst.name()]->instantiate(&ctx, &inst, evalctx);
		}
	}

	return ModuleContext::instantiate_module(inst, evalctx);
}
