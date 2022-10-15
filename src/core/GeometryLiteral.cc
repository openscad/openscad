// Several types associated with geometry-as-data.
// NEEDSWORK Should perhaps be split into GeometryLiteral.cc, HybridLiteral.cc,
// and GeometryInstantiation.cc.  (Or maybe GeometryInstantiation should
// go in ModuleInstantiation.cc.)
#include "GeometryLiteral.h"
#include "AST.h"
#include "Expression.h"
#include "Context.h"
#include "ScopeContext.h"
#include "Parameters.h"
#include "Value.h"
#include "node.h"
#include "Tree.h"

GeometryLiteral::GeometryLiteral(const Location& loc) : Expression(loc)
{
}

bool GeometryLiteral::isLiteral() const {
  return false;
}

Value GeometryLiteral::evaluate(const std::shared_ptr<const Context>& defining_context) const
{
  ContextHandle<ScopeContext> context{Context::create<ScopeContext>(defining_context, &body)};
  std::shared_ptr<AbstractNode> n =
    body.instantiateModules(*context, std::make_shared<LiteralNode>());
  return GeometryType(n);
}

void GeometryLiteral::print(std::ostream& stream, const std::string& indent) const
{
  std::string tab = "\t";
  stream << "{{\n";
  body.print(stream, indent+tab);
  stream << "}}";
}

Value::GeometryType::GeometryType(std::shared_ptr<AbstractNode> node)
  : node(node) {
}

// Copy explicitly only when necessary
GeometryType GeometryType::clone() const
{
  return GeometryType(this->node);
}


Value Value::GeometryType::operator==(const GeometryType& other) const {
  return this == &other;
}
Value Value::GeometryType::operator!=(const GeometryType& other) const {
  return this != &other;
}
Value Value::GeometryType::operator<(const GeometryType& other) const {
  return Value::undef("operation undefined (geometry < geometry)");
}
Value Value::GeometryType::operator>(const GeometryType& other) const {
  return Value::undef("operation undefined (geometry > geometry)");
}
Value Value::GeometryType::operator<=(const GeometryType& other) const {
  return Value::undef("operation undefined (geometry <= geometry)");
}
Value Value::GeometryType::operator>=(const GeometryType& other) const {
  return Value::undef("operation undefined (geometry >= geometry)");
}

void Value::GeometryType::print(std::ostream& stream) const {
  stream << "{{\n";
  // We skip over the top node because it's always a GroupNode and is boring;
  // it's implied by the {{ }}.
  for (auto child : node->children) {
    Tree t(child);
    stream << t.getString(*child, "");
  }
  stream << "}}";
}

std::shared_ptr<AbstractNode> Value::GeometryType::getNode() const {
  return node;
}

std::ostream& operator<<(std::ostream& stream, const GeometryType& g)
{
  g.print(stream);
  return stream;
}

HybridLiteral::HybridLiteral(const Location& loc) : Expression(loc)
{
}

bool HybridLiteral::isLiteral() const {
  return false;
}

Value HybridLiteral::evaluate(const std::shared_ptr<const Context>& defining_context) const
{
  ContextHandle<ScopeContext> context{Context::create<ScopeContext>(defining_context, &body)};
  std::shared_ptr<AbstractNode> n =
    this->body.instantiateModules(*context, std::make_shared<LiteralNode>());
  ObjectType obj(defining_context->session(), n);
  // NEEDSWORK: it would be nice if this was order-preserving, but
  // the lexical_variables list is not.
  const ValueMap& vm = context->get_lexical_variables();
  for (const auto& e : vm) {
    obj.set(e.first, e.second.clone());
  }
  return std::move(obj);
}

void HybridLiteral::print(std::ostream& stream, const std::string& indent) const
{
  std::string tab = "\t";
  stream << "{(\n";
  body.print(stream, indent+tab);
  stream << ")}";
}

GeometryInstantiation::GeometryInstantiation(shared_ptr<class Expression> expr, const Location& loc) : ModuleInstantiation(loc) {
  this->expr = expr;
}

GeometryInstantiation::~GeometryInstantiation()
{
}

std::shared_ptr<AbstractNode>
GeometryInstantiation::evaluate(
  const std::shared_ptr<const Context>& context) const
{
  Value v = expr->evaluate(context);
  switch (v.type()) {
  case Value::Type::GEOMETRY:
    {
    auto n = v.toGeometry().getNode()->clone();
    n->setModuleInstantiation(this);
    return n;
    }
  case Value::Type::OBJECT:
    {
      shared_ptr<AbstractNode> n = v.toObject().ptr->node;
      if (!n) {
        // NEEDSWORK should this be nullptr?
        return std::make_shared<GroupNode>(this);
      }
      n = n->clone();
      n->setModuleInstantiation(this);
      return n;
    }
  default:
    print_argConvert_warning("geometry", "value", v, {Value::Type::GEOMETRY}, loc, "???");
    return nullptr;
  }
}

void
GeometryInstantiation::print(
  std::ostream& stream,
  const std::string& indent,
  const bool inlined) const
{
  stream << "(";
  expr->print(stream, indent);
  stream << ");\n";
}
