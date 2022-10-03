

#include "ValueWrapper.h"

bool ValueWrapper::isLiteral() const
{
    switch(value->type()){
       case Value::Type::BOOL:
       case Value::Type::NUMBER:
       case Value::Type::STRING:
         return true;
       case Value::Type::VECTOR:
       case Value::Type::EMBEDDED_VECTOR:
       case Value::Type::RANGE:
       case Value::Type::FUNCTION:
       case Value::Type::OBJECT:
       case Value::Type::MODULE:
       case Value::Type::UNDEFINED:
       default:
         return false;
    }
}

void ValueWrapper::print(std::ostream& os, const std::string& indent) const
{
    os << indent << *this->value;
}



