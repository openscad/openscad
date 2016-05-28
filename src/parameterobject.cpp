#include "parameterobject.h"

#include "module.h"
#include "modcontext.h"

ParameterObject::ParameterObject()
{

}

void ParameterObject::applyParameter(class Assignment *assignment){
    ModuleContext ctx;
    const ValuePtr defaultValue =assignment->second.get()->evaluate(&ctx);
    if( defaultValue->type() == dvt ){
        assignment->second = shared_ptr<Expression>(new ExpressionConst(value));
    }
}

int ParameterObject::setValue(const class ValuePtr defaultValue, const class ValuePtr values){
    this->values = values;
    this->value = defaultValue;
    this->defaultValue = defaultValue;
    this->vt = values->type();
    this->dvt = defaultValue->type();
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


void ParameterObject::setAssignment(class Context *ctx, const class Assignment *assignment, const ValuePtr defaultValue){
    name = assignment->first;
    const Annotation *param = assignment->annotation("Parameter");
    const ValuePtr values = param->evaluate(ctx, "values");
    setValue(defaultValue, values);
    const Annotation *desc = assignment->annotation("Description");
    if (desc) {
        const ValuePtr v = desc->evaluate(ctx, "text");
        if (v->type() == Value::STRING) {
            description=QString::fromStdString(v->toString());
        }
    }
}
