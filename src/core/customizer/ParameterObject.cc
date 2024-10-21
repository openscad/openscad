#include "core/customizer/ParameterObject.h"

#include "core/customizer/Annotation.h"
#include "core/Assignment.h"
#include "core/Expression.h"
#include "core/SourceFile.h"

#include <variant>
#include <map>
#include <utility>
#include <memory>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

namespace {

bool set_enum_value(json& o, const std::string& name, const EnumParameter::EnumItem& item)
{
  EnumParameter::EnumValue itemValue = item.value;
  double *doubleValue = std::get_if<double>(&itemValue);
  if (doubleValue) {
    o[name] = *doubleValue;
    return true;
  } else {
    o[name] = std::get<std::string>(itemValue);
    return false;
  }
}

}

bool BoolParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
  boost::optional<bool> decoded = encodedValue.get_value_optional<bool>();
  if (!decoded) {
    return false;
  }
  if (store) {
    value = *decoded;
  }
  return true;
}

boost::property_tree::ptree BoolParameter::exportValue() const
{
  boost::property_tree::ptree output;
  output.put_value<bool>(value);
  return output;
}

json BoolParameter::jsonValue() const
{
  json o;
  o["type"] = "boolean";
  o["initial"] = defaultValue;
  return o;
}

void BoolParameter::apply(Assignment *assignment) const
{
  assignment->setExpr(std::make_shared<Literal>(value));
}

StringParameter::StringParameter(
  const std::string& name, const std::string& description, const std::string& group,
  const std::string& defaultValue,
  boost::optional<size_t> maximumSize
  ) :
  ParameterObject(name, description, group, ParameterObject::ParameterType::String),
  value(defaultValue), defaultValue(defaultValue),
  maximumSize(maximumSize)
{
  if (maximumSize && defaultValue.size() > *maximumSize) {
    maximumSize = defaultValue.size();
  }
}

bool StringParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
  if (store) {
    value = encodedValue.data();
    if (maximumSize && value.size() > *maximumSize) {
      value = value.substr(0, *maximumSize);
    }
  }
  return true;
}

boost::property_tree::ptree StringParameter::exportValue() const
{
  boost::property_tree::ptree output;
  output.data() = value;
  return output;
}

json StringParameter::jsonValue() const
{
  json o;
  o["type"] = "string";
  o["initial"] = defaultValue;
  if (maximumSize.is_initialized()) {
    o["maxLength"] = maximumSize.get();
  }
  return o;
}

void StringParameter::apply(Assignment *assignment) const
{
  assignment->setExpr(std::make_shared<Literal>(value));
}

bool NumberParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
  boost::optional<double> decoded = encodedValue.get_value_optional<double>();
  if (!decoded) {
    return false;
  }
  if (store) {
    value = *decoded;
    if (minimum && value < *minimum) {
      value = *minimum;
    }
    if (maximum && value > *maximum) {
      value = *maximum;
    }
  }
  return true;
}

boost::property_tree::ptree NumberParameter::exportValue() const
{
  boost::property_tree::ptree output;
  output.put_value<double>(value);
  return output;
}

json NumberParameter::jsonValue() const
{
  json o;
  o["type"] = "number";
  o["initial"] = defaultValue;

  if (maximum.is_initialized()) {
    o["max"] = maximum.get();
    o["min"] = minimum.is_initialized() ? minimum.get() : 0.0;
    o["step"] = step.is_initialized() ? step.get() : 1.0;
  }
  return o;
}

void NumberParameter::apply(Assignment *assignment) const
{
  assignment->setExpr(std::make_shared<Literal>(value));
}

bool VectorParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
  std::vector<double> decoded;

  // NOLINTBEGIN(*NewDeleteLeaks) LLVM bug https://github.com/llvm/llvm-project/issues/40486
  std::string encoded = boost::algorithm::erase_all_copy(encodedValue.data(), " ");
  if (encoded.size() < 2 || encoded[0] != '[' || encoded[encoded.size() - 1] != ']') {
    return false;
  }
  encoded.erase(encoded.begin());
  encoded.erase(encoded.end() - 1);

  std::vector<std::string> items;
  boost::algorithm::split(items, encoded, boost::algorithm::is_any_of(","));
  // NOLINTEND(*NewDeleteLeaks)

  for (const std::string& item : items) {
    std::stringstream stream(item);
    double itemValue;
    stream >> itemValue;
    if (!stream || !stream.eof()) {
      return false;
    }
    decoded.push_back(itemValue);
  }

  if (decoded.size() != value.size()) {
    return false;
  }

  if (store) {
    for (size_t i = 0; i < value.size(); i++) {
      value[i] = decoded[i];
      if (minimum && value[i] < *minimum) {
        value[i] = *minimum;
      }
      if (maximum && value[i] > *maximum) {
        value[i] = *maximum;
      }
    }
  }
  return true;
}

boost::property_tree::ptree VectorParameter::exportValue() const
{
  std::stringstream encoded;
  encoded << "[";
  for (size_t i = 0; i < value.size(); i++) {
    if (i > 0) {
      encoded << ", ";
    }
    encoded << value[i];
  }
  encoded << "]";

  boost::property_tree::ptree output;
  output.data() = encoded.str();
  return output;
}

json VectorParameter::jsonValue() const
{
  json o;
  o["type"] = "number";
  o["initial"] = defaultValue;

  if (maximum.is_initialized()) {
    o["max"] = maximum.get();
    o["min"] = minimum.is_initialized() ? minimum.get() : 0.0;
    o["step"] = step.is_initialized() ? step.get() : 1.0;
  }
  return o;
}

void VectorParameter::apply(Assignment *assignment) const
{
  std::shared_ptr<Vector> vector = std::make_shared<Vector>(Location::NONE);
  for (double item : value) {
    vector->emplace_back(new Literal(item));
  }
  assignment->setExpr(std::move(vector));
}

bool EnumParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
  bool found = false;
  int index;
  boost::optional<double> decodedDouble = encodedValue.get_value_optional<double>();
  for (size_t i = 0; i < items.size(); i++) {
    if ((decodedDouble && items[i].value == EnumValue(*decodedDouble)) || items[i].value == EnumValue(encodedValue.data())) {
      index = i;
      found = true;
      break;
    }
  }

  if (!found) {
    return false;
  }
  if (store) {
    valueIndex = index;
  }
  return true;
}

boost::property_tree::ptree EnumParameter::exportValue() const
{
  const EnumValue& itemValue = items[valueIndex].value;
  boost::property_tree::ptree output;
  if (std::holds_alternative<double>(itemValue)) {
    output.put_value<double>(std::get<double>(itemValue));
  } else {
    output.data() = std::get<std::string>(itemValue);
  }
  return output;
}

json EnumParameter::jsonValue() const
{
  json o;
  if (set_enum_value(o, "initial", items[defaultValueIndex])) {
    o["type"] = "number";
  } else {
    o["type"] = "string";
  }

  json options;
  for (const auto& item : items) {
    json option;
    option["name"] = item.key;
    set_enum_value(option, "value", item);
    options.push_back(option);
  }
  o["options"] = options;

  return o;
}

void EnumParameter::apply(Assignment *assignment) const
{
  const EnumValue& itemValue = items[valueIndex].value;
  if (std::holds_alternative<double>(itemValue)) {
    assignment->setExpr(std::make_shared<Literal>(std::get<double>(itemValue)));
  } else {
    assignment->setExpr(std::make_shared<Literal>(std::get<std::string>(itemValue)));
  }
}



struct EnumValues
{
  std::vector<EnumParameter::EnumItem> items;
  int defaultValueIndex;
};
static EnumValues parseEnumItems(const Expression *parameter, const std::string& defaultKey, const EnumParameter::EnumValue& defaultValue)
{
  EnumValues output;

  const auto *expression = dynamic_cast<const Vector *>(parameter);
  if (!expression) {
    return output;
  }

  std::vector<EnumParameter::EnumItem> items;
  const auto& elements = expression->getChildren();
  for (const auto& elementPointer : elements) {
    EnumParameter::EnumItem item;
    if (const auto *element = dynamic_cast<const Literal *>(elementPointer.get())) {
      // string or number literal
      if (element->isDouble()) {
        if (elements.size() == 1) {
          // a vector with a single numeric element is not an enum specifier,
          // it's a range with a maximum and no minimum.
          return output;
        }
        item.value = element->toDouble();
        item.key = STR(element->toDouble());
      } else if (element->isString()) {
        item.value = element->toString();
        item.key = element->toString();
      } else {
        return output;
      }
    } else if (const auto *element = dynamic_cast<const Vector *>(elementPointer.get())) {
      // [value, key] vector
      if (element->getChildren().size() != 2) {
        return output;
      }

      const auto *key = dynamic_cast<const Literal *>(element->getChildren()[1].get());
      if (!key) {
        return output;
      }
      if (key->isDouble()) {
        item.key = STR(key->toDouble());
      } else if (key->isString()) {
        item.key = key->toString();
      } else {
        return output;
      }

      const auto *value = dynamic_cast<const Literal *>(element->getChildren()[0].get());
      if (!value) {
        return output;
      }
      if (value->isDouble()) {
        item.value = value->toDouble();
      } else if (value->isString()) {
        item.value = value->toString();
      } else {
        return output;
      }
    } else {
      return output;
    }
    items.push_back(item);
  }

  output.items = std::move(items);
  for (size_t i = 0; i < output.items.size(); i++) {
    if (defaultValue == output.items[i].value) {
      output.defaultValueIndex = i;
      return output;
    }
  }
  EnumParameter::EnumItem defaultItem;
  defaultItem.key = defaultKey;
  defaultItem.value = defaultValue;
  output.items.insert(output.items.begin(), defaultItem);
  output.defaultValueIndex = 0;
  return output;
}

struct NumericLimits
{
  boost::optional<double> minimum;
  boost::optional<double> maximum;
  boost::optional<double> step;
};
static NumericLimits parseNumericLimits(const Expression *parameter, const std::vector<double>& values)
{
  NumericLimits output;

  if (const auto *step = dynamic_cast<const Literal *>(parameter)) {
    if (step->isDouble()) {
      output.step = step->toDouble();
    }
  } else if (const auto *maximum = dynamic_cast<const Vector *>(parameter)) {
    if (maximum->getChildren().size() == 1) {
      const auto *maximumChild = dynamic_cast<const Literal *>(maximum->getChildren()[0].get());
      if (maximumChild && maximumChild->isDouble()) {
        output.maximum = maximumChild->toDouble();
      }
    }
  } else if (const auto *range = dynamic_cast<const Range *>(parameter)) {
    const auto *minimum = dynamic_cast<const Literal *>(range->getBegin());
    const auto *maximum = dynamic_cast<const Literal *>(range->getEnd());
    if (
      minimum && minimum->isDouble()
      && maximum && maximum->isDouble()
      ) {
      output.minimum = minimum->toDouble();
      output.maximum = maximum->toDouble();

      const auto *step = dynamic_cast<const Literal *>(range->getStep());
      if (step && step->isDouble()) {
        output.step = step->toDouble();
      }
    }
  }
  for (double value : values) {
    if (output.minimum && value < output.minimum) {
      output.minimum = value;
    }
    if (output.maximum && value > output.maximum) {
      output.maximum = value;
    }
  }

  return output;
}

std::unique_ptr<ParameterObject> ParameterObject::fromAssignment(const Assignment *assignment)
{
  std::string name = assignment->getName();

  const Expression *parameter = nullptr;
  const Annotation *parameterAnnotation = assignment->annotation("Parameter");
  if (!parameterAnnotation) {
    return nullptr;
  }
  parameter = parameterAnnotation->getExpr().get();

  std::string description;
  const Annotation *descriptionAnnotation = assignment->annotation("Description");
  if (descriptionAnnotation) {
    const auto *expression = dynamic_cast<const Literal *>(descriptionAnnotation->getExpr().get());
    if (expression && expression->isString()) {
      description = expression->toString();
    }
  }

  std::string group = "Parameters";
  const Annotation *groupAnnotation = assignment->annotation("Group");
  if (groupAnnotation) {
    const auto *expression = dynamic_cast<const Literal *>(groupAnnotation->getExpr().get());
    if (expression && expression->isString()) {
      group = boost::algorithm::trim_copy(expression->toString());

    }
    if (group == "Hidden") return nullptr;
  }

  const Expression *valueExpression = assignment->getExpr().get();
  if (const auto *expression = dynamic_cast<const Literal *>(valueExpression)) {
    if (expression->isBool()) {
      return std::make_unique<BoolParameter>(name, description, group, expression->toBool());
    }

    if (expression->isDouble() || expression->isString()) {
      std::string key;
      EnumParameter::EnumValue value;
      if (expression->isDouble()) {
        value = expression->toDouble();
        key = STR(expression->toDouble());
      } else {
        value = expression->toString();
        key = expression->toString();
      }
      EnumValues values = parseEnumItems(parameter, key, value);
      if (!values.items.empty()) {
        return std::make_unique<EnumParameter>(name, description, group, values.defaultValueIndex, values.items);
      }
    }

    if (expression->isString()) {
      std::string value = expression->toString();
      boost::optional<size_t> maximumSize = boost::none;
      const auto *maximumSizeExpression = dynamic_cast<const Literal *>(parameter);
      if (maximumSizeExpression && maximumSizeExpression->isDouble()) {
        maximumSize = (size_t)(maximumSizeExpression->toDouble());
      }
      return std::make_unique<StringParameter>(name, description, group, value, maximumSize);
    }

    if (expression->isDouble()) {
      double value = expression->toDouble();
      NumericLimits limits = parseNumericLimits(parameter, {value});
      return std::make_unique<NumberParameter>(name, description, group, value, limits.minimum, limits.maximum, limits.step);
    }
  } else if (const auto *expression = dynamic_cast<const Vector *>(valueExpression)) {
    if (expression->getChildren().size() < 1 || expression->getChildren().size() > 4) {
      return nullptr;
    }

    std::vector<double> value;
    for (const auto& element : expression->getChildren()) {
      const auto *item = dynamic_cast<const Literal *>(element.get());
      if (!item) {
        return nullptr;
      }
      if (!item->isDouble()) {
        return nullptr;
      }
      value.push_back(item->toDouble());
    }

    NumericLimits limits = parseNumericLimits(parameter, value);
    return std::make_unique<VectorParameter>(name, description, group, value, limits.minimum, limits.maximum, limits.step);
  }
  return nullptr;
}

ParameterObjects ParameterObjects::fromSourceFile(const SourceFile *sourceFile)
{
  ParameterObjects output;
  for (const auto& assignment : sourceFile->scope.assignments) {
    std::unique_ptr<ParameterObject> parameter = ParameterObject::fromAssignment(assignment.get());
    if (parameter) {
      output.push_back(std::move(parameter));
    }
  }
  return output;
}

void ParameterObjects::reset()
{
  for (const auto& parameter : *this) {
    parameter->reset();
  }
}

void ParameterObjects::importValues(const ParameterSet& values)
{
  for (const auto& parameter : *this) {
    auto it = values.find(parameter->name());
    if (it == values.end()) {
      parameter->reset();
    } else {
      parameter->importValue(it->second, true);
    }
  }
}

ParameterSet ParameterObjects::exportValues(const std::string& setName)
{
  ParameterSet output;
  output.setName(setName);
  for (const auto& parameter : *this) {
    output[parameter->name()] = parameter->exportValue();
  }
  return output;
}

void ParameterObjects::apply(SourceFile *sourceFile) const
{
  std::map<std::string, ParameterObject *> namedParameters;
  for (const auto& parameter : *this) {
    namedParameters[parameter->name()] = parameter.get();
  }

  for (auto& assignment : sourceFile->scope.assignments) {
    if (namedParameters.count(assignment->getName())) {
      namedParameters[assignment->getName()]->apply(assignment.get());
    }
  }
}
