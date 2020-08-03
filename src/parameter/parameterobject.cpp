#include "parameterobject.h"

#include "module.h"
#include "modcontext.h"
#include "annotation.h"

ParameterObject::ParameterObject(std::shared_ptr<Context> ctx, const shared_ptr<Assignment> &assignment, const ValuePtr defaultValue)
{
  this->set = false;
  this->name = assignment->getName();
  const Annotation *param = assignment->annotation("Parameter");
  const ValuePtr values = param->evaluate(ctx);
  setValue(defaultValue, values);
  const Annotation *desc = assignment->annotation("Description");

  if (desc) {
    const ValuePtr v = desc->evaluate(ctx);
    if (v->type() == Value::Type::STRING) {
      description=QString::fromStdString(v->toString());
    }
  }
  
  const Annotation *group = assignment->annotation("Group");
  if (group) {
    const ValuePtr v = group->evaluate(ctx);
    if (v->type() == Value::Type::STRING) {
      groupName=v->toString();
    }
  } else {
    groupName="Parameters";
  }
}

void ParameterObject::applyParameter(const shared_ptr<Assignment> &assignment)
{
  ContextHandle<Context> ctx{Context::create<Context>()};
  const ValuePtr defaultValue = assignment->getExpr()->evaluate(ctx.ctx);
  
  if (defaultValue->type() == dvt) {
    assignment->setExpr(make_shared<Literal>(value));
  }
}

void ParameterObject::setValue(const class ValuePtr defaultValue, const class ValuePtr values)
{
  this->values = values;
  this->value = defaultValue;
  this->defaultValue = defaultValue;
  this->vt = values->type();
  this->dvt = defaultValue->type();
 
  bool makerBotMax = (vt == Value::Type::VECTOR && values->toVector().size() == 1 && values->toVector()[0]->type() == Value::Type::NUMBER); // [max] format from makerbot customizer

  if (dvt == Value::Type::BOOL) {
    this->target = CHECKBOX;
  } else if ((dvt == Value::Type::VECTOR) && (defaultValue->toVector().size() <= 4)) {
    this->target = checkVectorWidget();
  } else if ((vt == Value::Type::RANGE || makerBotMax) && (dvt == Value::Type::NUMBER)) {
    this->target = SLIDER;
  } else if ((makerBotMax) && (dvt == Value::Type::STRING)){
    this->target = TEXT;
  } else if ((vt == Value::Type::VECTOR) && ((dvt == Value::Type::NUMBER) || (dvt == Value::Type::STRING))) {
    this->target = COMBOBOX;
  } else if (dvt == Value::Type::NUMBER) {
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
  VectorType vec = defaultValue->toVector();
  if(vec.size()==0) return TEXT;
  for (unsigned int i = 0;i < vec.size();i++) {
    if (vec[i]->type() != Value::Type::NUMBER) {
      return TEXT;
    }
  }
  return VECTOR;
}
