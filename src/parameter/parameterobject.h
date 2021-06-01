#pragma once

#include "Assignment.h"
#include "expression.h"
#include "SourceFile.h"
#include "parameter/parameterset.h"

class ParameterObject
{
public:
	enum class ParameterType { Bool, String, Number, Vector, Enum};
	
	virtual ~ParameterObject() {}
	static std::unique_ptr<ParameterObject> fromAssignment(const Assignment* assignment);
	
	ParameterType type() const { return type_; }
	const std::string& name() const { return name_; }
	const std::string& description() const { return description_; }
	const std::string& group() const { return group_; }
	
	virtual void reset() = 0;
	virtual bool importValue(boost::property_tree::ptree encodedValue, bool store) = 0;
	virtual boost::property_tree::ptree exportValue() const = 0;
	virtual void apply(Assignment* assignment) const = 0;

protected:
	ParameterObject(const std::string& name, const std::string& description, const std::string& group, ParameterType type):
		type_(type), name_(name), description_(description), group_(group) {}

	ParameterType type_;
	std::string name_;
	std::string description_;
	std::string group_;
};

class BoolParameter : public ParameterObject
{
public:
	BoolParameter(
		const std::string& name, const std::string& description, const std::string& group,
		bool defaultValue
	):
		ParameterObject(name, description, group, ParameterObject::ParameterType::Bool),
		value(defaultValue), defaultValue(defaultValue)
	{}
	
	void reset() override { value = defaultValue; }
	bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
	boost::property_tree::ptree exportValue() const override;
	void apply(Assignment* assignment) const override;
	
	bool value;
	bool defaultValue;
};

class StringParameter : public ParameterObject
{
public:
	StringParameter(
		const std::string& name, const std::string& description, const std::string& group,
		const std::string& defaultValue,
		boost::optional<size_t> maximumSize
	);
	
	void reset() override { value = defaultValue; }
	bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
	boost::property_tree::ptree exportValue() const override;
	void apply(Assignment* assignment) const override;
	
	std::string value;
	std::string defaultValue;
	boost::optional<size_t> maximumSize;
};

class NumberParameter : public ParameterObject
{
public:
	NumberParameter(
		const std::string& name, const std::string& description, const std::string& group,
		double defaultValue,
		boost::optional<double> minimum, boost::optional<double> maximum, boost::optional<double> step
	):
		ParameterObject(name, description, group, ParameterObject::ParameterType::Number),
		value(defaultValue), defaultValue(defaultValue),
		minimum(minimum), maximum(maximum), step(step)
	{}
	
	void reset() override { value = defaultValue; }
	bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
	boost::property_tree::ptree exportValue() const override;
	void apply(Assignment* assignment) const override;
	
	double value;
	double defaultValue;
	boost::optional<double> minimum;
	boost::optional<double> maximum;
	boost::optional<double> step;
};

class VectorParameter : public ParameterObject
{
public:
	VectorParameter(
		const std::string& name, const std::string& description, const std::string& group,
		const std::vector<double>& defaultValue,
		boost::optional<double> minimum, boost::optional<double> maximum, boost::optional<double> step
	):
		ParameterObject(name, description, group, ParameterObject::ParameterType::Vector),
		value(defaultValue), defaultValue(defaultValue),
		minimum(minimum), maximum(maximum), step(step)
	{}
	
	void reset() override { value = defaultValue; }
	bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
	boost::property_tree::ptree exportValue() const override;
	void apply(Assignment* assignment) const override;
	
	std::vector<double> value;
	std::vector<double> defaultValue;
	boost::optional<double> minimum;
	boost::optional<double> maximum;
	boost::optional<double> step;
};

class EnumParameter : public ParameterObject
{
public:
	typedef boost::variant<double, std::string> EnumValue;
	struct EnumItem {
		std::string key;
		EnumValue value;
	};
	
	EnumParameter(
		const std::string& name, const std::string& description, const std::string& group,
		int defaultValueIndex,
		const std::vector<EnumItem>& items
	):
		ParameterObject(name, description, group, ParameterObject::ParameterType::Enum),
		valueIndex(defaultValueIndex), defaultValueIndex(defaultValueIndex),
		items(items)
	{}
	
	void reset() override { valueIndex = defaultValueIndex; }
	bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
	boost::property_tree::ptree exportValue() const override;
	void apply(Assignment* assignment) const override;
	
	int valueIndex;
	int defaultValueIndex;
	std::vector<EnumItem> items;
};

class ParameterObjects : public std::vector<std::unique_ptr<ParameterObject>>
{
public:
	static ParameterObjects fromSourceFile(const SourceFile* sourceFile);
	void reset();
	void importValues(const ParameterSet& values);
	ParameterSet exportValues(const std::string& setName);
	void apply(SourceFile* sourceFile) const;
};
