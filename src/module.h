#pragma once

#include <functional>
#include <string>
#include "AST.h"
#include "feature.h"

class AbstractNode;
class Arguments;
class Children;
class Context;
class ModuleInstantiation;

class AbstractModule
{
private:
  const Feature *feature;
public:
  AbstractModule() : feature(nullptr) {}
  AbstractModule(const Feature& feature) : feature(&feature) {}
  AbstractModule(const Feature *feature) : feature(feature) {}
  virtual ~AbstractModule() {}
  virtual bool is_experimental() const { return feature != nullptr; }
  virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
  virtual std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const = 0;
};

class BuiltinModule : public AbstractModule
{
public:
  BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const class ModuleInstantiation *, const std::shared_ptr<const Context>&), const Feature *feature = nullptr);
  BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const class ModuleInstantiation *, Arguments, Children), const Feature *feature = nullptr);
  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const override;

private:
  std::function<std::shared_ptr<AbstractNode> (const class ModuleInstantiation *, const std::shared_ptr<const class Context>&)> do_instantiate;
};

struct InstantiableModule
{
  std::shared_ptr<const Context> defining_context;
  const AbstractModule *module;
};
