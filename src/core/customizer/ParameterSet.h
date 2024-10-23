#pragma once

#include <map>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

class ParameterSet : public std::map<std::string, boost::property_tree::ptree>
{
public:
  [[nodiscard]] const std::string& name() const { return _name; }
  void setName(const std::string& name) { _name = name; }

private:
  std::string _name;
};

class ParameterSets : public std::vector<ParameterSet>
{
public:
  bool readFile(const std::string& filename);
  void writeFile(const std::string& filename) const;
};
