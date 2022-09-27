#pragma once

#include "Expression.h"

std::string generateAnonymousModuleName();
void pushAnonymousModuleName(std::string const & name);
std::string popAnonymousModuleName();

Expression* MakeModuleLiteral(
   const std::string& moduleName,  AssignmentList const & params,
   const AssignmentList& args, const Location& loc );



