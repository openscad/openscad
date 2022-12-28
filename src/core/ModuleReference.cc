
/*
Copyright (C) Andy Little (kwikius@yahoo.com) 10/10/2022  initial revision
Copyright (C) Andy Little (kwikius@yahoo.com) 12/12/2022  fix https://github.com/kwikius/openscad/issues/2
https://github.com/openscad/openscad/blob/master/COPYING
*/

#include "printutils.h"
#include "ModuleReference.h"
#include "Value.h"
#include "Context.h"
#include "Expression.h"
#include <set>

int64_t ModuleReference::next_id = 0;

Value ModuleReference::operator==(const ModuleReference& other) const {
  return this->getUniqueID() == other.getUniqueID();
}
Value ModuleReference::operator!=(const ModuleReference& other) const {
  return this->getUniqueID() != other.getUniqueID();
}
Value ModuleReference::operator<(const ModuleReference& other) const {
  return this->getUniqueID() < other.getUniqueID();
}
Value ModuleReference::operator>(const ModuleReference& other) const {
  return this->getUniqueID() > other.getUniqueID();
}
Value ModuleReference::operator<=(const ModuleReference& other) const {
 return this->getUniqueID() <= other.getUniqueID();
}
Value ModuleReference::operator>=(const ModuleReference& other) const {
  return this->getUniqueID() >= other.getUniqueID();
}

namespace {

   int getParamIndex(
      AssignmentList const & params,
      std::string const & name
   ){
      auto numParams = params.size();
      for ( auto i =0U; i < numParams; ++i){
         if ( params[i]->getName() == name){
            return static_cast<int>(i);
         }
      }
      return -1;
   }
}

bool ModuleReference::transformToInstantiationArgs(
      AssignmentList const & evalContextArgs,
      const Location& loc,
      const std::shared_ptr<const Context> evalContext,
      AssignmentList & argsOut
) const
{
    // module alias syntax just another name for the module
    // e.g m = module cube
    // forward the arguments to the module
    if ( this->module_literal_parameters->empty() && this->module_args->empty()){
       argsOut.assign(evalContextArgs.begin(),evalContextArgs.end());
       return true;
    }

    // simple syntax - instantiated with no arguments
    // following are equivalent
    // e.g  m = module cube([10,20,30]);
    //      m = module () cube([10,20,30]);

    // the following are also valid. They have no module_params
    //      m = module { cube([10,20,30]); }
    //      m = module () { cube([10,20,30]); }
    if ( this->module_literal_parameters->empty() && evalContextArgs.empty()){
       argsOut.assign(this->module_args->begin(),this->module_args->end());
       return true;
    }

    // arguments forwarding syntax
    // anonymous module syntax
    // m = module (module_literal_params) {...}
    if ( ! this->module_literal_parameters->empty() ){
      auto const & paramsIn = *(this->module_literal_parameters);
      auto const numParams = paramsIn.size();
      // init assignments
      for ( auto i = 0; i < numParams; ++i){
          auto new_arg = new Assignment("",loc);
          argsOut.push_back(std::shared_ptr<Assignment>(new_arg));
      };

      auto const numInArgs = evalContextArgs.size();
      std::set<std::string> named_args;

      auto positionalArgsPos = 0U;
      for ( auto i = 0U; i < numInArgs; ++i){
         //process args
         if ( evalContextArgs.at(i)->hasName()){
            // process named args
            auto const & name = evalContextArgs.at(i)->getName();
            if (named_args.find(name) != named_args.end()){
               LOG(message_group::Warning, loc, evalContext->documentRoot(),
                    "WARNING: arg '%1$s' supplied more than once", name);
                     return false;
            }else{
               int const idx = getParamIndex(paramsIn,name);
               if ( idx >= 0 ){
                  auto & arg = argsOut.at(idx);
                  if ( arg->hasExpr()){
                    LOG(message_group::Warning, loc, evalContext->documentRoot(),
                    "WARNING: arg '%1$s' overriding value", name);
                  }
                  arg->setExpr(std::shared_ptr<Expression>(evalContextArgs[i]->getExpr()));
                  named_args.insert(name);
               }else{
                 LOG(message_group::Warning, loc, evalContext->documentRoot(),
                    "param '%1$s' not found\n", name);
                  return false;
               }
            }
         }else{
            // process positional args
            while( positionalArgsPos < numParams  ){
                auto const param_name = paramsIn.at(positionalArgsPos)->getName();
                if (named_args.find(param_name) == named_args.end()){
                   argsOut.at(positionalArgsPos)->setExpr(
                      std::shared_ptr<Expression>(evalContextArgs.at(i)->getExpr()));
                   named_args.insert(param_name);
                   break;
                }else{
                   ++positionalArgsPos;
                }
             }
             if ( positionalArgsPos >= numParams  ){
                LOG(message_group::Warning, loc, evalContext->documentRoot(),
                    "Too many unnamed arguments supplied");
                return false;
             }
          }
       }
       // process params default args
       bool argsOk = true;
       for ( auto i = 0U; i < numParams; ++i){
         if (! argsOut.at(i)->hasExpr() ){
            auto const & paramIn = paramsIn.at(i);
            if ( paramIn->hasExpr()){
                argsOut.at(i)->setExpr(std::shared_ptr<Expression>(paramIn->getExpr()));
            }else{
                LOG(message_group::Warning, loc, evalContext->documentRoot(),
                    "arg '%1$s' needs to be specified",paramIn->getName());
               argsOk=false;
            }
         }
      }
      return argsOk;
    }
    LOG(message_group::Warning, loc, evalContext->documentRoot(),"Invalid Arguments format");
    return false;
}

const ModuleReference& Value::toModuleReference() const
{
   return *std::get<ModuleReferencePtr>(this->value);
}

std::ostream& operator<<(std::ostream& stream, const ModuleReference& m)
{
   stream << "module";
   auto const & params = m.getModuleLiteralParameters();
   if ( ! params->empty()){
      stream << "(";
      bool first = true;
      for (const auto& par : *params) {
         if  (! first){
            stream << ", ";
         }else{
            first = false;
         }
         stream << par->getName();
         auto const expr = par->getExpr();
         if (expr) {
           stream << " = " << *expr;
         }
      }
      stream << ")";
   }
   stream << " " << m.getModuleName() ;
   auto const & args = m.getModuleArgs();
   if ( ! args->empty() ){
      stream << "(" << *args << ")" ;
   }
   return stream;
}
