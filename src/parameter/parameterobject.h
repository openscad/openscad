#ifndef PARAMETEROBJECT_H
#define PARAMETEROBJECT_H

#pragma once

#include "value.h"
#include "qtgettext.h"
#include "Assignment.h"
#include "expression.h"

using namespace std;

class ParameterObject
{
    typedef enum { UNDEFINED, COMBOBOX, SLIDER, CHECKBOX, TEXT, NUMBER, VECTOR } parameter_type_t;

public:
    ValuePtr value;
    ValuePtr values;
    ValuePtr defaultValue;
    Value::ValueType dvt;
    parameter_type_t target;
    QString description;
    string name;
    bool set;
    string groupName;

private:
    Value::ValueType vt;
    void checkVectorWidget();

public:
    ParameterObject();
    void setAssignment(Context *context, const Assignment *assignment, const ValuePtr defaultValue);
    void applyParameter( Assignment *assignment);
    bool operator == (const ParameterObject &second);

protected:
    int setValue(const ValuePtr defaultValue, const ValuePtr values);
};

#endif // PARAMETEROBJECT_H
