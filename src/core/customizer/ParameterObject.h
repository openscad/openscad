#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <set>

#include "core/customizer/ParameterSet.h"
#include "core/Expression.h"
#include "json/json.hpp"
using json = nlohmann::json;

class SourceFile;
class Assignment;
class ObjectExpression;
class Context;
class ParameterObject
{
public:
  enum class ParameterType { Bool, String, Number, Vector, Enum };

  virtual ~ParameterObject() = default;
  static std::unique_ptr<ParameterObject> fromAssignment(const Assignment *assignment,
                                                         const Context *context = nullptr);
  static std::unique_ptr<ParameterObject> fromObjectExpression(const ObjectExpression *obj,
                                                               const Assignment *assignment,
                                                               const Context *context = nullptr);
  [[nodiscard]] ParameterType type() const { return type_; }
  [[nodiscard]] const std::string& name() const { return name_; }
  [[nodiscard]] const std::string& description() const { return description_; }
  [[nodiscard]] const std::string& group() const { return group_; }

  virtual void reset() = 0;
  virtual bool importValue(boost::property_tree::ptree encodedValue, bool store) = 0;
  [[nodiscard]] virtual boost::property_tree::ptree exportValue() const = 0;
  [[nodiscard]] virtual json jsonValue() const = 0;
  virtual void apply(Assignment *assignment) const = 0;
  virtual void updateContext(Context *context) const = 0;

  void setLocked(bool locked) { locked_ = locked; }
  [[nodiscard]] bool isLocked() const { return locked_; }

  void setHidden(bool hidden) { hidden_ = hidden; }
  [[nodiscard]] bool isHidden() const { return hidden_; }

  [[nodiscard]] const std::set<std::string>& getDependencies() const { return dependencies; }
  std::set<std::string>& getDependencies() { return dependencies; }

  virtual void updateAttributes(const Context *context);
  void setLockedExpression(std::shared_ptr<Expression> expr) { lockedExpr = expr; }
  void setHiddenExpression(std::shared_ptr<Expression> expr) { hiddenExpr = expr; }

protected:
  ParameterObject(std::string name, std::string description, std::string group, ParameterType type)
    : type_(type), name_(std::move(name)), description_(std::move(description)), group_(std::move(group))
  {
  }

  ParameterType type_;
  std::string name_;
  std::string description_;
  std::string group_;
  bool locked_ = false;
  bool hidden_ = false;
  std::shared_ptr<Expression> lockedExpr;
  std::shared_ptr<Expression> hiddenExpr;
  std::set<std::string> dependencies;
};

class BoolParameter : public ParameterObject
{
public:
  BoolParameter(const std::string& name, const std::string& description, const std::string& group,
                bool defaultValue)
    : ParameterObject(name, description, group, ParameterObject::ParameterType::Bool),
      value(defaultValue),
      defaultValue(defaultValue)
  {
  }

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;
  void updateContext(Context *context) const override;

  bool value;
  bool defaultValue;
};

class StringParameter : public ParameterObject
{
public:
  StringParameter(const std::string& name, const std::string& description, const std::string& group,
                  const std::string& defaultValue, boost::optional<size_t> maximumSize);

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;
  void updateContext(Context *context) const override;

  std::string value;
  std::string defaultValue;
  boost::optional<size_t> maximumSize;
};

class NumberParameter : public ParameterObject
{
public:
  NumberParameter(const std::string& name, const std::string& description, const std::string& group,
                  double defaultValue, boost::optional<double> minimum, boost::optional<double> maximum,
                  boost::optional<double> step)
    : ParameterObject(name, description, group, ParameterObject::ParameterType::Number),
      value(defaultValue),
      defaultValue(defaultValue),
      minimum(minimum),
      maximum(maximum),
      step(step)
  {
  }

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;
  void updateContext(Context *context) const override;

  double value;
  double defaultValue;
  boost::optional<double> minimum;
  boost::optional<double> maximum;
  boost::optional<double> step;
};

class VectorParameter : public ParameterObject
{
public:
  VectorParameter(const std::string& name, const std::string& description, const std::string& group,
                  const std::vector<double>& defaultValue, boost::optional<double> minimum,
                  boost::optional<double> maximum, boost::optional<double> step)
    : ParameterObject(name, description, group, ParameterObject::ParameterType::Vector),
      value(defaultValue),
      defaultValue(defaultValue),
      minimum(minimum),
      maximum(maximum),
      step(step)
  {
  }

  void reset() override { value = defaultValue; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;
  void updateContext(Context *context) const override;

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

  EnumParameter(const std::string& name, const std::string& description, const std::string& group,
                int defaultValueIndex, std::vector<EnumItem> items)
    : ParameterObject(name, description, group, ParameterObject::ParameterType::Enum),
      valueIndex(defaultValueIndex),
      defaultValueIndex(defaultValueIndex),
      items(std::move(items))
  {
  }

  void reset() override { valueIndex = defaultValueIndex; }
  bool importValue(boost::property_tree::ptree encodedValue, bool store) override;
  [[nodiscard]] boost::property_tree::ptree exportValue() const override;
  [[nodiscard]] json jsonValue() const override;
  void apply(Assignment *assignment) const override;
  void updateContext(Context *context) const override;

  int valueIndex;
  int defaultValueIndex;
  std::vector<EnumItem> items;
};

class ParameterObjects : public std::vector<std::unique_ptr<ParameterObject>>
{
public:
  static ParameterObjects fromSourceFile(const SourceFile *sourceFile, const Context *context = nullptr);
  void reset();
  void importValues(const ParameterSet& values);
  ParameterSet exportValues(const std::string& setName);
  void apply(SourceFile *sourceFile) const;
};
