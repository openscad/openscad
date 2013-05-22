#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <string>
#include <vector>
#include "value.h"
#include "typedefs.h"

class Expression
{
public:
	std::vector<Expression*> children;

	const Value const_value;
	std::string var_name;

	std::string call_funcname;
	AssignmentList call_arguments;

	// Boolean: ! && ||
	// Operators: * / % + -
	// Relations: < <= == != >= >
	// Vector element: []
	// Condition operator: ?:
	// Invert (prefix '-'): I
	// Constant value: C
	// Create Range: R
	// Create Vector: V
	// Create Matrix: M
	// Lookup Variable: L
	// Lookup member per name: N
	// Function call: F
  std::string type;

	Expression();
	Expression(const Value &val);
	Expression(const std::string &type, Expression *left, Expression *right);
	Expression(const std::string &type, Expression *expr);
	~Expression();

	Value evaluate(const class Context *context) const;
	std::string toString() const;

	mutable int recursioncount;
};

std::ostream &operator<<(std::ostream &stream, const Expression &expr);

#endif
