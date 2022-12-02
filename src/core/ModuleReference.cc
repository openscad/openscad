
/*
Copyright (C) Andy Little (kwikius@yahoo.com) 10/10/2022  initial revision
https://github.com/openscad/openscad/blob/master/COPYING
*/

#include "printutils.h"
#include "ModuleReference.h"
#include "Value.h"
#include "Context.h"
#include "Expression.h"

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

// the following are also valid. They have no module_args
    //      m = module { cube([10,20,30]); }
    //      m = module () { cube([10,20,30]); }
    if ( this->module_literal_parameters->empty() && evalContextArgs.empty()){
       argsOut.assign(this->module_args->begin(),this->module_args->end());
       return true;
    }

    // arguments forwarding syntax
    // anonymous module syntax
    // m = module (module_literal_params) {...}
    // TODO evalContextArgs may have named args
    if ( ! this->module_literal_parameters->empty() ){

       auto const nParams = this->module_literal_parameters->size();
       auto const nArgs = evalContextArgs.size();
       auto const nArgsToProcess = std::min(nArgs,nParams);
       if ( nArgs > nArgsToProcess){
          LOG(message_group::Warning, loc, evalContext->documentRoot(),
            "Ignoring Arguments greater than number of params");
       }

       // positional args
       for (auto i = 0; i < nArgsToProcess;++i){
          auto const & param = this->module_literal_parameters->at(i);
          auto const & arg = evalContextArgs.at(i);
          auto new_arg = new Assignment(param->getName(),loc);
          new_arg->setExpr(arg->getExpr());
          argsOut.push_back(std::shared_ptr<Assignment>(new_arg));
       }
       // default args from params
       // ToDo do these first from start to end of
       // todo put these in pos if param->hasExpr()
       for (auto i = nArgsToProcess; i < nParams; ++i ){
          auto const & param = this->module_literal_parameters->at(i);
          auto newArg = new Assignment(param->getName());
          auto expr = param->getExpr();
          if( !expr){
            LOG(message_group::Warning, loc, evalContext->documentRoot(),
               "Default arguments not supplied");
            return false;
          }
          // evaluate the expr in the moduleLiteral defining context
          newArg->setExpr(param->getExpr());
          argsOut.push_back(std::shared_ptr<Assignment>(newArg));
       }
       return true;
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
