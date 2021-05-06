#include "annotation.h"
#include "module.h"
#include "parameterobject.h"
#include "value.h"

#include <sstream>
#include <boost/algorithm/string.hpp>

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

void BoolParameter::apply(Assignment* assignment) const
{
	assignment->setExpr(std::make_shared<Literal>(Value(value)));
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

void StringParameter::apply(Assignment* assignment) const
{
	assignment->setExpr(std::make_shared<Literal>(Value(value)));
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

void NumberParameter::apply(Assignment* assignment) const
{
	assignment->setExpr(std::make_shared<Literal>(Value(value)));
}

bool VectorParameter::importValue(boost::property_tree::ptree encodedValue, bool store)
{
	std::vector<double> decoded;
	
	std::string encoded = boost::algorithm::replace_all_copy(encodedValue.data(), " ", "");
	if (encoded.size() < 2 || encoded[0] != '[' || encoded[encoded.size() - 1] != ']') {
		return false;
	}
	encoded.erase(encoded.begin());
	encoded.erase(encoded.end() - 1);
	
	std::vector<std::string> items;
	boost::algorithm::split(items, encoded, boost::algorithm::is_any_of(","));
	
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

void VectorParameter::apply(Assignment* assignment) const
{
	std::shared_ptr<Vector> vector = std::make_shared<Vector>(Location::NONE);
	for (double item : value) {
		vector->emplace_back(new Literal(Value(item)));
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
	EnumValue itemValue = items[valueIndex].value;
	boost::property_tree::ptree output;
	double* doubleValue = boost::get<double>(&itemValue);
	if (doubleValue) {
		output.put_value<double>(*doubleValue);
	} else {
		output.data() = boost::get<std::string>(itemValue);
	}
	return output;
}

void EnumParameter::apply(Assignment* assignment) const
{
	EnumValue itemValue = items[valueIndex].value;
	double* doubleValue = boost::get<double>(&itemValue);
	if (doubleValue) {
		assignment->setExpr(std::make_shared<Literal>(Value(*doubleValue)));
	} else {
		assignment->setExpr(std::make_shared<Literal>(Value(boost::get<std::string>(itemValue))));
	}
}






struct EnumValues
{
	std::vector<EnumParameter::EnumItem> items;
	int defaultValueIndex;
};
static EnumValues parseEnumItems(const Expression* parameter, const std::string& defaultKey, EnumParameter::EnumValue defaultValue)
{
	EnumValues output;
	
	const Vector* expression = dynamic_cast<const Vector*>(parameter);
	if (!expression) {
		return output;
	}
	
	std::vector<EnumParameter::EnumItem> items;
	const auto& elements = expression->getChildren();
	for (const auto& elementPointer : elements) {
		EnumParameter::EnumItem item;
		if (const Literal* element = dynamic_cast<const Literal*>(elementPointer.get())) {
			// string or number literal
			if (element->getValue().type() == Value::Type::NUMBER) {
				if (elements.size() == 1) {
					// a vector with a single numeric element is not an enum specifier,
					// it's a range with a maximum and no minimum.
					return output;
				}
				
				item.value = element->getValue().toDouble();
				item.key = element->getValue().toEchoString();
			} else if (element->getValue().type() == Value::Type::STRING) {
				item.value = element->getValue().toString();
				item.key = element->getValue().toString();
			} else {
				return output;
			}
		} else if (const Vector* element = dynamic_cast<const Vector*>(elementPointer.get())) {
			// [value, key] vector
			if (element->getChildren().size() != 2) {
				return output;
			}
			
			const Literal* key = dynamic_cast<const Literal*>(element->getChildren()[1].get());
			if (!key) {
				return output;
			}
			if (key->getValue().type() == Value::Type::NUMBER) {
				item.key = key->getValue().toEchoString();
			} else if (key->getValue().type() == Value::Type::STRING) {
				item.key = key->getValue().toString();
			} else {
				return output;
			}
			
			const Literal* value = dynamic_cast<const Literal*>(element->getChildren()[0].get());
			if (!value) {
				return output;
			}
			if (value->getValue().type() == Value::Type::NUMBER) {
				item.value = value->getValue().toDouble();
			} else if (value->getValue().type() == Value::Type::STRING) {
				item.value = value->getValue().toString();
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
static NumericLimits parseNumericLimits(const Expression* parameter)
{
	NumericLimits output;
	
	if (const Literal* step = dynamic_cast<const Literal*>(parameter)) {
		if (step->getValue().type() == Value::Type::NUMBER) {
			output.step = step->getValue().toDouble();
		}
	} else if (const Vector* maximum = dynamic_cast<const Vector*>(parameter)) {
		if (maximum->getChildren().size() == 1) {
			const Literal* maximumChild = dynamic_cast<const Literal*>(maximum->getChildren()[0].get());
			if (maximumChild && maximumChild->getValue().type() == Value::Type::NUMBER) {
				output.maximum = maximumChild->getValue().toDouble();
			}
		}
	} else if (const Range* range = dynamic_cast<const Range*>(parameter)) {
		const Literal* minimum = dynamic_cast<const Literal*>(range->getBegin());
		const Literal* maximum = dynamic_cast<const Literal*>(range->getEnd());
		if (
			   minimum && minimum->getValue().type() == Value::Type::NUMBER
			&& maximum && maximum->getValue().type() == Value::Type::NUMBER
		) {
			output.minimum = minimum->getValue().toDouble();
			output.maximum = maximum->getValue().toDouble();
			
			const Literal* step = dynamic_cast<const Literal*>(range->getStep());
			if (step && step->getValue().type() == Value::Type::NUMBER) {
				output.step = step->getValue().toDouble();
			}
		}
	}
	
	return output;
}

std::unique_ptr<ParameterObject> ParameterObject::fromAssignment(const Assignment* assignment)
{
	std::string name = assignment->getName();
	
	std::string description;
	const Annotation* descriptionAnnotation = assignment->annotation("Description");
	if (descriptionAnnotation) {
		const Literal* expression = dynamic_cast<const Literal*>(descriptionAnnotation->getExpr().get());
		if (expression && expression->getValue().type() == Value::Type::STRING) {
			description = expression->getValue().toString();
		}
	}
	
	std::string group = "Parameters";
	const Annotation* groupAnnotation = assignment->annotation("Group");
	if (groupAnnotation) {
		const Literal* expression = dynamic_cast<const Literal*>(groupAnnotation->getExpr().get());
		if (expression && expression->getValue().type() == Value::Type::STRING) {
			group = expression->getValue().toString();
		}
	}
	
	const Expression* parameter = nullptr;
	const Annotation* parameterAnnotation = assignment->annotation("Parameter");
	if (parameterAnnotation) {
		parameter = parameterAnnotation->getExpr().get();
	}
	
	const Expression* valueExpression = assignment->getExpr().get();
	if (const Literal* expression = dynamic_cast<const Literal*>(valueExpression)) {
		Value::Type type = expression->getValue().type();
		if (type == Value::Type::BOOL) {
			bool value = expression->getValue().toBool();
			return std::make_unique<BoolParameter>(name, description, group, value);
		}
		
		if (type == Value::Type::NUMBER || type == Value::Type::STRING) {
			std::string key;
			EnumParameter::EnumValue value;
			if (type == Value::Type::NUMBER) {
				value = expression->getValue().toDouble();
				key = expression->getValue().toEchoString();
			} else {
				value = expression->getValue().toString();
				key = expression->getValue().toString();
			}
			EnumValues values = parseEnumItems(parameter, key, value);
			if (!values.items.empty()) {
				return std::make_unique<EnumParameter>(name, description, group, values.defaultValueIndex, values.items);
			}
		}
		
		if (type == Value::Type::STRING) {
			std::string value = expression->getValue().toString();
			boost::optional<int> maximumSize = boost::none;
			const Literal* maximumSizeExpression = dynamic_cast<const Literal*>(parameter);
			if (maximumSizeExpression && maximumSizeExpression->getValue().type() == Value::Type::NUMBER) {
				maximumSize = (int)(maximumSizeExpression->getValue().toDouble());
			}
			return std::make_unique<StringParameter>(name, description, group, value, maximumSize);
		}
		
		if (type == Value::Type::NUMBER) {
			double value = expression->getValue().toDouble();
			NumericLimits limits = parseNumericLimits(parameter);
			return std::make_unique<NumberParameter>(name, description, group, value, limits.minimum, limits.maximum, limits.step);
		}
	} else if (const Vector* expression = dynamic_cast<const Vector*>(valueExpression)) {
		if (expression->getChildren().size() < 1 || expression->getChildren().size() > 4) {
			return nullptr;
		}
		
		std::vector<double> value;
		for (const auto& element : expression->getChildren()) {
			const Literal* item = dynamic_cast<const Literal*>(element.get());
			if (!item) {
				return nullptr;
			}
			if (item->getValue().type() != Value::Type::NUMBER) {
				return nullptr;
			}
			value.push_back(item->getValue().toDouble());
		}
		
		NumericLimits limits = parseNumericLimits(parameter);
		return std::make_unique<VectorParameter>(name, description, group, value, limits.minimum, limits.maximum, limits.step);
	}
	return nullptr;
}

ParameterObjects ParameterObjects::fromSourceFile(const SourceFile* sourceFile)
{
	ParameterObjects output;
	for (const auto& assignment : sourceFile->scope.assignments) {
		std::unique_ptr<ParameterObject> parameter = ParameterObject::fromAssignment(assignment.get());
		if (parameter) {
			output.push_back(std::move(parameter));
		}
	}
	return std::move(output);
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

void ParameterObjects::apply(SourceFile* sourceFile) const
{
	std::map<std::string, ParameterObject*> namedParameters;
	for (const auto& parameter : *this) {
		namedParameters[parameter->name()] = parameter.get();
	}
	
	for (auto& assignment : sourceFile->scope.assignments) {
		if (namedParameters.count(assignment->getName())) {
			namedParameters[assignment->getName()]->apply(assignment.get());
		}
	}
}
