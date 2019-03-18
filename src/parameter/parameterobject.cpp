#include "parameterobject.h"

#include "module.h"
#include "modcontext.h"
#include "annotation.h"

ParameterObject::ParameterObject(Context *ctx, const Assignment &assignment, const Value defaultValue)
{
  this->set = false;
  this->name = assignment.name;
  const Annotation *param = assignment.annotation("Parameter");
  Value values = param->evaluate(ctx);
  setValue(defaultValue.clone(), std::move(values));
  const Annotation *desc = assignment.annotation("Description");

  if (desc) {
    const Value v = desc->evaluate(ctx);
    if (v.type() == Value::ValueType::STRING) {
      description=QString::fromStdString(v.toString());
    }
  }
  
  const Annotation *group = assignment.annotation("Group");
  if (group) {
    const Value v = group->evaluate(ctx);
    if (v.type() == Value::ValueType::STRING) {
      groupName=v.toString();
    }
  } else {
    groupName="Parameters";
  }
}

void ParameterObject::applyParameter(Assignment &assignment)
{
  ModuleContext ctx;
  const Value defaultValue = assignment.expr->evaluate(&ctx);
  
  if (defaultValue.type() == dvt) {
    assignment.expr = shared_ptr<Expression>(new Literal(value.clone()));
  }
}

void ParameterObject::setValue(const class Value defaultValue, const class Value values)
{
  this->values = values.clone();
  this->value = defaultValue.clone();
  this->defaultValue = defaultValue.clone();
  this->vt = values.type();
  this->dvt = defaultValue.type();
 
  bool makerBotMax = (vt == Value::ValueType::VECTOR && values.toVectorPtr()->size() == 1 && values.toVectorPtr()[0].type() == Value::ValueType::NUMBER); // [max] format from makerbot customizer

  if (dvt == Value::ValueType::BOOL) {
    this->target = CHECKBOX;
  } else if ((dvt == Value::ValueType::VECTOR) && (defaultValue.toVectorPtr()->size() <= 4)) {
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
  const Value::VectorPtr &vec = defaultValue.toVectorPtr();
  if(vec->size()==0) return TEXT;
  for (unsigned int i = 0;i < vec->size();i++) {
    if (vec[i].type() != Value::ValueType::NUMBER) {
      return TEXT;
    }
  }
  return VECTOR;
}
