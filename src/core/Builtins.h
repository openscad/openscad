#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/Assignment.h"

class AbstractModule;
class BuiltinFunction;

void initialize_rng();

class Builtins
{
public:
  Builtins(const Builtins&) = delete;
  Builtins& operator=(const Builtins&) = delete;
  Builtins(Builtins&&) = delete;
  Builtins& operator=(Builtins&&) = delete;

  static Builtins& instance();

  static void init(const std::string& name, AbstractModule *module);
  static void init(const std::string& name, AbstractModule *module,
                   const std::vector<std::string>& calltipList);
  static void init(const std::string& name, BuiltinFunction *function,
                   const std::vector<std::string>& calltipList);
  static void initialize();
  std::string isDeprecated(const std::string& name) const;

  const auto& getAssignments() const { return assignments; }
  const auto& getFunctions() const { return functions; }
  const auto& getModules() const { return modules; }

  static std::unordered_map<std::string, std::vector<std::string>> keywordList;

private:
  Builtins();
  ~Builtins() = default;

  static void initKeywordList();

  AssignmentList assignments;
  std::unordered_map<std::string, BuiltinFunction *> functions;
  std::unordered_map<std::string, AbstractModule *> modules;

  std::unordered_map<std::string, std::string> deprecations;
};
