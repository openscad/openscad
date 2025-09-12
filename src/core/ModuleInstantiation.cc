#include "core/ModuleInstantiation.h"

#include <ostream>
#include <memory>
#include <cstddef>
#include <string>

#include "utils/compiler_specific.h"
#include "core/Context.h"
#include "core/Expression.h"
#include "core/module.h"
#include "utils/exceptions.h"
#include "utils/printutils.h"
#ifdef ENABLE_PYTHON
#include "python/python_public.h"
#endif

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

void ModuleInstantiation::print_python(std::ostream& stream, std::ostream& stream_def,
                                       const std::string& indent, const bool inlined,
                                       const int context_mode) const
{
  if (!inlined) stream << indent;
  //  if(context_mode == 1) stream << "return ";
  stream << modname << "(";
  int n = 0;
  if (scope->numElements() > 1) {
    scope->print_python(stream, stream_def, indent + "\t", false, 0);
    n++;
  } else if (scope->numElements() == 1) {
    scope->print_python(stream, stream_def, indent + "\t", true, 0);
    n++;
  }

  for (size_t i = 0; i < this->arguments.size(); ++i) {
    const auto& arg = this->arguments[i];
    if (n > 0) stream << ", ";
    if (!arg->getName().empty()) stream << arg->getName() << " = ";
    stream << *arg->getExpr();
    n++;
  }
  stream << ")";
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

void IfElseModuleInstantiation::print_python(std::ostream& stream, std::ostream& stream_def,
                                             const std::string& indent, const bool inlined,
                                             const int context_mode) const
{
  ModuleInstantiation::print(stream, indent, inlined);
  if (else_scope) {
    auto num_elements = else_scope->numElements();
    if (num_elements == 0) {
      stream << indent << "else;";
    } else {
      stream << indent << "else ";
      if (num_elements == 1) {
        else_scope->print_python(stream, stream_def, indent, true);
      } else {
        stream << "{\n";
        else_scope->print_python(stream, stream_def, indent + "\t", false);
        stream << indent << "}\n";
      }
    }
  }
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive modules are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
 */
static void NOINLINE print_trace(const ModuleInstantiation *mod,
                                 const std::shared_ptr<const Context>& context)
{
  LOG(message_group::Trace, mod->location(), context->documentRoot(), "called by '%1$s'", mod->name());
}

std::shared_ptr<AbstractNode> ModuleInstantiation::evaluate(
  const std::shared_ptr<const Context>& context) const
{
  boost::optional<InstantiableModule> module = context->lookup_module(this->name(), this->loc);
  if (!module) {
    std::shared_ptr<AbstractNode> result = nullptr;
    std::string error;
#ifdef ENABLE_PYTHON
    result = python_modulefunc(this, context, error);
    if (!error.empty() && result == nullptr) {
      LOG(message_group::Warning, loc, context->documentRoot(), "Python: '%1$s'", error);
      return nullptr;
    }
#endif
    if (result == nullptr && error.size() > 0)
      LOG(message_group::Warning, loc, context->documentRoot(), "Ignoring unknown module '%1$s'",
          this->name());  // TODO muss anschlagen, wenn das modul NIRGENDS gefunden wurde
    return result;
  }

  try {
    auto node = module->module->instantiate(module->defining_context, this, context);
    return node;
  } catch (EvaluationException& e) {
    if (e.traceDepth > 0) {
      print_trace(this, context);
      e.traceDepth--;
    }
    throw;
  }
}

std::shared_ptr<LocalScope> IfElseModuleInstantiation::makeElseScope()
{
  this->else_scope = std::make_shared<LocalScope>();
  return this->else_scope;
}
