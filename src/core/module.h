#pragma once

#include <functional>
#include <string>
#include "AST.h"
#include "Feature.h"

class AbstractNode;
class Arguments;
class Children;
class Context;
class ModuleInstantiation;

class AbstractModule
{
public:

   using abstractNodePtr = std::shared_ptr<AbstractNode>;
   using contextPtr = std::shared_ptr<const Context>;
   using ModInst = ModuleInstantiation;

   AbstractModule() : feature(nullptr) {}
   AbstractModule(const Feature& feature) : feature(&feature) {}
   AbstractModule(const Feature *feature) : feature(feature) {}
   virtual ~AbstractModule() {}

   virtual bool is_experimental() const
   {
      return feature != nullptr;
   }
   virtual bool is_enabled() const
   {
      return (feature == nullptr) || feature->is_enabled();
   }
   virtual abstractNodePtr instantiate(
      contextPtr const & defining_context,
      ModInst const *inst,
      contextPtr const & context
   ) const = 0;
private:
   const Feature *feature;
};

class BuiltinModule : public AbstractModule
{
public:

   using fnContextInstantiate =
      abstractNodePtr(*)(ModInst const *, contextPtr const &);
   using fnArgsChildrenInstantiate =
      abstractNodePtr (*) (ModInst const *, Arguments, Children);

   BuiltinModule(fnContextInstantiate,Feature const *feature = nullptr);
   BuiltinModule(fnArgsChildrenInstantiate,Feature const *feature = nullptr);
   abstractNodePtr instantiate(
      contextPtr const & defining_context,
      ModInst const *inst,
      contextPtr const & context
   ) const override;

private:
  std::function<
    abstractNodePtr (ModInst const *, contextPtr const &)
  > do_instantiate;
};

struct InstantiableModule
{
  using contextPtr = std::shared_ptr<const Context>;
  contextPtr defining_context;
  AbstractModule const * module;
};
