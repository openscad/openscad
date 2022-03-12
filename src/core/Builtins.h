#pragma once

#include <string>
#include <unordered_map>
#include "module.h"
#include "LocalScope.h"
#include "Assignment.h"

class Builtins
{
public:
  static Builtins *instance(bool erase = false);
  static void init(const std::string& name, class AbstractModule *module);
  static void init(const std::string& name, class AbstractModule *module, const std::vector<std::string>& calltipList);
  static void init(const std::string& name, class BuiltinFunction *function, const std::vector<std::string>& calltipList);
  void initialize();
  std::string isDeprecated(const std::string& name) const;

  const auto& getAssignments() const { return this->assignments; }
  const auto& getFunctions() const { return this->functions; }
  const auto& getModules() const { return this->modules; }

  static std::unordered_map<std::string, const std::vector<std::string>> keywordList;

private:
  Builtins();
  ~Builtins() = default;

  static void initKeywordList();

  AssignmentList assignments;
  std::unordered_map<std::string, class BuiltinFunction *> functions;
  std::unordered_map<std::string, class AbstractModule *> modules;

  std::unordered_map<std::string, std::string> deprecations;
};
