#pragma once

#include <cstdint>
//#include <limits>
#include <iostream>
#include <memory>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
//#ifndef Q_MOC_RUN
//#include <boost/variant.hpp>
//#include <boost/lexical_cast.hpp>
//#include <glib.h>
//#endif

#include "Assignment.h"
#include "ValuePtr.h"

class Context;
class Expression;
class Value;

class ModuleReference
{
public:
  ModuleReference(std::shared_ptr<const Context> context_in,
                  std::shared_ptr<AssignmentList> literal_params_in,
                  std::string const & module_name_in,
                  std::shared_ptr<AssignmentList> mod_args_in
                  )
    : context(context_in),
      module_literal_parameters(literal_params_in),
      module_name(module_name_in),
      module_args(mod_args_in),
      m_unique_id(generate_unique_id())
      { }

  Value operator==(const ModuleReference& other) const;
  Value operator!=(const ModuleReference& other) const;
  Value operator<(const ModuleReference& other) const;
  Value operator>(const ModuleReference& other) const;
  Value operator<=(const ModuleReference& other) const;
  Value operator>=(const ModuleReference& other) const;

  const std::shared_ptr<const Context>& getContext() const { return context; }
  const std::shared_ptr<AssignmentList>& getModuleLiteralParameters() const { return module_literal_parameters; }
  const std::string & getModuleName() const { return module_name; }
  const std::shared_ptr<AssignmentList>& getModuleArgs() const { return module_args; }
  bool transformToInstantiationArgs(
      AssignmentList const & evalContextArgs,
      const Location& loc,
      const std::shared_ptr<const Context> evalContext,
      AssignmentList & argsOut
  ) const ;
  int64_t getUniqueID() const { return m_unique_id;}
private:
  static int64_t generate_unique_id() { ++ next_id; return next_id;}
  static int64_t next_id;
  std::shared_ptr<const Context> context;
  std::shared_ptr<AssignmentList> module_literal_parameters;
  std::string module_name;
  std::shared_ptr<AssignmentList> module_args;
  int64_t const m_unique_id;
};

using ModuleReferencePtr = ValuePtr<ModuleReference>;

std::ostream& operator<<(std::ostream& stream, const ModuleReference& m);
