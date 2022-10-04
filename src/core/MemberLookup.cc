
#include <typeinfo>
#include <boost/regex.hpp>
#include "MemberLookup.h"
#include "ModuleReference.h"
#include "Value.h"
#include "Context.h"
#include "UserModule.h"
#include "Children.h"
#include "Arguments.h"
#include "ScopeContext.h"

MemberLookup::MemberLookup(Expression *expr, const std::string& member, const Location& loc)
  : Expression(loc), expr(expr), member(member) {}

Value MemberLookup::evaluate(const std::shared_ptr<const Context>& context) const
{
  const Value& v = this->expr->evaluate(context);
  static const boost::regex re_swizzle_validation("^([xyzw]{1,4}|[rgba]{1,4})$");

  switch (v.type()) {
  case Value::Type::VECTOR:
    if (this->member.length() > 1 && boost::regex_match(this->member, re_swizzle_validation)) {
      VectorType ret(context->session());
      for (const char& ch : this->member)
        switch (ch) {
        case 'r': case 'x': ret.emplace_back(v[0]); break;
        case 'g': case 'y': ret.emplace_back(v[1]); break;
        case 'b': case 'z': ret.emplace_back(v[2]); break;
        case 'a': case 'w': ret.emplace_back(v[3]); break;
        }
      return Value(std::move(ret));
    }
    if (this->member == "x") return v[0];
    if (this->member == "y") return v[1];
    if (this->member == "z") return v[2];
    if (this->member == "w") return v[3];
    if (this->member == "r") return v[0];
    if (this->member == "g") return v[1];
    if (this->member == "b") return v[2];
    if (this->member == "a") return v[3];
    break;
  case Value::Type::RANGE:
    if (this->member == "begin") return v[0];
    if (this->member == "step") return v[1];
    if (this->member == "end") return v[2];
    break;
  case Value::Type::OBJECT:
    return v[this->member];
  case Value::Type::MODULE:{
     auto const & modRef = v.toModuleReference();
     boost::optional<InstantiableModule> iModule = context->lookup_module(modRef.getModuleName(), this->loc);
     if (iModule) {
        auto user_module = dynamic_cast<const UserModule*>(iModule->module);
        // iModule->module  :  AbstractModule *  ==  UserModule * | BuiltinModule *
        if ( user_module){
            StaticModuleNameStack name{modRef.getModuleName()}; // push on static stack, pop at end of method!
            ContextHandle<UserModuleContext> module_context{
               Context::create<UserModuleContext>(
                  iModule->defining_context,
                  user_module,
                  this->location(),
                  Arguments(*modRef.getModuleArgs(), modRef.getContext()),
                  Children(&user_module->body, modRef.getContext())
               )
            };

            auto maybe_value = module_context->lookup_local_variable( this->member);
            if (maybe_value){
               return std::move(maybe_value->clone());
            }
        } else{
           //TODO
          // auto builin_module = dynamic_cast<const BuiltinModule*>(iModule->module)
        }
     }
  }

  default:
    break;
  }
  return Value::undefined.clone();
}

void MemberLookup::print(std::ostream& stream, const std::string&) const
{
  stream << *this->expr << "." << this->member;
}
