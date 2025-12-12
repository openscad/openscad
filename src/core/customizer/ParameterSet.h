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

// Structure to hold JSON parsing error details
struct JsonErrorInfo {
  std::string message;     // The error message (e.g., "expected '}' or ','")
  std::string filename;    // The JSON file path
  unsigned long line = 0;  // Line number where error occurred (0 if unknown)

  bool hasError() const { return !message.empty(); }
  void clear()
  {
    message.clear();
    filename.clear();
    line = 0;
  }
};

class ParameterSets : public std::vector<ParameterSet>
{
public:
  bool readFile(const std::string& filename, JsonErrorInfo& errorInfo);
  void writeFile(const std::string& filename) const;
};
