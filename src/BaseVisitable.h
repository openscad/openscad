#pragma once

#include "visitor.h"

class BaseVisitable
{
public:
	virtual ~BaseVisitable() {}
	virtual Response accept(class State&, Visitor&) const = 0;
};

#define VISITABLE() \
  virtual Response accept(class State &state, Visitor &visitor) const { \
		return visitor.visit(state, *this); \
	}
