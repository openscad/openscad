#include "parameterobject.h"

#include "module.h"
#include "modcontext.h"
#include "annotation.h"

ParameterObject::ParameterObject(Context *ctx, const Assignment &assignment, const ValuePtr defaultValue)
{
  this->name = assignment.name;
  const Annotation *param = assignment.annotation("Parameter");
  const ValuePtr values = param->evaluate(ctx);
  setValue(defaultValue, values);
  const Annotation *desc = assignment.annotation("Description");

  if (desc) {
    const ValuePtr v = desc->evaluate(ctx);
    if (v->type() == Value::ValueType::STRING) {
      description=QString::fromStdString(v->toString());
    }
  }
  
  const Annotation *group = assignment.annotation("Group");
  if (group) {
    const ValuePtr v = group->evaluate(ctx);
    if (v->type() == Value::ValueType::STRING) {
      groupName=v->toString();
    }
  } else {
    groupName="Parameters";
  }
}

void ParameterObject::applyParameter(Assignment &assignment)
{
  ModuleContext ctx;
  const ValuePtr defaultValue = assignment.expr->evaluate(&ctx);
  
  if (defaultValue->type() == dvt) {
    assignment.expr = shared_ptr<Expression>(new Literal(value));
  }
}

void ParameterObject::setValue(const class ValuePtr defaultValue, const class ValuePtr values)
{
  this->values = values;
  this->value = defaultValue;
  this->defaultValue = defaultValue;
  this->vt = values->type();
  this->dvt = defaultValue->type();
 
  bool makerBotMax = (vt == Value::ValueType::VECTOR && values->toVector().size() == 1 && values->toVector()[0]->type() == Value::ValueType::NUMBER); // [max] format from makerbot customizer

  if (dvt == Value::ValueType::BOOL) {
    this->target = CHECKBOX;
  } else if ((dvt == Value::ValueType::VECTOR) && (defaultValue->toVector().size() <= 4)) {
    this->target = checkVectorWidget();
  } else if ((vt == Value::ValueType::RANGE || makerBotMax) && (dvt == Value::ValueType::NUMBER)) {
    this->target = SLIDER;
  } else if ((makerBotMax) && (dvt == Value::ValueType::STRING)){
    this->target = TEXT;
  } else if ((vt == Value::ValueType::VECTOR) && ((dvt == Value::ValueType::NUMBER) || (dvt == Value::ValueType::STRING))) {
    this->target = COMBOBOX;
  } else if (dvt == Value::ValueType::NUMBER) {
    this->target = NUMBER;
  } else {
    this->target = TEXT;
  }
}

bool ParameterObject::operator == (const ParameterObject &second)
{
  return (this->defaultValue == second.defaultValue && this->values==second.values &&
          this->description == second.description && this->groupName == second.groupName);
}

ParameterObject::parameter_type_t ParameterObject::checkVectorWidget()
{
  Value::VectorType vec = defaultValue->toVector();
  if(vec.size()==0) return TEXT;
  for (unsigned int i = 0;i < vec.size();i++) {
    if (vec[i]->type() != Value::ValueType::NUMBER) {
      return TEXT;
    }
  }
  return VECTOR;
}
