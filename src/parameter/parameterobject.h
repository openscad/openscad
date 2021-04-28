#pragma once

#include "Assignment.h"
#include "expression.h"
#include "FileModule.h"
#include "value.h"
#include "parameter/parameterset.h"

class ParameterObject
{
public:
	typedef enum { UNDEFINED, COMBOBOX, SLIDER, CHECKBOX, TEXT, NUMBER, VECTOR } parameter_type_t;

	Value value;
	Value values;
	Value defaultValue;
	Value::Type dvt;
	parameter_type_t target;
	std::string name;
	std::string groupName;
	std::string description;

private:
	Value::Type vt;
	void setValue(const Value &defaultValue, const Value &values);
	static bool isVector4(const Value& defaultValue);

public:
	ParameterObject(std::shared_ptr<Context> context, const Assignment* assignment, const Value &defaultValue);
	static std::unique_ptr<ParameterObject> fromAssignment(const Assignment* assignment);
	void reset();
	bool importValue(boost::property_tree::ptree encodedValue, bool store);
	boost::property_tree::ptree exportValue() const;
	void apply(Assignment* assignment) const;
};

class ParameterObjects : public std::vector<std::unique_ptr<ParameterObject>>
{
public:
	static ParameterObjects fromModule(const FileModule* module);
	void reset();
	void importValues(const ParameterSet& values);
	ParameterSet exportValues(const std::string& setName);
	void apply(FileModule *fileModule) const;
};
