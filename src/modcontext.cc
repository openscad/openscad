#include "modcontext.h"
#include "module.h"
#include "expression.h"
#include "function.h"
#include "printutils.h"
#include "builtin.h"

#include <boost/foreach.hpp>

ModuleContext::ModuleContext(const class Module *module, const Context *parent, const EvalContext *evalctx)
	: Context(parent), functions_p(NULL), modules_p(NULL), usedlibs_p(NULL)
{
	if (module) setModule(*module, evalctx);
}

ModuleContext::~ModuleContext()
{
}

void ModuleContext::setModule(const Module &module, const EvalContext *evalctx)
{
	this->setVariables(module.definition_arguments, evalctx);
	this->evalctx = evalctx;

  // FIXME: Hack - split out file context into separate class?
	const FileModule *fm = dynamic_cast<const FileModule*>(&module);
	if (fm) {
		this->usedlibs_p = &(fm->usedlibs);
		if (!fm->modulePath().empty()) this->document_path = fm->modulePath();
	}
	
	// FIXME: Don't access module members directly
	this->functions_p = &module.scope.functions;
	this->modules_p = &module.scope.modules;
	BOOST_FOREACH(const Assignment &ass, module.scope.assignments) {
		this->set_variable(ass.first, ass.second->evaluate(this));
	}
}

/*!
	Only used to initialize builtins for the top-level root context
*/
void ModuleContext::registerBuiltin()
{
	// FIXME: built-ins only contains variables, setModule isn't really needed for this
  // FIXME: Where to put set_variable from setModule?
	this->setModule(Builtins::instance()->getRootModule());
	this->set_constant("PI",Value(M_PI));
}

class RecursionGuard
{
public:
	RecursionGuard(const ModuleContext &c, const std::string &name) : c(c), name(name) { 
		c.recursioncount[name]++; 
	}
	~RecursionGuard() { if (--c.recursioncount[name] == 0) c.recursioncount.erase(name); }
	bool recursion_detected() const { return (c.recursioncount[name] > 100); }
private:
	const ModuleContext &c;
	const std::string &name;
};

Value ModuleContext::evaluate_function(const std::string &name, const EvalContext *evalctx) const
{
	RecursionGuard g(*this, name);
	if (g.recursion_detected()) { 
		PRINTB("Recursion detected calling function '%s'", name);
		return Value();
	}

	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end()) {
		return this->functions_p->find(name)->second->evaluate(this, evalctx);
	}
	
	if (this->usedlibs_p) {
		BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, *this->usedlibs_p) {
			if (m.second->scope.functions.find(name) != m.second->scope.functions.end()) {
				ModuleContext ctx(m.second, this->parent);
				// FIXME: Set document path
#if 0 && DEBUG
				PRINTB("New lib Context for %s func:", name);
				ctx.dump(NULL, NULL);
#endif
				return m.second->scope.functions[name]->evaluate(&ctx, evalctx);
			}
		}
	}
	return Context::evaluate_function(name, evalctx);
}

AbstractNode *ModuleContext::instantiate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const
{
	if (this->modules_p && this->modules_p->find(inst.name()) != this->modules_p->end()) {
		AbstractModule *m = this->modules_p->find(inst.name())->second;
		std::string replacement = Builtins::instance()->isDeprecated(inst.name());
		if (!replacement.empty()) {
			PRINTB("DEPRECATED: The %s() module will be removed in future releases. Use %s() instead.", inst.name() % replacement);
		}
		return m->instantiate(this, &inst, evalctx);
	}

	if (this->usedlibs_p) {
		BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, *this->usedlibs_p) {
			assert(m.second);
			if (m.second->scope.modules.find(inst.name()) != m.second->scope.modules.end()) {
				ModuleContext ctx(m.second, this->parent);
				// FIXME: Set document path
#if 0 && DEBUG
				PRINT("New lib Context:");
				ctx.dump(NULL, &inst);
#endif
				return m.second->scope.modules[inst.name()]->instantiate(&ctx, &inst, evalctx);
			}
		}
	}

	return Context::instantiate_module(inst, evalctx);
}

#ifdef DEBUG
void ModuleContext::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	if (inst) 
		PRINTB("ModuleContext %p (%p) for %s inst (%p) ", this % this->parent % inst->name() % inst);
	else 
		PRINTB("ModuleContext: %p (%p)", this % this->parent);
	PRINTB("  document path: %s", this->document_path);
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			PRINT("  module args:");
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				PRINTB("    %s = %s", arg.first % variables[arg.first]);
			}
		}
	}
	typedef std::pair<std::string, Value> ValueMapType;
	PRINT("  vars:");
  BOOST_FOREACH(const ValueMapType &v, constants) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, config_variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		

}
#endif

FileContext::FileContext(const class FileModule &module, 
												 const Context *parent, const EvalContext *evalctx)
	:ModuleContext(&module, parent, evalctx)
{
	
}

