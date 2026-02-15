#include "core/customizer/ParameterObject.h"

#include <boost/algorithm/string.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/AST.h"
#include "core/customizer/Annotation.h"
#include "core/Context.h"
#include "core/Assignment.h"
#include "core/Expression.h"
#include "core/SourceFile.h"
#include "core/Value.h"
#include "core/customizer/Annotation.h"
#include "utils/printutils.h"

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

}  // namespace

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

StringParameter::StringParameter(const std::string& name, const std::string& description,
                                 const std::string& group, const std::string& defaultValue,
                                 boost::optional<size_t> maximumSize)
  : ParameterObject(name, description, group, ParameterObject::ParameterType::String),
    value(defaultValue),
    defaultValue(defaultValue),
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
    if ((decodedDouble && items[i].value == EnumValue(*decodedDouble)) ||
        items[i].value == EnumValue(encodedValue.data())) {
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

struct EnumValues {
  std::vector<EnumParameter::EnumItem> items;
  int defaultValueIndex;
};
static EnumValues parseEnumItems(const Expression *parameter, const std::string& defaultKey,
                                 const EnumParameter::EnumValue& defaultValue)
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

struct NumericLimits {
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
    if (minimum && minimum->isDouble() && maximum && maximum->isDouble()) {
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

static std::unique_ptr<ParameterObject> createParameter(const std::string& name,
                                                        const std::string& description,
                                                        const std::string& group,
                                                        const Expression *parameterExpr,
                                                        const Expression *valueExpression)
{
  if (const auto *lit = dynamic_cast<const Literal *>(valueExpression)) {
    if (lit->isBool()) return std::make_unique<BoolParameter>(name, description, group, lit->toBool());

    if (lit->isDouble() || lit->isString()) {
      std::string key = lit->isDouble() ? STR(lit->toDouble()) : lit->toString();
      EnumParameter::EnumValue value = lit->isDouble() ? EnumParameter::EnumValue(lit->toDouble())
                                                       : EnumParameter::EnumValue(lit->toString());
      EnumValues values = parseEnumItems(parameterExpr, key, value);
      if (!values.items.empty())
        return std::make_unique<EnumParameter>(name, description, group, values.defaultValueIndex,
                                               values.items);
    }

    if (lit->isString()) {
      boost::optional<size_t> maximumSize;
      if (const auto *maxLit = dynamic_cast<const Literal *>(parameterExpr)) {
        if (maxLit->isDouble()) maximumSize = (size_t)maxLit->toDouble();
      }
      return std::make_unique<StringParameter>(name, description, group, lit->toString(), maximumSize);
    }

    if (lit->isDouble()) {
      double val = lit->toDouble();
      NumericLimits limits = parseNumericLimits(parameterExpr, {val});
      return std::make_unique<NumberParameter>(name, description, group, val, limits.minimum,
                                               limits.maximum, limits.step);
    }
  } else if (const auto *vec = dynamic_cast<const Vector *>(valueExpression)) {
    std::vector<double> values;
    for (const auto& child : vec->getChildren()) {
      if (const auto *item = dynamic_cast<const Literal *>(child.get())) {
        if (item->isDouble()) values.push_back(item->toDouble());
      }
    }
    if (values.size() >= 1 && values.size() <= 4) {
      NumericLimits limits = parseNumericLimits(parameterExpr, values);
      return std::make_unique<VectorParameter>(name, description, group, values, limits.minimum,
                                               limits.maximum, limits.step);
    }
  }
  return nullptr;
}

std::unique_ptr<ParameterObject> ParameterObject::fromAssignment(const Assignment *assignment,
                                                                 const Context *context)
{
  const Annotation *nativeAnn = assignment->annotation("NativeAttributes");
  if (nativeAnn) {
    if (auto *obj = dynamic_cast<const ObjectExpression *>(nativeAnn->getExpr().get())) {
      return fromObjectExpression(obj, assignment, context);
    }
  }

  const Annotation *paramAnn = assignment->annotation("Parameter");
  if (!paramAnn) return nullptr;

  const Expression *parameterExpr = paramAnn->getExpr().get();
  std::string description;
  std::string group = "Parameters";

  const Annotation *descAnn = assignment->annotation("Description");
  if (descAnn) {
    if (auto *lit = dynamic_cast<const Literal *>(descAnn->getExpr().get())) {
      if (lit->isString()) description = lit->toString();
    }
  }

  const Annotation *groupAnn = assignment->annotation("Group");
  if (groupAnn) {
    if (auto *lit = dynamic_cast<const Literal *>(groupAnn->getExpr().get())) {
      if (lit->isString()) group = boost::algorithm::trim_copy(lit->toString());
    }
    if (group == "Hidden") return nullptr;
  }

  return createParameter(assignment->getName(), description, group, parameterExpr,
                         assignment->getExpr().get());
}

std::unique_ptr<ParameterObject> ParameterObject::fromObjectExpression(const ObjectExpression *nativeObj,
                                                                       const Assignment *assignment,
                                                                       const Context *context)
{
  std::string description, group = "Parameters";
  bool isLocked = false;
  bool isHidden = false;
  const Expression *rangeExpr = nullptr;
  std::shared_ptr<Expression> lockedExpr;
  std::shared_ptr<Expression> hiddenExpr;
  // std::shared_ptr<Expression> descExpr;
  std::set<std::string> tempDependencies;
  for (const auto& member : nativeObj->getMembers()) {
    std::string key = member->getName();
    const Expression *expr = member->getExpr().get();

    if (key == "locked") {
      lockedExpr = member->getExpr();
    } else if (key == "hidden") {
      hiddenExpr = member->getExpr();
    } else {
      // print no key was found
    }
    if (expr) {
      expr->collectDependencies(tempDependencies);
    }

    if (context) {
      try {
        Value val = expr->evaluate(context->get_shared_ptr());
        if (key == "description" || key == "desc") {
          description = " " + val.toStrUtf8Wrapper().toString();
        } else if (key == "group") {
          group = val.toStrUtf8Wrapper().toString();
        } else if (key == "locked") {
          isLocked = val.toBool();
        } else if (key == "hidden") {
          isHidden = val.toBool();
        } else if (key == "range" || key == "min" || key == "max" || key == "step") {
          // TODO: add support for dynamic range expressions
          rangeExpr = expr;
        }
      } catch (...) {  // TODO: handle evaluation errors
                       // if evaluation fails (e.g. variable not found), consume the error
                       // and fall back to the static literal parsing.
      }
    }

    if (description.empty() && key == "description") {
      if (const auto *lit = dynamic_cast<const Literal *>(expr)) description = " " + lit->toString();
    } else if (group == "Parameters" && key == "group") {
      if (const auto *lit = dynamic_cast<const Literal *>(expr)) group = lit->toString();
    }

    if ((key == "range" || key == "min" || key == "max" || key == "step") && !rangeExpr) {
      rangeExpr = expr;
    }
  }

  auto param =
    createParameter(assignment->getName(), description, group, rangeExpr, assignment->getExpr().get());
  if (param) {
    param->setLocked(isLocked);
    param->setHidden(isHidden);
    param->setLockedExpression(lockedExpr);

    param->getDependencies() = std::move(tempDependencies);
  }
  return param;
}

ParameterObjects ParameterObjects::fromSourceFile(const SourceFile *sourceFile, const Context *context)
{
  ParameterObjects output;
  for (const auto& assignment : sourceFile->scope->assignments) {
    std::unique_ptr<ParameterObject> parameter =
      ParameterObject::fromAssignment(assignment.get(), context);
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

  for (auto& assignment : sourceFile->scope->assignments) {
    if (namedParameters.count(assignment->getName())) {
      namedParameters[assignment->getName()]->apply(assignment.get());
    }
  }
}

void ParameterObject::updateAttributes(const Context *context)
{
  if (context) {
    if (this->lockedExpr) {
      try {
        bool newLocked = this->lockedExpr->evaluate(context->get_shared_ptr()).toBool();
        this->setLocked(newLocked);
      } catch (...) {
      }
    }
    if (this->hiddenExpr) {
      try {
        bool newHidden = this->hiddenExpr->evaluate(context->get_shared_ptr()).toBool();
        this->setHidden(newHidden);
      } catch (...) {
      }
    }
  }
}

void BoolParameter::updateContext(Context *context) const
{
  context->set_variable(name_, Value(value));
}

void StringParameter::updateContext(Context *context) const
{
  context->set_variable(name_, Value(value));
}

void NumberParameter::updateContext(Context *context) const
{
  context->set_variable(name_, Value(value));
}

void VectorParameter::updateContext(Context *context) const
{
  // We need to construct a Vector Value
  Value::VectorType vecType(nullptr);
  for (double v : value) {
    vecType.emplace_back(Value(v));
  }
  context->set_variable(name_, Value(std::move(vecType)));
}

void EnumParameter::updateContext(Context *context) const
{
  const EnumValue& itemValue = items[valueIndex].value;
  if (std::holds_alternative<double>(itemValue)) {
    context->set_variable(name_, Value(std::get<double>(itemValue)));
  } else {
    context->set_variable(name_, Value(std::get<std::string>(itemValue)));
  }
}
