#pragma once

#include <string>
#include <boost/optional.hpp>

#include "core/callables.h"
#include "core/Context.h"

class Location;

class BuiltinContext : public Context
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
  BuiltinContext(EvaluationSession *session);

  friend class Context;
};
