#include "compiler_specific.h"
#include "Context.h"
#include "ModuleInstantiation.h"
#include "Expression.h"
#include "exceptions.h"
#include "printutils.h"

ModuleInstantiation::ModuleInstantiation(const std::string name, const AssignmentList args, const Location& loc)
   : ASTNode(loc), arguments(std::move(args)), modname(std::move(name)), isLookup(true)
{
}

/* Subclass must supply the meat. */
ModuleInstantiation::ModuleInstantiation(const Location& loc) : ASTNode(loc)
{
}

ModuleInstantiation::ModuleInstantiation(Expression *ref_expr, const AssignmentList args, const Location& loc)
    : ASTNode(loc), arguments(args), ref_expr(ref_expr)
{
  if (typeid(*ref_expr) == typeid(Lookup)) {
    isLookup = true;
    const Lookup *lookup = static_cast<Lookup *>(ref_expr);
    modname = lookup->get_name();
  } else {
    isLookup = false;
    std::ostringstream s;
    s << "(";
    ref_expr->print(s, "");
    s << ")";
    modname = s.str();
  }
}

void ModuleInstantiation::print(std::ostream& stream, const std::string& indent, const bool inlined) const
{
  if (!inlined) stream << indent;
  stream << modname + "(";
  for (size_t i = 0; i < this->arguments.size(); ++i) {
    const auto& arg = this->arguments[i];
    if (i > 0) stream << ", ";
    if (!arg->getName().empty()) stream << arg->getName() << " = ";
    stream << *arg->getExpr();
  }
  if (scope.numElements() == 0) {
    stream << ");\n";
  } else if (scope.numElements() == 1) {
    stream << ") ";
    scope.print(stream, indent, true);
  } else {
    stream << ") {\n";
    scope.print(stream, indent + "\t", false);
    stream << indent << "}\n";
  }
}

void IfElseModuleInstantiation::print(std::ostream& stream, const std::string& indent, const bool inlined) const
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

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive modules are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
 */
static void NOINLINE print_trace(const ModuleInstantiation *mod, const std::shared_ptr<const Context>& context){
  LOG(message_group::Trace, mod->location(), context->documentRoot(), "called by '%1$s'", mod->name());
}

boost::optional<InstantiableModule> ModuleInstantiation::evaluate_module_expression(const std::shared_ptr<const Context>& context) const
{
  if (isLookup) {
    return context->lookup_module(modname, location());
  } else {
    Value v = ref_expr->evaluate(context);
    if (v.type() == Value::Type::MODULE) {
      return InstantiableModule{context, v.toModule().getModule()};
    } else {
      LOG(message_group::Warning, loc, context->documentRoot(), "Can't call module on %1$s", v.typeName());
      return boost::none;
    }
  }
}

std::shared_ptr<AbstractNode> ModuleInstantiation::evaluate(const std::shared_ptr<const Context>& context) const
{
  auto module = evaluate_module_expression(context);
  if (!module) {
    return nullptr;
  }

  try{
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

LocalScope *IfElseModuleInstantiation::makeElseScope()
{
  this->else_scope = std::make_unique<LocalScope>();
  return this->else_scope.get();
}
