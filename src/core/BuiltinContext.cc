#include "core/BuiltinContext.h"

#include <cmath>

#include "core/Builtins.h"
#include "core/Expression.h"
#include "core/function.h"
#include "utils/printutils.h"

BuiltinContext::BuiltinContext(EvaluationSession *session) : Context(session)
{
}

void BuiltinContext::init()
{
  for (const auto& assignment : Builtins::instance()->getAssignments()) {
    this->set_variable(assignment->getName(), assignment->getExpr()->evaluate(shared_from_this()));
  }

  this->set_variable("PI", M_PI);
}

boost::optional<CallableFunction> BuiltinContext::lookup_local_function(const std::string& name, const Location& loc) const
{
  const auto& search = Builtins::instance()->getFunctions().find(name);
  if (search != Builtins::instance()->getFunctions().end()) {
    BuiltinFunction *f = search->second;
    if (f->is_enabled()) {
      return CallableFunction{f};
    }

    LOG(message_group::Warning, loc, documentRoot(), "Experimental builtin function '%1$s' is not enabled", name);
  }
  return Context::lookup_local_function(name, loc);
}

boost::optional<InstantiableModule> BuiltinContext::lookup_local_module(const std::string& name, const Location& loc) const
{
  const auto& search = Builtins::instance()->getModules().find(name);
  if (search != Builtins::instance()->getModules().end()) {
    AbstractModule *m = search->second;
    if (!m->is_enabled()) {
      LOG(message_group::Warning, loc, documentRoot(), "Experimental builtin module '%1$s' is not enabled", name);
    }
    std::string replacement = Builtins::instance()->instance()->isDeprecated(name);
    if (!replacement.empty()) {
      LOG(message_group::Deprecated, loc, documentRoot(), "The %1$s() module will be removed in future releases. Use %2$s instead.", name, replacement);
    }
    if (m->is_enabled()) {
      return InstantiableModule{get_shared_ptr(), m};
    }
  }
  return Context::lookup_local_module(name, loc);
}
