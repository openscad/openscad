#pragma once

#include <string>
#include <memory>
#include "Assignment.h"
#include "Expression.h"
#include "Context.h"
#include "AST.h"

class Value;

class ModuleLiteral : public Expression
{
public:

  ModuleLiteral(const std::string& name, const AssignmentList & literal_params,
               const AssignmentList& module_args, const Location& loc );
  Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  // the name of the module  we are referring to
  std::string const module_name;
  AssignmentList module_literal_parameters;
  AssignmentList module_arguments;
};

std::string generateAnonymousModuleName();
void pushAnonymousModuleName(std::string const & name);
std::string popAnonymousModuleName();

Expression* MakeModuleLiteral(
   const std::string& moduleName,  AssignmentList const & params,
   const AssignmentList& args, const Location& loc );



