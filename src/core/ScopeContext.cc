#include "core/ScopeContext.h"
#include "core/Expression.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "core/SourceFileCache.h"
#include "core/UserModule.h"

#include <utility>
#include <memory>
#include <cmath>
#include <vector>

// Experimental code. See issue #399
#if 0
void ScopeContext::evaluateAssignments(const AssignmentList& assignments)
{
  // First, assign all simple variables
  std::list<std::string> undefined_vars;
  for (const auto& ass : assignments) {
    Value tmpval = ass.second->evaluate(this);
    if (tmpval.isUndefined()) undefined_vars.push_back(ass.first);
    else this->set_variable(ass.first, tmpval);
  }

  // Variables which couldn't be evaluated in the first pass is attempted again,
  // to allow for initialization out of order

  std::unordered_map<std::string, Expression *> tmpass;
  for (const auto& ass : assignments) {
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
    }
  }
}
#endif // if 0

void ScopeContext::init()
{
  for (const auto& assignment : scope->assignments) {
    if (assignment->getExpr()->isLiteral() && lookup_local_variable(assignment->getName())) {
      LOG(message_group::Warning, assignment->location(), this->documentRoot(), "Parameter %1$s is overwritten with a literal", assignment->getName());
    }
    try{
      set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
    } catch (EvaluationException& e) {
      if (e.traceDepth > 0) {
        if(assignment->locationOfOverwrite().isNone()){
          LOG(message_group::Trace, assignment->location(), this->documentRoot(), "assignment to '%1$s'", assignment->getName());
        } else {
          LOG(message_group::Trace, assignment->location(), this->documentRoot(), "overwritten assignment to '%1$s' (this is where the assignment is evaluated)", assignment->getName());
          LOG(message_group::Trace, assignment->locationOfOverwrite(), this->documentRoot(), "overwriting assignment to '%1$s'", assignment->getName());
        }
        e.traceDepth--;
      }
      throw;
    }
  }

// Experimental code. See issue #399
//	evaluateAssignments(module.scope.assignments);
}

boost::optional<CallableFunction> ScopeContext::lookup_local_function(const std::string& name, const Location& loc) const
{
  const auto& search = scope->functions.find(name);
  if (search != scope->functions.end()) {
    return CallableFunction{CallableUserFunction{get_shared_ptr(), search->second.get()}};
  }
  return Context::lookup_local_function(name, loc);
}

boost::optional<InstantiableModule> ScopeContext::lookup_local_module(const std::string& name, const Location& loc) const
{
  const auto& search = scope->modules.find(name);
  if (search != scope->modules.end()) {
    return InstantiableModule{get_shared_ptr(), search->second.get()};
  }
  return Context::lookup_local_module(name, loc);
}

UserModuleContext::UserModuleContext(const std::shared_ptr<const Context>& parent, const UserModule *module, const Location& loc, Arguments arguments, Children children) :
  ScopeContext(parent, &module->body),
  children(std::move(children))
{
  set_variable("$children", Value(double(this->children.size())));
  set_variable("$parent_modules", Value(double(StaticModuleNameStack::size())));
  apply_variables(Parameters::parse(std::move(arguments), loc, module->parameters, parent).to_context_frame());
}

std::vector<const std::shared_ptr<const Context> *> UserModuleContext::list_referenced_contexts() const
{
  std::vector<const std::shared_ptr<const Context> *> output = Context::list_referenced_contexts();
  output.push_back(&children.getContext());
  return output;
}

boost::optional<CallableFunction> FileContext::lookup_local_function(const std::string& name, const Location& loc) const
{
  auto result = ScopeContext::lookup_local_function(name, loc);
  if (result) {
    return result;
  }

  for (const auto& m : source_file->usedlibs) {
    // usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
    auto usedmod = SourceFileCache::instance()->lookup(m);
    if (usedmod && usedmod->scope.functions.find(name) != usedmod->scope.functions.end()) {
      ContextHandle<FileContext> context{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
      PRINTDB("FileContext for function %s::%s:", m % name);
      PRINTDB("%s", context->dump());
#endif
      return CallableFunction{CallableUserFunction{*context, usedmod->scope.functions[name].get()}};
    }
  }
  return boost::none;
}

boost::optional<InstantiableModule> FileContext::lookup_local_module(const std::string& name, const Location& loc) const
{
  auto result = ScopeContext::lookup_local_module(name, loc);
  if (result) {
    return result;
  }

  for (const auto& m : source_file->usedlibs) {
    // usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
    auto usedmod = SourceFileCache::instance()->lookup(m);
    if (usedmod && usedmod->scope.modules.find(name) != usedmod->scope.modules.end()) {
      ContextHandle<FileContext> context{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
      PRINTDB("FileContext for module %s::%s:", m % name);
      PRINTDB("%s", context->dump());
#endif
      return InstantiableModule{*context, usedmod->scope.modules[name].get()};
    }
  }
  return boost::none;
}
