#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "core/module.h"
#include "core/Assignment.h"

class AbstractModule;
class BuiltinFunction;

void initialize_rng();

class Builtins
{
public:
  static Builtins *instance(bool erase = false);
  static void init(const std::string& name, AbstractModule *module);
  static void init(const std::string& name, AbstractModule *module, const std::vector<std::string>& calltipList);
  static void init(const std::string& name, BuiltinFunction *function, const std::vector<std::string>& calltipList);
  void initialize();
  std::string isDeprecated(const std::string& name) const;

  const auto& getAssignments() const { return this->assignments; }
  const auto& getFunctions() const { return this->functions; }
  const auto& getModules() const { return this->modules; }

  static std::unordered_map<std::string, const std::vector<std::string>> keywordList;

private:
  Builtins();
  virtual ~Builtins() = default;

  static void initKeywordList();

  AssignmentList assignments;
  std::unordered_map<std::string, BuiltinFunction *> functions;
  std::unordered_map<std::string, AbstractModule *> modules;

  std::unordered_map<std::string, std::string> deprecations;
};
