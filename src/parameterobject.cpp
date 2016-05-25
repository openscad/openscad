#include "parameterobject.h"
#include<QDebug>

ParameterObject::ParameterObject()
{

}

void ParameterObject::applyParameter(class Assignment *assignment){
    assignment->second = shared_ptr<Expression>(new ExpressionConst(value));
}

int ParameterObject::setValue(const class ValuePtr defaultValue, const class ValuePtr values){
    this->values = values;
    this->value = defaultValue;
    this->defaultValue = defaultValue;
    vt = values->type();
    dvt = defaultValue->type();
    if (dvt == Value::BOOL) {
        target = CHECKBOX;
    } else if ((dvt == Value::VECTOR) && (defaultValue->toVector().size() <= 4)) {
        target = VECTOR;
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
