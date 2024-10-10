#pragma once

#include <memory>
#include <functional>
#include "Feature.h"

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
  virtual ~AbstractModule() = default;
  [[nodiscard]] virtual bool is_experimental() const { return feature != nullptr; }
  [[nodiscard]] virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
  virtual std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const = 0;
};

class BuiltinModule : public AbstractModule
{
public:
  BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const ModuleInstantiation *, const std::shared_ptr<const Context>&), const Feature *feature = nullptr);
  BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const ModuleInstantiation *, Arguments, const Children&), const Feature *feature = nullptr);
  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const override;

private:
  std::function<std::shared_ptr<AbstractNode>(const ModuleInstantiation *, const std::shared_ptr<const Context>&)> do_instantiate;
};

struct InstantiableModule
{
  std::shared_ptr<const Context> defining_context;
  const AbstractModule *module;
};
