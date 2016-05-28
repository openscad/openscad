#ifndef PARAMETEROBJECT_H
#define PARAMETEROBJECT_H

#pragma once

#include "value.h"
#include "qtgettext.h"
#include "value.h"
#include "typedefs.h"
#include "expression.h"
#include<QString>

using namespace std;
class ParameterObject
{
    typedef enum { UNDEFINED, COMBOBOX, SLIDER, CHECKBOX, TEXT, NUMBER, VECTOR } parameter_type_t;

public:
    ValuePtr value;
    ValuePtr values;
    ValuePtr defaultValue;
    Value::ValueType vt;
    Value::ValueType dvt;
    parameter_type_t target;
    QString description;
    string name;

    ParameterObject();
    void setAssignment(class Context *context, const class Assignment *assignment, const ValuePtr defaultValue);
    void applyParameter(class Assignment *assignment);

protected:
    int setValue(const class ValuePtr defaultValue, const class ValuePtr values);
};

#endif // PARAMETEROBJECT_H
