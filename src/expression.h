#pragma once

#include <string>
#include <vector>
#include "value.h"
#include "typedefs.h"

class Expression
{
public:
	std::vector<Expression*> children;

	ValuePtr const_value;
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
	// Let expression: l
  // List comprehension expression: i
  // List comprehension: c
  std::string type;

	Expression();
	Expression(const ValuePtr &val);
	Expression(const std::string &type, Expression *left, Expression *right);
	Expression(const std::string &type, Expression *expr);
	~Expression();

	ValuePtr evaluate(const class Context *context) const;

	std::string toString() const;

private:
	mutable int recursioncount;

	// The following sub_* methods are needed to minimize stack usage only.
	ValuePtr sub_evaluate_function(const class Context *context) const;
	ValuePtr sub_evaluate_member(const class Context *context) const;
	ValuePtr sub_evaluate_range(const class Context *context) const;
	ValuePtr sub_evaluate_vector(const class Context *context) const;
	ValuePtr sub_evaluate_let_expression(const class Context *context) const;
	ValuePtr sub_evaluate_list_comprehension(const class Context *context) const;
};

std::ostream &operator<<(std::ostream &stream, const Expression &expr);
