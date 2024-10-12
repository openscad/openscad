#include "gui/ScadApi.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <string>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include "core/Builtins.h"
#include "gui/ScintillaEditor.h"
#include "core/parsersettings.h"

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
    if (ch == '\\') lastWasEscape = true; //next character will be literal handle \"
    else if (lastWasEscape) lastWasEscape = false;
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

} // namespace

ScadApi::ScadApi(ScintillaEditor *editor, QsciLexer *lexer) : QsciAbstractAPIs(lexer), editor(editor)
{
  for (const auto& iter : Builtins::keywordList) {
    QStringList calltipList;
    for (const auto& it : iter.second)
      calltipList.append(QString::fromStdString(it));

    funcs.append(ApiFunc(QString::fromStdString(iter.first), calltipList));
  }
}

void ScadApi::updateAutoCompletionList(const QStringList& context, QStringList& list)
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

void ScadApi::autoCompleteFolder(const QStringList& context, const QString& text, const int col, QStringList& list)
{
  const QRegularExpression re(R"(\s*(use|include)\s*<\s*)");
  const auto useDir = QFileInfo{text.left(col).replace(re, "")}.dir().path();

  QFileInfoList dirs;
  dirs << QFileInfo(editor->filepath);
  for (const auto& path : get_library_path()) {
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

    list << getSorted(result, [](const QFileInfo& i){
      return i.isDir();
    });
    list << getSorted(result, [](const QFileInfo& i){
      return i.isFile();
    });
    list.removeDuplicates();
  }
}

void ScadApi::autoCompleteFunctions(const QStringList& context, QStringList& list)
{
  const QString& c = context.last();
  // for now we only auto-complete functions and modules
  if (c.isEmpty()) {
    return;
  }

  for (const auto& func : funcs) {
    const QString& name = func.get_name();
    if (name.startsWith(c)) {
      if (!list.contains(name)) {
        list << name;
      }
    }
  }
}

void ScadApi::autoCompletionSelected(const QString& /*selection*/)
{
}

QStringList ScadApi::callTips(const QStringList& context, int /*commas*/, QsciScintilla::CallTipsStyle /*style*/, QList<int>& /*shifts*/)
{
  QStringList callTips;
  for (const auto& func : funcs) {
    if (func.get_name() == context.at(context.size() - 2)) {
      callTips = func.get_params();
      break;
    }
  }
  return callTips;
}
