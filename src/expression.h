#pragma once

#include <string>
#include <vector>
#include "value.h"
#include "typedefs.h"

#define EXPRESSION_TYPE_NOT ('!')
#define EXPRESSION_TYPE_LOGICAL_AND ('&')
#define EXPRESSION_TYPE_LOGICAL_OR ('|')
#define EXPRESSION_TYPE_MULTIPLY ('*')
#define EXPRESSION_TYPE_DIVISION ('/')
#define EXPRESSION_TYPE_MODULO ('%')
#define EXPRESSION_TYPE_PLUS ('+')
#define EXPRESSION_TYPE_MINUS ('-')
#define EXPRESSION_TYPE_LESS ('<')
#define EXPRESSION_TYPE_LESS_OR_EQUAL ('0')
#define EXPRESSION_TYPE_EQUAL ('=')
#define EXPRESSION_TYPE_NOT_EQUAL ('1')
#define EXPRESSION_TYPE_GREATER_OR_EQUAL ('2')
#define EXPRESSION_TYPE_GREATER ('>')
#define EXPRESSION_TYPE_TERNARY ('?')
#define EXPRESSION_TYPE_ARRAY_ACCESS ('[')
#define EXPRESSION_TYPE_INVERT ('I')
#define EXPRESSION_TYPE_CONST ('C')
#define EXPRESSION_TYPE_RANGE ('R')
#define EXPRESSION_TYPE_VECTOR ('V')
#define EXPRESSION_TYPE_LOOKUP ('L')
#define EXPRESSION_TYPE_MEMBER ('N')
#define EXPRESSION_TYPE_FUNCTION ('F')
#define EXPRESSION_TYPE_LET ('l')
#define EXPRESSION_TYPE_LC_EXPRESSION ('i')
#define EXPRESSION_TYPE_LC ('c')

class ExpressionEvaluator
{
public:
    virtual ValuePtr evaluate(const class Expression *expr, const class Context *context) const = 0;
};

class ExpressionEvaluatorInit
{
public:
    ExpressionEvaluatorInit();
};

class Expression
{
public:
	std::vector<Expression*> children;

	ValuePtr const_value;
	std::string var_name;

	std::string call_funcname;
	AssignmentList call_arguments;

	Expression(const ValuePtr &val);
	Expression(const unsigned char type);
	Expression(const unsigned char type, Expression *left, Expression *right);
	Expression(const unsigned char type, Expression *expr);
	~Expression();

        void setType(const unsigned char type);
	ValuePtr evaluate(const class Context *context) const;

	std::string toString() const;

private:
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
        unsigned char type;
        ExpressionEvaluator *evaluator;

	mutable int recursioncount;
        
        static ExpressionEvaluator *evaluators[256];
        static ExpressionEvaluatorInit evaluatorInit;
        
        friend class ExpressionEvaluatorInit;
        friend class ExpressionEvaluatorLc; // for type
        friend class ExpressionEvaluatorFunction; // for recursioncount
};

std::ostream &operator<<(std::ostream &stream, const Expression &expr);
