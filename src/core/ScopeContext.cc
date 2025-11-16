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
      if (assignment->locationOfOverwrite().isNone()) {
        e.LOG(message_group::Trace, assignment->location(), this->documentRoot(), "assignment to %1$s",
              quoteVar(assignment->getName()));
      } else {
        e.LOG(message_group::Trace, assignment->location(), this->documentRoot(),
              "overwritten assignment to %1$s (this is where the assignment is evaluated)",
              quoteVar(assignment->getName()));
        e.LOG(message_group::Trace, assignment->locationOfOverwrite(), this->documentRoot(),
              "overwriting assignment to %1$s", quoteVar(assignment->getName()));
      }
      e.traceDepth--;

      throw;
    }
  }
}

boost::optional<CallableFunction> ScopeContext::lookup_local_function(const std::string& name,
                                                                      const Location& loc) const
{
  const auto defined = scope->lookup<UserFunction *>(name);
  if (defined) {
    return CallableFunction{CallableUserFunction{get_shared_ptr(), *defined}};
  }
  return Context::lookup_local_function(name, loc);
}

boost::optional<InstantiableModule> ScopeContext::lookup_local_module(const std::string& name,
                                                                      const Location& loc) const
{
  const auto defined = scope->lookup<UserModule *>(name);
  if (defined) {
    return InstantiableModule{get_shared_ptr(), *defined};
  }
  return Context::lookup_local_module(name, loc);
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

boost::optional<InstantiableModule> FileContext::lookup_local_module(const std::string& name,
                                                                     const Location& loc) const
{
  auto result = ScopeContext::lookup_local_module(name, loc);
  if (result) {
    return result;
  }

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
