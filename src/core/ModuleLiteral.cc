
#include <cstdio>
#include <stack>
#include <algorithm>
#include "ModuleLiteral.h"
#include "ModuleReference.h"
#include "Expression.h"
#include "LocalScope.h"
#include "UserModule.h"
#include "Value.h"
#include "Feature.h"
#include "ValueWrapper.h"

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

ModuleLiteral::ModuleLiteral(const std::string& mod_name, const AssignmentList &literal_params,
                         const AssignmentList& mod_args, const Location& loc )
: Expression(loc),module_name(mod_name),module_literal_parameters(literal_params),module_arguments(mod_args)
{};

Value ModuleLiteral::evaluate(const std::shared_ptr<const Context>& context) const
{
   AssignmentList const & params_in = this->module_literal_parameters;
   AssignmentList * params_out = new AssignmentList;

   bool in_default_args = false;
   for ( auto i = 0; i < params_in.size(); ++i){
       auto const & param = params_in[i];
       auto new_param = new Assignment(param->getName(),loc);
       if ( static_cast<bool>(param->getExpr()) != false ){
         new_param->setExpr(
           std::shared_ptr<ValueWrapper>(
               new ValueWrapper(
                  std::shared_ptr<Value>(
                     new Value(std::move(param->getExpr()->evaluate(context))))
              ,loc)
           )
         );
        in_default_args = true;
       }else{
         if ( in_default_args){
             LOG(message_group::Warning, loc, context->documentRoot(),"default args not continuous to last param");
         }
       }
       params_out->push_back(std::shared_ptr<Assignment>(new_param));
   }
   return ModuleReferencePtr(
      ModuleReference(
         context,
         std::unique_ptr<AssignmentList>(params_out),
         this->module_name,
         std::unique_ptr<AssignmentList>{new AssignmentList{this->module_arguments}}
      )
   );
}

void ModuleLiteral::print(std::ostream& stream, const std::string& indent) const
{
   stream << indent << "module ";
   if ( ! this->module_literal_parameters.empty()){
      stream << "(" << this->module_literal_parameters << ")";
   }
   stream << this->module_name ;
   if ( ! this->module_arguments.empty() ){
      stream << "(" << this->module_arguments << ")" ;
   }
}

