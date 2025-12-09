#include "core/ModuleInstantiation.h"

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>

#include "core/Context.h"
#include "core/Expression.h"
#include "core/callables.h"
#include "core/module.h"
#include "utils/compiler_specific.h"
#include "utils/exceptions.h"
#include "utils/printutils.h"
#include "utils/CallTraceStack.h"

void ModuleInstantiation::print(std::ostream& stream, const std::string& indent,
                                const bool inlined) const
{
  if (!inlined) stream << indent;
  stream << modname + "(";
  for (size_t i = 0; i < this->arguments.size(); ++i) {
    const auto& arg = this->arguments[i];
    if (i > 0) stream << ", ";
    if (!arg->getName().empty()) stream << arg->getName() << " = ";
    stream << *arg->getExpr();
  }
  if (scope->numElements() == 0) {
    stream << ");\n";
  } else if (scope->numElements() == 1) {
    stream << ") ";
    scope->print(stream, indent, true);
  } else {
    stream << ") {\n";
    scope->print(stream, indent + "\t", false);
    stream << indent << "}\n";
  }
}

void IfElseModuleInstantiation::print(std::ostream& stream, const std::string& indent,
                                      const bool inlined) const
{
  ModuleInstantiation::print(stream, indent, inlined);
  if (else_scope) {
    auto num_elements = else_scope->numElements();
    if (num_elements == 0) {
      stream << indent << "else;";
    } else {
      stream << indent << "else ";
      if (num_elements == 1) {
        else_scope->print(stream, indent, true);
      } else {
        stream << "{\n";
        else_scope->print(stream, indent + "\t", false);
        stream << indent << "}\n";
      }
    }
  }
}

std::shared_ptr<AbstractNode> ModuleInstantiation::evaluate(
  const std::shared_ptr<const Context>& context) const
{
  boost::optional<InstantiableModule> module = context->lookup_module(this->name(), this->loc);
  if (!module) {
    return nullptr;
  }

  // Use CallTraceStack guard instead of try/catch
  CallTraceStack::Guard trace_guard(CallTraceStack::Entry::Type::ModuleInstantiation, this->name(),
                                    this->loc, context);

  // No try/catch needed - exception propagates directly, trace is in CallTraceStack
  return module->module->instantiate(module->defining_context, this, context);
}

std::shared_ptr<LocalScope> IfElseModuleInstantiation::makeElseScope()
{
  this->else_scope = std::make_shared<LocalScope>();
  return this->else_scope;
}
