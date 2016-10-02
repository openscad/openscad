#include "parameterset.h"
#include "comment.h"
#include "modcontext.h"
#include "expression.h"

#include <boost/property_tree/json_parser.hpp>

std::string ParameterSet::parameterSetsKey("parameterSets");
std::string ParameterSet::fileFormatVersionKey("fileFormatVersion");
std::string ParameterSet::fileFormatVersionValue("1");

boost::optional<pt::ptree &> ParameterSet::parameterSets()
{
	return root.get_child_optional(ParameterSet::parameterSetsKey);
}

std::vector<std::string> ParameterSet::getParameterNames()
{
	std::vector<std::string> names;

	boost::optional<pt::ptree &> sets = parameterSets();
	if (sets.is_initialized()) {
		for (const auto &v : sets.get()) {
			names.push_back(v.first);
		}
	}
	return names;
}

boost::optional<pt::ptree &> ParameterSet::getParameterSet(const std::string &setName)
{
	boost::optional<pt::ptree &> sets = root.get_child_optional(ParameterSet::parameterSetsKey);
	if (!sets.is_initialized()) {
		return sets;
	}

	boost::optional<pt::ptree &> set = sets.get().get_child_optional(setName);
	return set;
}

void ParameterSet::addParameterSet(const std::string setName, const pt::ptree & set)
{
	boost::optional<pt::ptree &> sets = parameterSets();
	if (sets.is_initialized()) {
		sets.get().erase(setName);
	}

	root.add_child(ParameterSet::parameterSetsKey + "." + setName, set);
}

void ParameterSet::readParameterSet(const std::string &filename)
{
  try {
    pt::read_json(filename, this->root);
  }
  catch (const pt::json_parser_error &e) {
    // FIXME: Better error handling
    std::cerr << e.what() << std::endl;
  }
}

void ParameterSet::writeParameterSet(const std::string &filename)
{
  // Always force default version for now
  root.put<std::string>(fileFormatVersionKey, fileFormatVersionValue);

  try {
    pt::write_json(filename, this->root);
  }
  catch (const pt::json_parser_error &e) {
    // FIXME: Better error handling
    std::cerr << e.what() << std::endl;
  }
}

void ParameterSet::applyParameterSet(FileModule *fileModule, const std::string &setName)
{
  if (fileModule == NULL || this->root.empty()) return;
  try {
    ModuleContext ctx;
    std::string path = parameterSetsKey + "." + setName;
    for (auto &assignment : fileModule->scope.assignments) {
      for (auto &v : root.get_child(path)) {
        if (v.first == assignment.name) {
          const ValuePtr defaultValue = assignment.expr->evaluate(&ctx);
          if (defaultValue->type() == Value::STRING) {
            assignment.expr = shared_ptr<Expression>(new Literal(ValuePtr(v.second.data())));
          }
          else if (defaultValue->type() == Value::BOOL) {
            assignment.expr = shared_ptr<Expression>(new Literal(ValuePtr(v.second.get_value<bool>())));
          } else {
            shared_ptr<Expression> params = CommentParser::parser(v.second.data().c_str());
            if (!params) continue;
            ModuleContext ctx;
            if (defaultValue->type() == params->evaluate(&ctx)->type()) {
              assignment.expr = params;
            }
          }
        }
      }
    }
  }
  catch (std::exception const& e) {
    // FIXME: Better error handling
    std::cerr << e.what() << std::endl;
  }
}

