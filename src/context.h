#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <QHash>
#include <QString>
#include "value.h"

class Context
{
public:
	const Context *parent;
	QHash<QString, Value> variables;
	QHash<QString, Value> config_variables;
	const QHash<QString, class AbstractFunction*> *functions_p;
	const QHash<QString, class AbstractModule*> *modules_p;
	const class ModuleInstantiation *inst_p;
	QString document_path;

	static QVector<const Context*> ctx_stack;

	Context(const Context *parent = NULL);
	~Context();

	void args(const QVector<QString> &argnames, const QVector<class Expression*> &argexpr, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);

	void set_variable(QString name, Value value);
	Value lookup_variable(QString name, bool silent = false) const;

	QString get_absolute_path(const QString &filename) const;

	Value evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues) const;
	class AbstractNode *evaluate_module(const ModuleInstantiation *inst) const;
};

#endif
