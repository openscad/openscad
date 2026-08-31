#pragma once

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>
#include <vector>

#include "core/str_utf8_wrapper.h"

class ParameterSet : public std::map<std::string, boost::property_tree::ptree>
{
public:
  [[nodiscard]] const std::string& name() const { return _name; }
  // Normalised here rather than at the call sites, so that a name is in NFC no
  // matter whether it came from a parameter set file or from the customizer.
  void setName(const std::string& name) { _name = normalize_utf8_nfc(name); }

private:
  std::string _name;
};

class ParameterSets : public std::vector<ParameterSet>
{
public:
  bool readFile(const std::string& filename);
  void writeFile(const std::string& filename) const;
};
