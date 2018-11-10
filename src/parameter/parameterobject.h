#pragma once

#include "value.h"
#include "Assignment.h"
#include "expression.h"

#include <QString>

class ParameterObject
{
public:
	typedef enum { UNDEFINED, COMBOBOX, SLIDER, CHECKBOX, TEXT, NUMBER, VECTOR } parameter_type_t;

	ValuePtr value;
	ValuePtr values;
	ValuePtr defaultValue;
	Value::ValueType dvt;
	parameter_type_t target;
	QString description;
	std::string name;
	bool set;
	std::string groupName;

private:
	Value::ValueType vt;
	parameter_type_t checkVectorWidget();
	void setValue(const ValuePtr defaultValue, const ValuePtr values);

public:
	ParameterObject(Context *context, const Assignment &assignment, const ValuePtr defaultValue);
	void applyParameter(Assignment &assignment);
	bool operator==(const ParameterObject &second);
};
