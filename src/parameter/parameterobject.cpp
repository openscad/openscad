#include "parameterobject.h"

#include "module.h"
#include "modcontext.h"
#include "annotation.h"

ParameterObject::ParameterObject() : focus(false)
{
}

void ParameterObject::applyParameter(Assignment &assignment)
{
  ModuleContext ctx;
  const ValuePtr defaultValue = assignment.expr->evaluate(&ctx);
  
  if (defaultValue->type() == dvt) {
    assignment.expr = shared_ptr<Expression>(new Literal(value));
  }
}

int ParameterObject::setValue(const class ValuePtr defaultValue, const class ValuePtr values)
{
  this->values = values;
  this->value = defaultValue;
  this->defaultValue = defaultValue;
  this->vt = values->type();
  this->dvt = defaultValue->type();
  
  if (dvt == Value::BOOL) {
    target = CHECKBOX;
  } else if ((dvt == Value::VECTOR) && (defaultValue->toVector().size() <= 4)) {
    checkVectorWidget();
  } else if ((vt == Value::VECTOR) && ((dvt == Value::NUMBER) || (dvt == Value::STRING))) {
    target = COMBOBOX;
  } else if ((vt == Value::RANGE) && (dvt == Value::NUMBER)) {
    target = SLIDER;
  } else if (dvt == Value::NUMBER) {
    target = NUMBER;
  } else {
    target = TEXT;
  }
  
  return target;
}

void ParameterObject::setAssignment(Context *ctx, const Assignment *assignment, const ValuePtr defaultValue)
{
  name = assignment->name;
  const Annotation *param = assignment->annotation("Parameter");
  const ValuePtr values = param->evaluate(ctx);
  setValue(defaultValue, values);
  const Annotation *desc = assignment->annotation("Description");

  if (desc) {
    const ValuePtr v = desc->evaluate(ctx);
    if (v->type() == Value::STRING) {
      description=QString::fromStdString(v->toString());
    }
  }
  
  const Annotation *group = assignment->annotation("Group");
  if (group) {
    const ValuePtr v = group->evaluate(ctx);
    if (v->type() == Value::STRING) {
      groupName=v->toString();
    }
  } else {
    groupName="Parameters";
  }
}

bool ParameterObject::operator == (const ParameterObject &second)
{
  return (this->defaultValue == second.defaultValue && this->values==second.values &&
          this->description == second.description && this->groupName == second.groupName);
}

void ParameterObject::checkVectorWidget()
{
  Value::VectorType vec = defaultValue->toVector();
  for (unsigned int i = 0;i < vec.size();i++) {
    if (vec[i]->type() != Value::NUMBER) {
      target = TEXT;
      return;
    }
  }
  target = VECTOR;
}
