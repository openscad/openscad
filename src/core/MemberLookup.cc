
/*
Copyright (C) Andy Little (kwikius@yahoo.com) 10/10/2022  initial revision module_reference member lookup
https://github.com/openscad/openscad/blob/master/COPYING
*/
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
  case Value::Type::MODULE:
    LOG(message_group::Warning, loc, context->documentRoot(),"Member access not available for module reference");
    break;
  default:
    break;
  }
  return Value::undefined.clone();
}

void MemberLookup::print(std::ostream& stream, const std::string&) const
{
  stream << *this->expr << "." << this->member;
}
