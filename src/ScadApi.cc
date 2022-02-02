#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include "ScadApi.h"
#include "builtin.h"
#include "ScintillaEditor.h"
#include "parsersettings.h"

namespace {

bool isInString(const std::u32string& text, const int col)
{
	//first see if we are in a string literal. if so, don't allow auto complete
	bool lastWasEscape = false;
	bool inString = false;
	int dx = 0;
	int count = col;
	while (count-- > 0) {
		const char32_t ch = text.at(dx++);
		if (ch == '\\')
			lastWasEscape = true; //next character will be literal handle \"
		else if (lastWasEscape)
			lastWasEscape = false;
		else if (ch == '"') //string toggle
			inString = !inString;
	}
	return inString;
}

bool isUseOrInclude(const QString& text, const int col)
{
	const QRegularExpression re("\\s*(use|include)\\s*<[^>]*$");
	const QRegularExpressionMatch match = re.match(text.left(col));
	return match.hasMatch();
}

template <typename C>
QStringList getSorted(const QFileInfoList& list, C cond) {
	QStringList result;
	for (const auto& info : list) {
		if (cond(info)) {
			result << info.fileName();
		}
	}
	result.sort();
	return result;
}

}

ScadApi::ScadApi(ScintillaEditor *editor, QsciLexer *lexer) : QsciAbstractAPIs(lexer), editor(editor)
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
	int line, col;
	editor->qsci->getCursorPosition(&line, &col);
	const auto& text = editor->qsci->text(line);

	if (isInString(text.toStdU32String(), col)) {
		return;
	} else if (isUseOrInclude(text, col)) {
		autoCompleteFolder(context, text, col, list);
	} else {
		autoCompleteFunctions(context, list);
	}
}

void ScadApi::autoCompleteFolder(const QStringList &context, const QString& text, const int col, QStringList &list)
{
	const QRegularExpression re("\\s*(use|include)\\s*<\\s*");
	const auto useDir = QFileInfo{text.left(col).replace(re, "")}.dir().path();

	QFileInfoList dirs;
	dirs << QFileInfo(editor->filepath);
	for (const auto &path : get_library_path()) {
		dirs << QFileInfo(QString::fromStdString(path) + "/");
	}

	for (const auto& info : dirs) {
		const auto dir = QDir{info.dir().filePath(useDir)};
		if (!dir.exists()) {
			continue;
		}

		QFileInfoList result;
		const auto& prefix = context.last();
		const auto& infoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
		for (const auto& info : infoList) {
			if (info.fileName().startsWith(prefix) && (info.isDir() || info.suffix().toLower() == "scad")) {
				result << info;
			}
		}

		list << getSorted(result, [](QFileInfo i){ return i.isDir(); });
		list << getSorted(result, [](QFileInfo i){ return i.isFile(); });
		list.removeDuplicates();
	}
}

void ScadApi::autoCompleteFunctions(const QStringList &context, QStringList &list)
{
	const QString c = context.last();
	// for now we only auto-complete functions and modules
	if (c.isEmpty()) {
		return;
	}

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

void ScadApi::autoCompletionSelected(const QString & /*selection*/)
{
}

QStringList ScadApi::callTips(const QStringList &context, int /*commas*/, QsciScintilla::CallTipsStyle /*style*/, QList< int > & /*shifts*/)
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
