#include <QObject>
#include <QString>
#include <QStringList>

#include <Qsci/qsciapis.h>

class ApiFunc
{
private:
	QString name;
	QStringList params;
	
public:
	ApiFunc(const QString name, const QString param) : name(name), params(QStringList(param)) { }
	ApiFunc(const QString name, const QString param1, const QString param2) : name(name), params(QStringList(param1) << param2) { }
	ApiFunc(const QString name, const QStringList params) : name(name), params(params) { }
	virtual ~ApiFunc() { }
	
	const QString& get_name() const { return name; }
	const QStringList& get_params() const { return params; }
	
	ApiFunc & operator=(const ApiFunc &other) {
	        if (this != &other) {
			this->name = other.name;
			this->params = other.params;
		}
	        return *this;
        }
};

class ScadApi : public QsciAbstractAPIs
{
	Q_OBJECT

private:
	QsciScintilla *qsci;
	QList<ApiFunc> funcs;
	
public:
	ScadApi(QsciScintilla *qsci, QsciLexer *lexer);
	virtual ~ScadApi();
	
	void updateAutoCompletionList (const QStringList &context, QStringList &list);
	void autoCompletionSelected (const QString &selection);
	QStringList callTips (const QStringList &context, int commas, QsciScintilla::CallTipsStyle style, QList< int > &shifts);
};
