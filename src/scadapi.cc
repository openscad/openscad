#include "scadapi.h"
#include "builtin.h"

ScadApi::ScadApi(QsciScintilla *qsci, QsciLexer *lexer) : QsciAbstractAPIs(lexer), qsci(qsci)
{
	for (auto iter = Builtins::keywordList.cbegin(); iter != Builtins::keywordList.cend(); ++iter)
	{
		QStringList calltipList;
		for(auto it = iter->second.cbegin(); it != iter->second.cend(); ++it)
			calltipList.append(QString::fromStdString(*it));

		funcs.append(ApiFunc(QString::fromStdString(iter->first), calltipList));
	}
}

ScadApi::~ScadApi()
{
}

void ScadApi::updateAutoCompletionList(const QStringList &context, QStringList &list)
{
    //first see if we are in a string literal. if so, don't allow auto comlete
    bool lastWasEscape=false;
    bool inString=false;
    int line, col;
    qsci->getCursorPosition(&line, &col);
    auto sLine = qsci->text(line);  //get the current line of text
    QByteArray ba = sLine.toLocal8Bit();
    const char *c_str = ba.data();
    const char *p = c_str;
    while (col-- > 0) {
        if (*p == '\\')
            lastWasEscape = true;   //next character will be literal handle \"
        else if (lastWasEscape)
            lastWasEscape = false;
        else if (*p == '"')        //string toggle
            inString = !inString;
        p++;
    }
    if (inString) return;  //we are in string literal, don't return autocomplete list
    //not in string literal, proceed as normal
	const QString c = context.last();
	for (int a = 0; a < funcs.size(); ++a) {
		const ApiFunc &func = funcs.at(a);
		const QString &name = func.get_name();
		if (name.startsWith(c)) {
			if (!list.contains(name)) {
				list << name;
			}
		}
	}
}

void ScadApi::autoCompletionSelected (const QString & /*selection*/)
{
}

QStringList ScadApi::callTips (const QStringList &context, int /*commas*/, QsciScintilla::CallTipsStyle /*style*/, QList< int > & /*shifts*/)
{
	QStringList callTips;
	for (int a = 0; a < funcs.size(); ++a) {
		if (funcs.at(a).get_name() == context.at(context.size() - 2)) {
			callTips = funcs.at(a).get_params();
			break;
		}
	}
	return callTips;
}
