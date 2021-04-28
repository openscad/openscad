#include "annotation.h"
#include "comment.h"
#include "modcontext.h"
#include "module.h"
#include "parameterobject.h"

ParameterObject::ParameterObject(std::shared_ptr<Context> ctx, const Assignment* assignment, const Value &defaultValue) :
    value(Value::undefined.clone()), values(Value::undefined.clone()), defaultValue(Value::undefined.clone())
{
  this->name = assignment->getName();
  const Annotation *param = assignment->annotation("Parameter");
  setValue(defaultValue, param->evaluate(ctx));
  const Annotation *desc = assignment->annotation("Description");

  if (desc) {
    Value v = desc->evaluate(ctx);
    if (v.type() == Value::Type::STRING) {
      description=v.toString();
    }
  }
  
  const Annotation *group = assignment->annotation("Group");
  if (group) {
    Value v = group->evaluate(ctx);
    if (v.type() == Value::Type::STRING) {
      groupName = v.toString();
    }
  } else {
    groupName = "Parameters";
  }
}

void ParameterObject::setValue(const Value &defaultValue, const Value &values)
{
  this->values = values.clone();
  this->value = defaultValue.clone();
  this->defaultValue = defaultValue.clone();
  this->vt = values.type();
  this->dvt = defaultValue.type();
 
  bool makerBotMax = (vt == Value::Type::VECTOR && values.toVector().size() == 1 && values.toVector()[0].type() == Value::Type::NUMBER); // [max] format from makerbot customizer

  if (dvt == Value::Type::BOOL) {
    this->target = CHECKBOX;
  } else if (dvt == Value::Type::VECTOR) {
    this->target = VECTOR;
  } else if ((vt == Value::Type::RANGE || makerBotMax) && (dvt == Value::Type::NUMBER)) {
    this->target = SLIDER;
  } else if ((makerBotMax) && (dvt == Value::Type::STRING)){
    this->target = TEXT;
  } else if ((vt == Value::Type::VECTOR) && ((dvt == Value::Type::NUMBER) || (dvt == Value::Type::STRING))) {
    this->target = COMBOBOX;
  } else if (dvt == Value::Type::NUMBER) {
    this->target = NUMBER;
  } else {
    this->target = TEXT;
  }
}

bool ParameterObject::isVector4(const Value& defaultValue)
{
	const auto &vec = defaultValue.toVector();
	if (vec.size() < 1 || vec.size() > 4) {
		return false;
	}
	for (size_t i = 0; i < vec.size(); ++i) {
		if (vec[i].type() != Value::Type::NUMBER) {
			return false;
		}
	}
	return true;
}

std::unique_ptr<ParameterObject> ParameterObject::fromAssignment(const Assignment* assignment)
{
	const Annotation *param = assignment->annotation("Parameter");
	if (!param) {
		return nullptr;
	}

	ContextHandle<Context> ctx{Context::create<Context>()};
	Value defaultValue = assignment->getExpr()->evaluate(ctx.ctx);
	if (defaultValue.type() == Value::Type::UNDEFINED) {
		return nullptr;
	} else if (defaultValue.type() == Value::Type::VECTOR && !isVector4(defaultValue)) {
		return nullptr;
	}

	return std::make_unique<ParameterObject>(ctx.ctx, assignment, std::move(defaultValue));
}

void ParameterObject::reset()
{
	value = defaultValue.clone();
}

bool ParameterObject::importValue(boost::property_tree::ptree encodedValue, bool store)
{
	if (dvt == Value::Type::STRING) {
		if (store) {
			value = Value(encodedValue.data());
		}
	} else if (dvt == Value::Type::BOOL) {
		if (store) {
			value = encodedValue.get_value<bool>();
		}
	} else {
		shared_ptr<Expression> params = CommentParser::parser(encodedValue.data().c_str());
		if (!params) {
			if (store) {
				reset();
			}
			return false;
		}
		ContextHandle<Context> ctx{Context::create<Context>()};
		Value newValue = params->evaluate(ctx.ctx);
		if (dvt == newValue.type()) {
			if (store) {
				value = std::move(newValue);
			}
		} else {
			if (store) {
				reset();
			}
			return false;
		}
	}
	return true;
}

boost::property_tree::ptree ParameterObject::exportValue() const
{
	boost::property_tree::ptree output;
	output.data() = value.toString();
	return output;
}

void ParameterObject::apply(Assignment* assignment) const
{
	ContextHandle<Context> ctx{Context::create<Context>()};
	Value defaultValue = assignment->getExpr()->evaluate(ctx.ctx);
	
	if (defaultValue.type() == dvt) {
		assignment->setExpr(make_shared<Literal>(value.clone()));
	}
}

ParameterObjects ParameterObjects::fromModule(const FileModule* module)
{
	ParameterObjects output;
	ContextHandle<Context> ctx{Context::create<Context>()};
	for (const auto& assignment : module->scope.assignments) {
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
		auto it = values.find(parameter->name);
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
		output[parameter->name] = parameter->exportValue();
	}
	return output;
}

void ParameterObjects::apply(FileModule *fileModule) const
{
	std::map<std::string, ParameterObject*> namedParameters;
	for (const auto& parameter : *this) {
		namedParameters[parameter->name] = parameter.get();
	}
	
	for (auto& assignment : fileModule->scope.assignments) {
		if (namedParameters.count(assignment->getName())) {
			namedParameters[assignment->getName()]->apply(assignment.get());
		}
	}
}
