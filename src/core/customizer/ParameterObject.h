#pragma once

#include <memory>
#include <cstddef>
#include "json/json.hpp"
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/Identifier.h"
#include "core/customizer/ParameterSet.h"

using json = nlohmann::json;

class SourceFile;
class Assignment;

class ParameterObject
{
public:
  enum class ParameterType { Bool, String, Number, Vector, Enum };

  virtual ~ParameterObject() = default;
  static std::unique_ptr<ParameterObject> fromAssignment(const Assignment *assignment);

  [[nodiscard]] ParameterType type() const { return type_; }
  [[nodiscard]] const Identifier& name() const { return name_; }
  [[nodiscard]] const std::string& description() const { return description_; }
  [[nodiscard]] const std::string& group() const { return group_; }

  virtual void reset() = 0;
  virtual bool importValue(boost::property_tree::ptree encodedValue, bool store) = 0;
  [[nodiscard]] virtual boost::property_tree::ptree exportValue() const = 0;
  [[nodiscard]] virtual json jsonValue() const = 0;
  virtual void apply(Assignment *assignment) const = 0;

protected:
  ParameterObject(const Identifier &name, const std::string &description, const std::string group, ParameterType type) :
    type_(type), name_(name), description_(description), group_(group) {}

  ParameterType type_;
  Identifier name_;
  std::string description_;
  std::string group_;
};

class BoolParameter : public ParameterObject
{
public:
  BoolParameter(
    const Identifier& name, const std::string& description, const std::string& group,
    bool defaultValue
    ) :
    ParameterObject(name, description, group, ParameterObject::ParameterType::Bool),
    value(defaultValue), defaultValue(defaultValue)
  {}

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;

  bool value;
  bool defaultValue;
};

class StringParameter : public ParameterObject
{
public:
  StringParameter(
    const Identifier& name, const std::string& description, const std::string& group,
    const std::string& defaultValue,
    boost::optional<size_t> maximumSize
    );

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;

  std::string value;
  std::string defaultValue;
  boost::optional<size_t> maximumSize;
};

class NumberParameter : public ParameterObject
{
public:
  NumberParameter(
    const Identifier& name, const std::string& description, const std::string& group,
    double defaultValue,
    boost::optional<double> minimum, boost::optional<double> maximum, boost::optional<double> step
    ) :
    ParameterObject(name, description, group, ParameterObject::ParameterType::Number),
    value(defaultValue), defaultValue(defaultValue),
    minimum(minimum), maximum(maximum), step(step)
  {}

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;

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
    const Identifier& name, const std::string& description, const std::string& group,
    const std::vector<double>& defaultValue,
    boost::optional<double> minimum, boost::optional<double> maximum, boost::optional<double> step
    ) :
    ParameterObject(name, description, group, ParameterObject::ParameterType::Vector),
    value(defaultValue), defaultValue(defaultValue),
    minimum(minimum), maximum(maximum), step(step)
  {}

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;

  std::vector<double> value;
  std::vector<double> defaultValue;
  boost::optional<double> minimum;
  boost::optional<double> maximum;
  boost::optional<double> step;
};

class EnumParameter : public ParameterObject
{
public:
  using EnumValue = std::variant<double, std::string>;
  struct EnumItem {
    std::string key;
    EnumValue value;
  };

  EnumParameter(
    const Identifier& name, const std::string& description, const std::string& group,
    int defaultValueIndex,
    std::vector<EnumItem> items
    ) :
    ParameterObject(name, description, group, ParameterObject::ParameterType::Enum),
    valueIndex(defaultValueIndex), defaultValueIndex(defaultValueIndex),
    items(std::move(items))
  {}

  void reset() override { valueIndex = defaultValueIndex; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;

  int valueIndex;
  int defaultValueIndex;
  std::vector<EnumItem> items;
};

class ParameterObjects : public std::vector<std::unique_ptr<ParameterObject>>
{
public:
  static ParameterObjects fromSourceFile(const SourceFile *sourceFile);
  void reset();
  void importValues(const ParameterSet& values);
  ParameterSet exportValues(const std::string& setName);
  void apply(SourceFile *sourceFile) const;
};
