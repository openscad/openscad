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

#include <iostream>  // coryrc

void ScopeContext::init()
{
  for (const auto& assignment : scope->assignments) {
    if (assignment->getExpr()->isLiteral() && lookup_local_variable(assignment->getName())) {
      LOG(message_group::Warning, assignment->location(), this->documentRoot(),
          "Parameter %1$s is overwritten with a literal", quoteVar(assignment->getName()));
    }
    try {
      set_variable(assignment->getName(), assignment->getExpr()->evaluate(get_shared_ptr()));
    } catch (EvaluationException& e) {
      if (e.traceDepth > 0) {
        if (assignment->locationOfOverwrite().isNone()) {
          LOG(message_group::Trace, assignment->location(), this->documentRoot(), "assignment to %1$s",
              quoteVar(assignment->getName()));
        } else {
          LOG(message_group::Trace, assignment->location(), this->documentRoot(),
              "overwritten assignment to %1$s (this is where the assignment is evaluated)",
              quoteVar(assignment->getName()));
          LOG(message_group::Trace, assignment->locationOfOverwrite(), this->documentRoot(),
              "overwriting assignment to %1$s", quoteVar(assignment->getName()));
        }
        e.traceDepth--;
      }
      throw;
    }
  }
}

boost::optional<CallableFunction> ScopeContext::lookup_local_function(const std::string& name,
                                                                      const Location& loc) const
{
  // std::cerr << "lookup_local_function('"<<name<<"',...)\n";
  const auto defined = scope->lookup<UserFunction *>(name);
  if (defined) {
    // std::cerr << "\tFound in this scope\n";
    return CallableFunction{CallableUserFunction{get_shared_ptr(), *defined}};
  }
  // std::cerr << "\tMissing in this function scope\n";

  // Search assignments before searching namespaces included via `using`.
  // x = function () ...; should shadow function () x = ...; in a namespace.
  auto in_assignment = Context::lookup_local_function(name, loc);
  if (in_assignment) return in_assignment;

  // std::cerr << "\tMissing in the assignments as a function literal\n";

  for (auto ns_name : scope->getUsings()) {
    // std::cerr << "\tSearching namespace '"<< ns_name<<"'\n";
    auto ret = session()->lookup_namespace<CallableFunction>(ns_name, name);
    if (ret) return ret;
  }
  return boost::none;
}

boost::optional<InstantiableModule> ScopeContext::lookup_local_module(const std::string& name,
                                                                      const Location& loc) const
{
  const auto defined = scope->lookup<UserModule *>(name);
  if (defined) {
    return InstantiableModule{get_shared_ptr(), *defined};
  }

  // Search assignments before searching namespaces included via `using`.
  // x = module () ...; should shadow module () x = ...; in a namespace.
  auto in_assignment = Context::lookup_local_module(name, loc);
  if (in_assignment) return in_assignment;

  for (auto ns_name : scope->getUsings()) {
    auto ret = session()->lookup_namespace<InstantiableModule>(ns_name, name);
    if (ret) return ret;
  }
  return boost::none;
}

boost::optional<CallableFunction> ScopeContext::lookup_function_as_namespace(
  const std::string& name) const
{
  // std::cerr << "lookup_function_as_namespace<CallableFunction>('"<<name<<"')\n";
  if (auto uf = scope->lookup<UserFunction *>(name)) {
    // std::cerr << "\tFound\n";
    return CallableFunction{CallableUserFunction{get_shared_ptr(), *uf}};
  }
  // std::cerr << "\tNot in this scope\n";

  return Context::lookup_local_function(name, Location::NONE);
}

boost::optional<InstantiableModule> ScopeContext::lookup_module_as_namespace(
  const std::string& name) const
{
  // std::cerr << "lookup_module_as_namespace<InstantiableModule>('"<<name<<"')\n";
  if (auto um = scope->lookup<UserModule *>(name)) {
    return InstantiableModule{get_shared_ptr(), *um};
  }

  return Context::lookup_local_module(name, Location::NONE);
}

UserModuleContext::UserModuleContext(const std::shared_ptr<const Context>& parent,
                                     const UserModule *module, const Location& loc, Arguments arguments,
                                     Children children)
  : ScopeContext(parent, module->body), children(std::move(children))
{
  set_variable("$children", Value(double(this->children.size())));
  set_variable("$parent_modules", Value(double(StaticModuleNameStack::size())));
  apply_variables(
    Parameters::parse(std::move(arguments), loc, module->parameters, parent).to_context_frame());
}

std::vector<const std::shared_ptr<const Context> *> UserModuleContext::list_referenced_contexts() const
{
  std::vector<const std::shared_ptr<const Context> *> output = Context::list_referenced_contexts();
  output.push_back(&children.getContext());
  return output;
}

boost::optional<CallableFunction> FileContext::lookup_local_function(const std::string& name,
                                                                     const Location& loc) const
{
  auto result = ScopeContext::lookup_local_function(name, loc);
  if (result) {
    return result;
  }

  return lookup_function_from_uses(name);
}

boost::optional<InstantiableModule> FileContext::lookup_local_module(const std::string& name,
                                                                     const Location& loc) const
{
  auto result = ScopeContext::lookup_local_module(name, loc);
  if (result) {
    return result;
  }

  return lookup_module_from_uses(name);
}

boost::optional<CallableFunction> FileContext::lookup_function_from_uses(const std::string& name) const
{
  // std::cerr << "lookup_function_from_uses('"<<name<<"')\n";
  for (const auto& m : source_file->usedlibs) {
    // usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
    auto usedmod = SourceFileCache::instance()->lookup(m);
    if (usedmod) {
      if (const auto defined = usedmod->scope->lookup<UserFunction *>(name)) {
        // `use` can only be used in the top-level scope, so this next
        // line *should* be always passing the BuiltinContext as the new parent.
        ContextHandle<FileContext> context{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
        PRINTDB("FileContext for function %s::%s:", m % name);
        PRINTDB("%s", context->dump());
#endif
        return CallableFunction{CallableUserFunction{*context, *defined}};
      }
    }
  }
  return boost::none;
}

boost::optional<InstantiableModule> FileContext::lookup_module_from_uses(const std::string& name) const
{
  // std::cerr << "lookup_module_from_uses('"<<name<<"')\n";
  for (const auto& m : source_file->usedlibs) {
    // usedmod is nullptr if the library wasn't be compiled (error or file-not-found)
    auto usedmod = SourceFileCache::instance()->lookup(m);
    if (usedmod) {
      if (const auto defined = usedmod->scope->lookup<UserModule *>(name)) {
        // `use` can only be used in the top-level scope, so this next
        // line *should* be always passing the BuiltinContext as the new parent.
        // Improvement idea: puposefully get builtins from session() so this is never wrong.
        ContextHandle<FileContext> context{Context::create<FileContext>(this->parent, usedmod)};
#ifdef DEBUG
        PRINTDB("FileContext for module %s::%s:", m % name);
        PRINTDB("%s", context->dump());
#endif
        return InstantiableModule{*context, *defined};
      }
    }
  }
  return boost::none;
}
