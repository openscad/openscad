#include "core/customizer/ParameterSet.h"
#include "utils/printutils.h"
#include <boost/property_tree/json_parser.hpp>

#include <string>

static std::string parameterSetsKey("parameterSets");
static std::string fileFormatVersionKey("fileFormatVersion");
static std::string fileFormatVersionValue("1");

bool ParameterSets::readFile(const std::string& filename)
{
  boost::property_tree::ptree root;

  try {
    boost::property_tree::read_json(filename, root);
  } catch (const boost::property_tree::json_parser_error& e) {
    LOG(message_group::Error, "Cannot open Parameter Set '%1$s': %2$s", filename, e.what());
    return false;
  }

  boost::optional<boost::property_tree::ptree&> sets = root.get_child_optional(parameterSetsKey);
  if (!sets) {
    return false;
  }

  for (const auto& entry : *sets) {
    ParameterSet set;
    set.setName(entry.first);
    for (const auto& value : entry.second) {
      set[value.first] = value.second;
    }
    push_back(set);
  }
  return true;
}

void ParameterSets::writeFile(const std::string& filename) const
{
  boost::property_tree::ptree sets;
  for (const auto& set : *this) {
    boost::property_tree::ptree setTree;
    for (const auto& parameter : set) {
      setTree.push_back(boost::property_tree::ptree::value_type(parameter.first, parameter.second));
    }
    sets.push_back(boost::property_tree::ptree::value_type(set.name(), setTree));
  }

  boost::property_tree::ptree root;
  root.put<std::string>(fileFormatVersionKey, fileFormatVersionValue);
  root.push_back(boost::property_tree::ptree::value_type(parameterSetsKey, sets));

  try {
    boost::property_tree::write_json(filename, root);
  } catch (const boost::property_tree::json_parser_error& e) {
    LOG(message_group::Error, "Cannot write Parameter Set '%1$s': %2$s", filename, e.what());
  }
}
