#pragma once

#include "Value.h"
#include "Expression.h"

class ValueWrapper : public Expression{
public:
  ValueWrapper( std::shared_ptr<Value> const & v, const Location& loc)
    :Expression(loc), value(v){}

  bool isLiteral() const override;
  Value evaluate(const std::shared_ptr<const Context>& context) const override { return std::move(value->clone());}
  void print(std::ostream&, const std::string&) const override;
private:
  std::shared_ptr<Value> value;
};


