#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <QVector>
#include <QString>

class Expression
{
public:
	QVector<Expression*> children;

	class Value *const_value;
	QString var_name;

	QString call_funcname;
	QVector<QString> call_argnames;

	// Boolean: ! && ||
	// Operators: * / % + ++ -
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
	QString type;

	Expression();
	~Expression();

	Value evaluate(const class Context *context) const;
	QString dump() const;
};

#endif
