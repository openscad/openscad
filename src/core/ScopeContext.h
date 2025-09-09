#pragma once

#include <memory>
#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "core/Arguments.h"
#include "core/AST.h"
#include "core/callables.h"
#include "core/Children.h"
#include "core/Context.h"
#include "core/function.h"
#include "core/module.h"
#include "core/SourceFile.h"
#include "core/Using.h"

class UserModule;

class ScopeContext : public Context
{
public:
  void init() override;
  boost::optional<CallableFunction> lookup_local_function(const std::string& name,
                                                          const Location& loc) const override;
  boost::optional<InstantiableModule> lookup_local_module(const std::string& name,
                                                          const Location& loc) const override;

  boost::optional<CallableFunction> lookup_function_as_namespace(const std::string& name) const override;
  boost::optional<InstantiableModule> lookup_module_as_namespace(const std::string& name) const override;

protected:
  ScopeContext(const std::shared_ptr<const Context>& parent, std::shared_ptr<const LocalScope> scope)
    : Context(parent), scope(std::move(scope))
  {
  }

private:
  const std::shared_ptr<const LocalScope> scope;

  friend class Context;
};

class UserModuleContext : public ScopeContext
{
public:
  const Children *user_module_children() const override { return &children; }
  std::vector<const std::shared_ptr<const Context> *> list_referenced_contexts() const override;

protected:
  UserModuleContext(const std::shared_ptr<const Context>& parent, const UserModule *module,
                    const Location& loc, Arguments arguments, Children children);

private:
  Children children;

  friend class Context;
};

class FileContext : public ScopeContext
{
public:
  boost::optional<CallableFunction> lookup_local_function(const std::string& name,
                                                          const Location& loc) const override;
  boost::optional<InstantiableModule> lookup_local_module(const std::string& name,
                                                          const Location& loc) const override;

protected:
  FileContext(const std::shared_ptr<const Context>& parent,
              std::shared_ptr<const SourceFile> source_file)
    : ScopeContext(parent, source_file->scope), source_file(source_file)
  {
  }

private:
  const std::shared_ptr<const SourceFile> source_file;
  boost::optional<CallableFunction> lookup_function_from_uses(const std::string& name) const;
  boost::optional<InstantiableModule> lookup_module_from_uses(const std::string& name) const;

  friend class Context;
};
