#include "core/ScopeContext.h"

#include "core/Arguments.h"
#include "core/EvaluationSession.h"
#include "core/Expression.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "core/SourceFileCache.h"
#include "core/UserModule.h"
#include "core/SourceFile.h"
#include "core/Value.h"

#include <utility>
#include <memory>
#include <cmath>
#include <vector>

#include <iostream>  // TODO: coryrc - remove

void ScopeContext::init()
{
  for (const auto& assignment : scope->getAssignments()) {
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

boost::optional<const Value&> ScopeContext::lookup_local_variable(const std::string& name) const
{
  auto result = Context::lookup_local_variable(name);
  if (result) return result;

  // TODO: coryrc - special variables are preserved, so you *could* look them up in usings... but should
  // we? currently returns undef but we can save time by skipping all this if special var.
  for (auto ns_name : scope->getUsings()) {
    // std::cerr << "\tSearching namespace '"<< ns_name<<"'\n";
    auto ret = session()->lookup_namespace<const Value&>(ns_name, name);
    if (ret) return ret;
  }
  return {};
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

FileContext::FileContext(const std::shared_ptr<const Context>& parent,
                         std::shared_ptr<const SourceFile> source_file)
  : ScopeContext(parent, source_file->scope), source_file(std::move(source_file))
{
}

template <typename T>
boost::optional<std::pair<T, std::shared_ptr<const SourceFile>>> FileContext::lookup_from_uses(
  const std::string& name) const
{
  // std::cerr << "lookup_function_from_uses('"<<name<<"')\n";
  auto ns_name = get_namespace_name();
  if (ns_name.empty()) ns_name = "top_level";  // TODO: coryrc - use static const string for "top_level"
  const auto& v = source_file->getNamespaceUsedLibrariesReverseOrdered(ns_name);
  for (auto it = v.rbegin(); it != v.rend(); ++it) {
    auto& m = *it;
    // usedmod is nullptr if the library wasn't parsed successfully (error or file-not-found)
    auto usedmod = SourceFileCache::instance()->lookup(m);
    if (usedmod) {
      if (auto search = usedmod->scope->lookup<T>(name)) {
        return boost::optional<std::pair<T, std::shared_ptr<const SourceFile>>>{
          std::pair(*search, usedmod)};
      }
    }
  }
  return {};
}

boost::optional<CallableFunction> FileContext::lookup_function_from_uses(const std::string& name) const
{
  // std::cerr << "lookup_function_from_uses('"<<name<<"')\n";
  if (auto pair = lookup_from_uses<UserFunction *>(name)) {
    auto [func, usedmod] = *pair;
    ContextHandle<FileContext> context{
      Context::create<FileContext>(session()->getBuiltinContext(), usedmod)};
#ifdef DEBUG
    PRINTDB("FileContext for function %s::%s:", m % name);
    PRINTDB("%s", context->dump());
#endif
    return CallableFunction{CallableUserFunction{*context, func}};
  }
  return {};
}

boost::optional<InstantiableModule> FileContext::lookup_module_from_uses(const std::string& name) const
{
  // std::cerr << "lookup_module_from_uses('"<<name<<"')\n";
  if (auto pair = lookup_from_uses<UserModule *>(name)) {
    auto [mod, usedmod] = *pair;
    ContextHandle<FileContext> context{
      Context::create<FileContext>(session()->getBuiltinContext(), usedmod)};
#ifdef DEBUG
    PRINTDB("FileContext for module %s::%s:", m % name);
    PRINTDB("%s", context->dump());
#endif
    return InstantiableModule{*context, mod};
  }
  return {};
}
