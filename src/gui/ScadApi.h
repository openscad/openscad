#pragma once

#include <QList>
#include <utility>
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
  ApiFunc(QString name, QString param) : name(std::move(name)), params{std::move(param)} { }
  ApiFunc(QString name, QString param1, QString param2) : name(std::move(name)), params{std::move(param1), std::move(param2)} { }
  ApiFunc(QString name, QStringList params) : name(std::move(name)), params(std::move(params)) { }

  const QString& get_name() const
  {
    return name;
  }

  const QStringList& get_params() const
  {
    return params;
  }

  ApiFunc& operator=(const ApiFunc& other)
  {
    if (this != &other) {
      this->name = other.name;
      this->params = other.params;
    }
    return *this;
  }
};

class ScadTemplate
{
private:
  QString text;
  int cursor_offset;
public:

  ScadTemplate() : text(""), cursor_offset(0) { }
  ScadTemplate(QString text, int cursor_offset) : text(std::move(text)), cursor_offset(cursor_offset) { }
  const QString& get_text() const { return text; }
  int get_cursor_offset() const { return cursor_offset; }

  ScadTemplate& operator=(const ScadTemplate& other)
  {
    if (this != &other) {
      this->text = other.text;
      this->cursor_offset = other.cursor_offset;
    }
    return *this;
  }
};

class ScintillaEditor;

class ScadApi : public QsciAbstractAPIs
{
  Q_OBJECT

private:
  ScintillaEditor *editor;
  QList<ApiFunc> funcs;

protected:
  void autoCompleteFolder(const QStringList& context, const QString& text, const int col, QStringList& list);
  void autoCompleteFunctions(const QStringList& context, QStringList& list);

public:
  ScadApi(ScintillaEditor *editor, QsciLexer *lexer);

  void updateAutoCompletionList(const QStringList& context, QStringList& list) override;
  void autoCompletionSelected(const QString& selection) override;
  QStringList callTips(const QStringList& context, int commas, QsciScintilla::CallTipsStyle style, QList<int>& shifts) override;
};
