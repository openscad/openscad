
#include <cstdio>
#include <stack>
#include "ModuleLiteral.h"
#include "LocalScope.h"
#include "UserModule.h"
#include "Value.h"
#include "Feature.h"

extern std::stack<LocalScope *> scope_stack;

int64_t ModuleReference::next_id = 0;

Expression* MakeModuleLiteral(
   const std::string& moduleName,
   const AssignmentList& parameters,
   const AssignmentList& arguments,
   const Location& loc
)
{
   if (Feature::ExperimentalModuleLiteral.is_enabled()){
      return new ModuleLiteral(moduleName,parameters,arguments,loc);
   } else {
       LOG(message_group::Warning, loc,"", "Experimental module-literal is not enabled");
       return new Literal(boost::none,loc);
   }
}

namespace {
  int32_t anonymousModuleNameCount = 0U;
  char itoaArr[50] = {0};

  std::stack<std::string> anonModuleStack;
}
std::string generateAnonymousModuleName()
{
   sprintf(itoaArr,"__&ML[%d]__",anonymousModuleNameCount++);
   return std::string(itoaArr);
}

void pushAnonymousModuleName(std::string const & name)
{
   anonModuleStack.push(name);
}

std::string popAnonymousModuleName()
{
   std::string name = anonModuleStack.top();
   anonModuleStack.pop();
   return name;
}


