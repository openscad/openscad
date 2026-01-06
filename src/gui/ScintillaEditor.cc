#include "gui/ScintillaEditor.h"

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QKeyCombination>
#endif
#include <QMenu>
#include <QObject>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <exception>
#include <memory>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <QString>
#include <QChar>
#include <QRegularExpression>
#include <QShortcut>
#include <Qsci/qscicommandset.h>

#include "gui/Preferences.h"
#include "platform/PlatformUtils.h"
#include "core/Settings.h"
#include "gui/ScadLexer.h"

#include <QWheelEvent>
#include <QPoint>
#include <QToolTip>

namespace fs = std::filesystem;

const QString ScintillaEditor::cursorPlaceHolder = "^~^";

// In setCursorPosition, how many lines should be visible above and below the cursor
const int setCursorPositionVisibleLines = 3;

class SettingsConverter
{
public:
  QsciScintilla::WrapMode toWrapMode(const std::string& val);
  QsciScintilla::WrapVisualFlag toLineWrapVisualization(const std::string& val);
  QsciScintilla::WrapIndentMode toLineWrapIndentationStyle(const std::string& val);
  QsciScintilla::WhitespaceVisibility toShowWhitespaces(const std::string& val);
};

QsciScintilla::WrapMode SettingsConverter::toWrapMode(const std::string& val)
{
  if (val == "Char") {
    return QsciScintilla::WrapCharacter;
  } else if (val == "Word") {
    return QsciScintilla::WrapWord;
  } else {
    return QsciScintilla::WrapNone;
  }
}

QsciScintilla::WrapVisualFlag SettingsConverter::toLineWrapVisualization(const std::string& val)
{
  if (val == "Text") {
    return QsciScintilla::WrapFlagByText;
  } else if (val == "Border") {
    return QsciScintilla::WrapFlagByBorder;
#if QSCINTILLA_VERSION >= 0x020700
  } else if (val == "Margin") {
    return QsciScintilla::WrapFlagInMargin;
#endif
  } else {
    return QsciScintilla::WrapFlagNone;
  }
}

QsciScintilla::WrapIndentMode SettingsConverter::toLineWrapIndentationStyle(const std::string& val)
{
  if (val == "Same") {
    return QsciScintilla::WrapIndentSame;
  } else if (val == "Indented") {
    return QsciScintilla::WrapIndentIndented;
  } else {
    return QsciScintilla::WrapIndentFixed;
  }
}

QsciScintilla::WhitespaceVisibility SettingsConverter::toShowWhitespaces(const std::string& val)
{
  if (val == "Always") {
    return QsciScintilla::WsVisible;
  } else if (val == "AfterIndentation") {
    return QsciScintilla::WsVisibleAfterIndent;
  } else {
    return QsciScintilla::WsInvisible;
  }
}

EditorColorScheme::EditorColorScheme(const fs::path& path) : path(path)
{
  try {
    boost::property_tree::read_json(path.generic_string(), pt);
    _name = QString::fromStdString(pt.get<std::string>("name"));
    _index = pt.get<int>("index");
  } catch (const std::exception& e) {
    LOG("Error reading color scheme file '%1$s': %2$s", path.generic_string(), e.what());
    _name = "";
    _index = 0;
  }
}

bool EditorColorScheme::valid() const { return !_name.isEmpty(); }

const QString& EditorColorScheme::name() const { return _name; }

int EditorColorScheme::index() const { return _index; }

const boost::property_tree::ptree& EditorColorScheme::propertyTree() const { return pt; }

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
  api = nullptr;
  lexer = nullptr;
  scintillaLayout = new QVBoxLayout(this);
  qsci = new QsciScintilla(this);

  contentsRendered = false;
  findState = 0;  // FIND_HIDDEN
  filepath = "";

  // Force EOL mode to Unix, since QTextStream will manage local EOL modes.
  qsci->setEolMode(QsciScintilla::EolUnix);

  //
  // Remapping some scintilla key binding which conflict with OpenSCAD global
  // key bindings, as well as some minor scintilla bugs
  //
  QsciCommand *c;
  // NOLINTBEGIN(bugprone-suspicious-enum-usage)
#ifdef Q_OS_MACOS
  // Alt-Backspace should delete left word (Alt-Delete already deletes right word)
  c = qsci->standardCommands()->find(QsciCommand::DeleteWordLeft);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  c->setKey((Qt::Key_Backspace | Qt::ALT).toCombined());
#else
  c->setKey(Qt::Key_Backspace | Qt::ALT);
#endif
#endif
  // Cmd/Ctrl-T is handled by the menu
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  c = qsci->standardCommands()->boundTo((Qt::Key_T | Qt::CTRL).toCombined());
#else
  c = qsci->standardCommands()->boundTo(Qt::Key_T | Qt::CTRL);
#endif
  c->setKey(0);
  // Cmd/Ctrl-D is handled by the menu
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  c = qsci->standardCommands()->boundTo((Qt::Key_D | Qt::CTRL).toCombined());
#else
  c = qsci->standardCommands()->boundTo(Qt::Key_D | Qt::CTRL);
#endif
  c->setKey(0);
  // Ctrl-Shift-Z should redo on all platforms
  c = qsci->standardCommands()->find(QsciCommand::Redo);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  c->setKey(QKeyCombination(Qt::CTRL | Qt::SHIFT, Qt::Key_Z).toCombined());
  c->setAlternateKey((Qt::Key_Y | Qt::CTRL).toCombined());
#else
  c->setKey(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);
  c->setAlternateKey(Qt::Key_Y | Qt::CTRL);
#endif

#ifdef Q_OS_MACOS
  const unsigned long modifier = Qt::META;
#else
  const unsigned long modifier = Qt::CTRL;
#endif

  QShortcut *shortcutCalltip;
  shortcutCalltip = new QShortcut(modifier | Qt::SHIFT | Qt::Key_Space, this);
  connect(shortcutCalltip, &QShortcut::activated, [=]() { qsci->callTip(); });

  QShortcut *shortcutAutocomplete;
  shortcutAutocomplete = new QShortcut(modifier | Qt::Key_Space, this);
  connect(shortcutAutocomplete, &QShortcut::activated, [=]() { qsci->autoCompleteFromAPIs(); });
  // NOLINTEND(bugprone-suspicious-enum-usage)

  scintillaLayout->setContentsMargins(0, 0, 0, 0);
  scintillaLayout->addWidget(qsci);

  qsci->setUtf8(true);
  qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
  qsci->setCaretLineVisible(true);

  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, errorIndicatorNumber);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, findIndicatorNumber);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, selectionIndicatorIsActiveNumber);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, selectionIndicatorIsActiveNumber + 1);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, selectionIndicatorIsImpactedNumber);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, selectionIndicatorIsImpactedNumber + 1);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, selectionIndicatorIsImpactedNumber + 2);

  qsci->markerDefine(QsciScintilla::Circle, errMarkerNumber);
  qsci->markerDefine(QsciScintilla::Bookmark, bmMarkerNumber);

  qsci->markerDefine('1', selectionMarkerLevelNumber);
  qsci->markerDefine('2', selectionMarkerLevelNumber + 1);
  qsci->markerDefine('3', selectionMarkerLevelNumber + 2);
  qsci->markerDefine('4', selectionMarkerLevelNumber + 3);
  qsci->markerDefine('5', selectionMarkerLevelNumber + 4);
  qsci->markerDefine('+', selectionMarkerLevelNumber + 5);

  qsci->setMarginType(numberMargin, QsciScintilla::NumberMargin);
  qsci->setMarginLineNumbers(numberMargin, true);
  qsci->setMarginMarkerMask(numberMargin, 0);

  qsci->setMarginType(symbolMargin, QsciScintilla::SymbolMargin);
  qsci->setMarginLineNumbers(symbolMargin, false);
  qsci->setMarginWidth(symbolMargin, 0);
  qsci->setMarginMarkerMask(
    symbolMargin, 1 << errMarkerNumber | 1 << bmMarkerNumber | 1 << selectionMarkerLevelNumber |
                    1 << (selectionMarkerLevelNumber + 1) | 1 << (selectionMarkerLevelNumber + 2) |
                    1 << (selectionMarkerLevelNumber + 3) | 1 << (selectionMarkerLevelNumber + 4) |
                    1 << (selectionMarkerLevelNumber + 5));

#if ENABLE_LEXERTL
  auto newLexer = new ScadLexer2(this);
  newLexer->finalizeLexer();
  setLexer(newLexer);
#else
  setLexer(new ScadLexer(this));
#endif

  initMargin();

  connect(qsci, &QsciScintilla::textChanged, this, &ScintillaEditor::contentsChanged);
  connect(qsci, &QsciScintilla::modificationChanged, this, &ScintillaEditor::fireModificationChanged);
  connect(qsci, &QsciScintilla::userListActivated, this, &ScintillaEditor::onUserListSelected);
  qsci->installEventFilter(this);
  qsci->viewport()->installEventFilter(this);

  qsci->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(qsci, &QsciScintilla::customContextMenuRequested, this,
          &ScintillaEditor::showContextMenuEvent);

  qsci->indicatorDefine(QsciScintilla::ThinCompositionIndicator, hyperlinkIndicatorNumber);
  qsci->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, hyperlinkIndicatorNumber,
                      QsciScintilla::INDIC_HIDDEN);
  connect(qsci, &QsciScintilla::indicatorClicked, this, &ScintillaEditor::onIndicatorClicked);
  connect(qsci, &QsciScintilla::indicatorReleased, this, &ScintillaEditor::onIndicatorReleased);

#if QSCINTILLA_VERSION >= 0x020b00
  connect(qsci, &QsciScintilla::SCN_URIDROPPED, this, &ScintillaEditor::uriDropped);
#endif
  connect(qsci, &QsciScintilla::SCN_FOCUSIN, this, &ScintillaEditor::focusIn);

  // Disabling buffered drawing resolves non-integer HiDPI scaling.
  qsci->SendScintilla(QsciScintillaBase::SCI_SETBUFFEREDDRAW, false);
}

QPoint ScintillaEditor::mapToGlobal(const QPoint& pos) { return qsci->mapToGlobal(pos); }

QMenu *ScintillaEditor::createStandardContextMenu() { return qsci->createStandardContextMenu(); }

void ScintillaEditor::addTemplate()
{
  addTemplate(PlatformUtils::resourceBasePath());
  addTemplate(PlatformUtils::userConfigPath());
  for (const auto& key : templateMap.keys()) {
    userList.append(key);
  }
}

void ScintillaEditor::addTemplate(const fs::path& path)
{
  const auto template_path = path / "templates";

  if (fs::exists(template_path) && fs::is_directory(template_path)) {
    for (const auto& dirEntry : boost::make_iterator_range(fs::directory_iterator{template_path}, {})) {
      if (!fs::is_regular_file(dirEntry.status())) continue;

      const auto& path = dirEntry.path();
      if (!(path.extension() == ".json")) continue;

      boost::property_tree::ptree pt;
      try {
        boost::property_tree::read_json(path.generic_string().c_str(), pt);
        const QString key = QString::fromStdString(pt.get<std::string>("key"));
        const QString content = QString::fromStdString(pt.get<std::string>("content"));
        const int cursor_offset = pt.get<int>("offset", -1);

        templateMap.insert(key, ScadTemplate(content, cursor_offset));
      } catch (const std::exception& e) {
        LOG("Error reading template file '%1$s': %2$s", path.generic_string(), e.what());
      }
    }
  }
}

void ScintillaEditor::displayTemplates() { qsci->showUserList(1, userList); }

void ScintillaEditor::foldUnfold() { qsci->foldAll(); }

/**
 * Apply the settings that are changeable in the preferences. This is also
 * called in the event handler from the preferences.
 */
void ScintillaEditor::applySettings()
{
  SettingsConverter conv;

  qsci->setIndentationWidth(Settings::Settings::indentationWidth.value());
  qsci->setTabWidth(Settings::Settings::tabWidth.value());
  qsci->setWrapMode(conv.toWrapMode(Settings::Settings::lineWrap.value()));
  qsci->setWrapIndentMode(
    conv.toLineWrapIndentationStyle(Settings::Settings::lineWrapIndentationStyle.value()));
  qsci->setWrapVisualFlags(
    conv.toLineWrapVisualization(Settings::Settings::lineWrapVisualizationEnd.value()),
    conv.toLineWrapVisualization(Settings::Settings::lineWrapVisualizationBegin.value()),
    Settings::Settings::lineWrapIndentation.value());
  qsci->setWhitespaceVisibility(conv.toShowWhitespaces(Settings::Settings::showWhitespace.value()));
  qsci->setWhitespaceSize(Settings::Settings::showWhitespaceSize.value());
  qsci->setAutoIndent(Settings::Settings::autoIndent.value());
  qsci->setBackspaceUnindents(Settings::Settings::backspaceUnindents.value());

  const auto& indentStyle = Settings::Settings::indentStyle.value();
  qsci->setIndentationsUseTabs(indentStyle == "Tabs");
  const auto& tabKeyFunction = Settings::Settings::tabKeyFunction.value();
  qsci->setTabIndents(tabKeyFunction == "Indent");

  qsci->setBraceMatching(Settings::Settings::enableBraceMatching.value()
                           ? QsciScintilla::SloppyBraceMatch
                           : QsciScintilla::NoBraceMatch);
  qsci->setCaretLineVisible(Settings::Settings::highlightCurrentLine.value());
  onTextChanged();

  setupAutoComplete(false);
}

void ScintillaEditor::setupAutoComplete(const bool forceOff)
{
  if (qsci->isListActive()) {
    qsci->cancelList();
  }

  if (qsci->isCallTipActive()) {
    qsci->SendScintilla(QsciScintilla::SCI_CALLTIPCANCEL);
  }

  const bool configValue = GlobalPreferences::inst()->getValue("editor/enableAutocomplete").toBool();
  const bool enable = configValue && !forceOff;

  if (enable) {
    qsci->setAutoCompletionSource(QsciScintilla::AcsAPIs);
    qsci->setAutoCompletionFillupsEnabled(false);
    qsci->setAutoCompletionFillups("(");
    qsci->setCallTipsVisible(10);
    qsci->setCallTipsStyle(QsciScintilla::CallTipsContext);
  } else {
    qsci->setAutoCompletionSource(QsciScintilla::AcsNone);
    qsci->setAutoCompletionFillupsEnabled(false);
    qsci->setCallTipsStyle(QsciScintilla::CallTipsNone);
  }

  int val = GlobalPreferences::inst()->getValue("editor/characterThreshold").toInt();
  qsci->setAutoCompletionThreshold(val <= 0 ? 1 : val);
}

void ScintillaEditor::fireModificationChanged() { emit modificationChanged(this); }

void ScintillaEditor::setPlainText(const QString& text)
{
  qsci->setText(text);
  setContentModified(false);
}

QString ScintillaEditor::toPlainText() { return qsci->text(); }

void ScintillaEditor::setContentModified(bool modified)
{
  // FIXME: Due to an issue with QScintilla, we need to do this on the document itself.
  qsci->SCN_SAVEPOINTLEFT();
  qsci->setModified(modified);
}

bool ScintillaEditor::isContentModified() { return qsci->isModified(); }

void ScintillaEditor::highlightError(int error_pos)
{
  int line, index;
  qsci->lineIndexFromPosition(error_pos, &line, &index);
  qsci->fillIndicatorRange(line, index, line, index + 1, errorIndicatorNumber);
  qsci->markerAdd(line, errMarkerNumber);
  updateSymbolMarginVisibility();
}

void ScintillaEditor::unhighlightLastError()
{
  auto totalLength = qsci->length();
  int line, index;
  qsci->lineIndexFromPosition(totalLength, &line, &index);
  qsci->clearIndicatorRange(0, 0, line, index, errorIndicatorNumber);
  qsci->markerDeleteAll(errMarkerNumber);
  updateSymbolMarginVisibility();
}

QColor ScintillaEditor::readColor(const boost::property_tree::ptree& pt, const std::string& name,
                                  const QColor& defaultColor)
{
  try {
    const auto val = pt.get<std::string>(name);
    return {val.c_str()};
  } catch (const std::exception& e) {
    return defaultColor;
  }
}

std::string ScintillaEditor::readString(const boost::property_tree::ptree& pt, const std::string& name,
                                        const std::string& defaultValue)
{
  try {
    return pt.get<std::string>(name);
  } catch (const std::exception& e) {
    return defaultValue;
  }
}

int ScintillaEditor::readInt(const boost::property_tree::ptree& pt, const std::string& name,
                             const int defaultValue)
{
  try {
    const auto val = pt.get<int>(name);
    return val;
  } catch (const std::exception& e) {
    return defaultValue;
  }
}

#if ENABLE_LEXERTL
void ScintillaEditor::setLexer(ScadLexer2 *newLexer)
{
  delete this->api;
  this->qsci->setLexer(newLexer);
  this->api = new ScadApi(this, newLexer);
  delete this->lexer;
  this->lexer = newLexer;
}
#else
void ScintillaEditor::setLexer(ScadLexer *newLexer)
{
  delete this->api;
  this->qsci->setLexer(newLexer);
  this->api = new ScadApi(this, newLexer);
  delete this->lexer;
  this->lexer = newLexer;
}
#endif  // if ENABLE_LEXERTL

void ScintillaEditor::setColormap(const EditorColorScheme *colorScheme)
{
  const auto& pt = colorScheme->propertyTree();

  try {
    auto font = this->lexer->font(this->lexer->defaultStyle());
    const QColor textColor(pt.get<std::string>("text").c_str());
    const QColor paperColor(pt.get<std::string>("paper").c_str());

#if ENABLE_LEXERTL

    /// See original attempt at https://github.com/openscad/openscad/tree/lexertl/src

    auto *newLexer = new ScadLexer2(this);

    // Custom keywords must be set before the lexer is constructed/finalized
    boost::optional<const boost::property_tree::ptree&> keywords = pt.get_child_optional("keywords");
    if (keywords.is_initialized()) {
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom1", ""), ScadLexer2::Custom1);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom2", ""), ScadLexer2::Custom2);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom3", ""), ScadLexer2::Custom3);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom4", ""), ScadLexer2::Custom4);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom5", ""), ScadLexer2::Custom5);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom6", ""), ScadLexer2::Custom6);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom7", ""), ScadLexer2::Custom7);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom8", ""), ScadLexer2::Custom8);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom9", ""), ScadLexer2::Custom9);
      newLexer->addKeywords(readString(keywords.get(), "keyword-custom10", ""), ScadLexer2::Custom10);
    }

    newLexer->finalizeLexer();
    setLexer(newLexer);

    // All other properties must be set after attaching to QSCintilla so
    // the editor gets the change events and updates itself to match
    newLexer->setFont(font);
    newLexer->setColor(textColor);
    newLexer->setPaper(paperColor);

    const auto& colors = pt.get_child("colors");

    newLexer->setColor(readColor(colors, "operator", textColor), ScadLexer2::Operator);
    newLexer->setColor(readColor(colors, "comment", textColor), ScadLexer2::Comment);
    newLexer->setColor(readColor(colors, "number", textColor), ScadLexer2::Number);
    newLexer->setColor(readColor(colors, "string", textColor), ScadLexer2::String);
    newLexer->setColor(readColor(colors, "variables", textColor), ScadLexer2::Variable);
    newLexer->setColor(readColor(colors, "keywords", textColor),
                       ScadLexer2::Keyword);  // formerly keyword1
    newLexer->setColor(readColor(colors, "transformations", textColor),
                       ScadLexer2::Transformation);  // formerly keyword3
    newLexer->setColor(readColor(colors, "booleans", textColor),
                       ScadLexer2::Boolean);  // formerly keyword3
    newLexer->setColor(readColor(colors, "functions", textColor),
                       ScadLexer2::Function);                                       // formerly keyword2
    newLexer->setColor(readColor(colors, "models", textColor), ScadLexer2::Model);  // formerly keyword3
    newLexer->setColor(readColor(colors, "special-variables", textColor),
                       ScadLexer2::SpecialVariable);  // formerly keyword1

    newLexer->setColor(readColor(colors, "keyword-custom1", textColor), ScadLexer2::Custom1);
    newLexer->setColor(readColor(colors, "keyword-custom2", textColor), ScadLexer2::Custom2);
    newLexer->setColor(readColor(colors, "keyword-custom3", textColor), ScadLexer2::Custom3);
    newLexer->setColor(readColor(colors, "keyword-custom4", textColor), ScadLexer2::Custom4);
    newLexer->setColor(readColor(colors, "keyword-custom5", textColor), ScadLexer2::Custom5);
    newLexer->setColor(readColor(colors, "keyword-custom6", textColor), ScadLexer2::Custom6);
    newLexer->setColor(readColor(colors, "keyword-custom7", textColor), ScadLexer2::Custom7);
    newLexer->setColor(readColor(colors, "keyword-custom8", textColor), ScadLexer2::Custom8);
    newLexer->setColor(readColor(colors, "keyword-custom9", textColor), ScadLexer2::Custom9);
    newLexer->setColor(readColor(colors, "keyword-custom10", textColor), ScadLexer2::Custom10);

#else
    auto *newLexer = new ScadLexer(this);

    // Keywords must be set before the lexer is attached to QScintilla
    // as they seem to be read and cached at attach time.
    boost::optional<const boost::property_tree::ptree&> keywords = pt.get_child_optional("keywords");
    if (keywords.is_initialized()) {
      newLexer->setKeywords(1, readString(keywords.get(), "keyword-set1", ""));
      newLexer->setKeywords(2, readString(keywords.get(), "keyword-set2", ""));
      newLexer->setKeywords(3, readString(keywords.get(), "keyword-set-doc", ""));
      newLexer->setKeywords(4, readString(keywords.get(), "keyword-set3", ""));
    }

    // See https://github.com/openscad/openscad/issues/1172 for details about why we can't do syntax
    // coloring with # lines
    newLexer->setStylePreprocessor(
      true);  // does not work on first word, but allows remaining words to be syntax colored

    setLexer(newLexer);

    // All other properties must be set after attaching to QSCintilla so
    // the editor gets the change events and updates itself to match
    newLexer->setFont(font);
    newLexer->setColor(textColor);
    newLexer->setPaper(paperColor);

    const auto& colors = pt.get_child("colors");
    newLexer->setColor(readColor(colors, "keyword1", textColor), QsciLexerCPP::Keyword);
    newLexer->setColor(readColor(colors, "keyword2", textColor), QsciLexerCPP::KeywordSet2);
    newLexer->setColor(readColor(colors, "keyword3", textColor), QsciLexerCPP::GlobalClass);
    newLexer->setColor(readColor(colors, "number", textColor), QsciLexerCPP::Number);
    newLexer->setColor(readColor(colors, "string", textColor), QsciLexerCPP::DoubleQuotedString);
    newLexer->setColor(readColor(colors, "operator", textColor), QsciLexerCPP::Operator);
    newLexer->setColor(readColor(colors, "comment", textColor), QsciLexerCPP::Comment);
    newLexer->setColor(readColor(colors, "commentline", textColor), QsciLexerCPP::CommentLine);
    newLexer->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentDoc);
    newLexer->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentLineDoc);
    newLexer->setColor(readColor(colors, "commentdockeyword", textColor),
                       QsciLexerCPP::CommentDocKeyword);

#endif  // ENABLE_LEXERTL

    // Somehow, the margin font got lost when we deleted the old lexer
    qsci->setMarginsFont(font);

    const auto& caret = pt.get_child("caret");
    qsci->setCaretWidth(readInt(caret, "width", 1));
    qsci->setCaretForegroundColor(readColor(caret, "foreground", textColor));
    qsci->setCaretLineBackgroundColor(readColor(caret, "line-background", paperColor));

    qsci->setMarkerBackgroundColor(readColor(colors, "error-marker", QColor(255, 0, 0, 100)),
                                   errMarkerNumber);
    qsci->setMarkerBackgroundColor(readColor(colors, "bookmark-marker", QColor(150, 200, 255, 100)),
                                   bmMarkerNumber);  // light blue
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker1", QColor(11, 156, 49, 100)),
                                   selectionMarkerLevelNumber);
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker2", QColor(11, 156, 49, 50)),
                                   selectionMarkerLevelNumber + 1);
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker3", QColor(11, 156, 49, 50)),
                                   selectionMarkerLevelNumber + 2);
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker4", QColor(11, 156, 49, 50)),
                                   selectionMarkerLevelNumber + 3);
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker5", QColor(11, 156, 49, 50)),
                                   selectionMarkerLevelNumber + 4);
    qsci->setMarkerBackgroundColor(readColor(colors, "reference-marker6", QColor(11, 156, 49, 50)),
                                   selectionMarkerLevelNumber + 5);
    qsci->setMarkerBackgroundColor(readColor(colors, "bookmark-marker", QColor(150, 200, 255, 50)),
                                   bmMarkerNumber);  // light blue
    qsci->setIndicatorForegroundColor(
      readColor(colors, "selected-highlight-indicator", QColor(11, 156, 49, 100)),
      selectionIndicatorIsActiveNumber);  // light green
    qsci->setIndicatorOutlineColor(
      readColor(colors, "selected-highlight-indicator-outline", QColor(11, 156, 49, 100)),
      selectionIndicatorIsActiveNumber);  // light green
    qsci->setIndicatorForegroundColor(
      readColor(colors, "selected-highlight1-indicator", QColor(11, 156, 49, 50)),
      selectionIndicatorIsActiveNumber + 1);  // light green
    qsci->setIndicatorOutlineColor(
      readColor(colors, "selected-highlight1-indicator-outline", QColor(11, 156, 49, 50)),
      selectionIndicatorIsActiveNumber + 1);  // light green
    qsci->setIndicatorForegroundColor(
      readColor(colors, "referenced-highlight0-indicator", QColor(255, 128, 128, 100)),
      selectionIndicatorIsImpactedNumber);  // light green
    qsci->setIndicatorOutlineColor(
      readColor(colors, "referenced-highlight0-indicator-outline", QColor(255, 128, 128, 100)),
      selectionIndicatorIsImpactedNumber);  // light green
    qsci->setIndicatorForegroundColor(
      readColor(colors, "referenced-highlight1-indicator", QColor(255, 128, 128, 100)),
      selectionIndicatorIsImpactedNumber + 1);  // light green
    qsci->setIndicatorOutlineColor(
      readColor(colors, "referenced-highlight1-indicator-outline", QColor(255, 128, 128, 80)),
      selectionIndicatorIsImpactedNumber + 1);  // light green
    qsci->setIndicatorForegroundColor(
      readColor(colors, "referenced-highlight2-indicator", QColor(255, 128, 128, 100)),
      selectionIndicatorIsImpactedNumber + 2);  // light green
    qsci->setIndicatorOutlineColor(
      readColor(colors, "referenced-highlight2-indicator-outline", QColor(255, 128, 128, 60)),
      selectionIndicatorIsImpactedNumber + 2);  // light green
    qsci->setIndicatorForegroundColor(readColor(colors, "error-indicator", QColor(255, 0, 0, 100)),
                                      errorIndicatorNumber);  // red
    qsci->setIndicatorOutlineColor(readColor(colors, "error-indicator-outline", QColor(255, 0, 0, 100)),
                                   errorIndicatorNumber);  // red
    qsci->setIndicatorForegroundColor(readColor(colors, "find-indicator", QColor(255, 255, 0, 100)),
                                      findIndicatorNumber);  // yellow
    qsci->setIndicatorOutlineColor(readColor(colors, "find-indicator-outline", QColor(255, 255, 0, 100)),
                                   findIndicatorNumber);  // yellow
    qsci->setIndicatorForegroundColor(
      readColor(colors, "hyperlink-indicator", QColor(139, 24, 168, 100)),
      hyperlinkIndicatorNumber);  // violet
    qsci->setIndicatorOutlineColor(
      readColor(colors, "hyperlink-indicator-outline", QColor(139, 24, 168, 100)),
      hyperlinkIndicatorNumber);  // violet
    qsci->setIndicatorHoverForegroundColor(
      readColor(colors, "hyperlink-indicator-hover", QColor(139, 24, 168, 100)),
      hyperlinkIndicatorNumber);  // violet
    qsci->setWhitespaceForegroundColor(readColor(colors, "whitespace-foreground", textColor));
    qsci->setMarginsBackgroundColor(readColor(colors, "margin-background", paperColor));
    qsci->setMarginsForegroundColor(readColor(colors, "margin-foreground", textColor));
    qsci->setFoldMarginColors(readColor(colors, "margin-background", paperColor),
                              readColor(colors, "margin-background", paperColor));
    qsci->setMatchedBraceBackgroundColor(readColor(colors, "matched-brace-background", paperColor));
    qsci->setMatchedBraceForegroundColor(readColor(colors, "matched-brace-foreground", textColor));
    qsci->setUnmatchedBraceBackgroundColor(readColor(colors, "unmatched-brace-background", paperColor));
    qsci->setUnmatchedBraceForegroundColor(readColor(colors, "unmatched-brace-foreground", textColor));
    qsci->setSelectionForegroundColor(readColor(colors, "selection-foreground", paperColor));
    qsci->setSelectionBackgroundColor(readColor(colors, "selection-background", textColor));
    qsci->setEdgeColor(readColor(colors, "edge", textColor));
  } catch (const std::exception& e) {
    noColor();
  }
}

void ScintillaEditor::noColor()
{
  this->lexer->setPaper(Qt::white);
  this->lexer->setColor(Qt::black);
  qsci->setCaretWidth(2);
  qsci->setCaretForegroundColor(Qt::black);
  qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), errMarkerNumber);
  qsci->setMarkerBackgroundColor(QColor(150, 200, 255, 100), bmMarkerNumber);  // light blue
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 100), selectionMarkerLevelNumber);
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 50), selectionMarkerLevelNumber + 1);
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 50), selectionMarkerLevelNumber + 2);
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 50), selectionMarkerLevelNumber + 3);
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 50), selectionMarkerLevelNumber + 4);
  qsci->setMarkerBackgroundColor(QColor(11, 156, 49, 50), selectionMarkerLevelNumber + 5);
  qsci->setMarkerBackgroundColor(QColor(150, 200, 255, 100), bmMarkerNumber);  // light blue
  qsci->setIndicatorForegroundColor(QColor(11, 156, 49, 100), selectionIndicatorIsActiveNumber);
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), selectionIndicatorIsActiveNumber);
  qsci->setIndicatorForegroundColor(QColor(11, 156, 49, 50), selectionIndicatorIsActiveNumber + 1);
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), selectionIndicatorIsActiveNumber + 1);
  qsci->setIndicatorForegroundColor(QColor(255, 128, 128, 100), selectionIndicatorIsImpactedNumber);
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), selectionIndicatorIsImpactedNumber);
  qsci->setIndicatorForegroundColor(QColor(255, 128, 128, 80), selectionIndicatorIsImpactedNumber + 1);
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), selectionIndicatorIsImpactedNumber + 1);
  qsci->setIndicatorForegroundColor(QColor(255, 128, 128, 60), selectionIndicatorIsImpactedNumber + 2);
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), selectionIndicatorIsImpactedNumber + 2);

  qsci->setIndicatorForegroundColor(QColor(255, 0, 0, 128), errorIndicatorNumber);  // red
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), errorIndicatorNumber);  // only alpha part is used
  qsci->setIndicatorForegroundColor(QColor(255, 255, 0, 128), findIndicatorNumber);  // yellow
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), findIndicatorNumber);  // only alpha part is used
  qsci->setIndicatorForegroundColor(QColor(139, 24, 168, 128), hyperlinkIndicatorNumber);  // violet
  qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255),
                                 hyperlinkIndicatorNumber);  // only alpha part is used
  qsci->setIndicatorHoverForegroundColor(QColor(139, 24, 168, 128), hyperlinkIndicatorNumber);  // violet
  qsci->setCaretLineBackgroundColor(Qt::white);
  qsci->setWhitespaceForegroundColor(Qt::black);
  qsci->setSelectionForegroundColor(Qt::black);
  qsci->setSelectionBackgroundColor(QColor("LightSkyBlue"));
  qsci->setMatchedBraceBackgroundColor(QColor("LightBlue"));
  qsci->setMatchedBraceForegroundColor(Qt::black);
  qsci->setUnmatchedBraceBackgroundColor(QColor("pink"));
  qsci->setUnmatchedBraceForegroundColor(Qt::black);
  qsci->setMarginsBackgroundColor(QColor("whiteSmoke"));
  qsci->setMarginsForegroundColor(QColor("gray"));
  qsci->setFoldMarginColors(QColor("whiteSmoke"), QColor("whiteSmoke"));
  qsci->setEdgeColor(Qt::black);
}

void ScintillaEditor::enumerateColorSchemesInPath(ScintillaEditor::colorscheme_set_t& result_set,
                                                  const fs::path& path)
{
  const auto color_schemes = path / "color-schemes" / "editor";

  if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
    for (const auto& dirEntry : boost::make_iterator_range(fs::directory_iterator{color_schemes}, {})) {
      if (!fs::is_regular_file(dirEntry.status())) continue;

      const auto& path = dirEntry.path();
      if (!(path.extension() == ".json")) continue;

      auto colorScheme = std::make_shared<EditorColorScheme>(path);
      if (colorScheme->valid()) {
        result_set.emplace(colorScheme->index(), colorScheme);
      }
    }
  }
}

ScintillaEditor::colorscheme_set_t ScintillaEditor::enumerateColorSchemes()
{
  colorscheme_set_t result_set;

  enumerateColorSchemesInPath(result_set, PlatformUtils::resourceBasePath());
  enumerateColorSchemesInPath(result_set, PlatformUtils::userConfigPath());

  return result_set;
}

QStringList ScintillaEditor::colorSchemes()
{
  QStringList colorSchemes;
  for (const auto& colorSchemeEntry : enumerateColorSchemes()) {
    colorSchemes << colorSchemeEntry.second.get()->name();
  }
  colorSchemes << "Off";

  return colorSchemes;
}

bool ScintillaEditor::canUndo() { return qsci->isUndoAvailable(); }

void ScintillaEditor::setHighlightScheme(const QString& name)
{
  for (const auto& colorSchemeEntry : enumerateColorSchemes()) {
    const auto colorScheme = colorSchemeEntry.second.get();
    if (colorScheme->name() == name) {
      setColormap(colorScheme);
      return;
    }
  }

  noColor();
}

void ScintillaEditor::insert(const QString& text) { qsci->insert(text); }

void ScintillaEditor::setText(const QString& text)
{
  qsci->selectAll(true);
  qsci->replaceSelectedText(text);
}

void ScintillaEditor::undo() { qsci->undo(); }

void ScintillaEditor::redo() { qsci->redo(); }

void ScintillaEditor::cut() { qsci->cut(); }

void ScintillaEditor::copy() { qsci->copy(); }

void ScintillaEditor::paste() { qsci->paste(); }

void ScintillaEditor::zoomIn() { qsci->zoomIn(); }

void ScintillaEditor::zoomOut() { qsci->zoomOut(); }

void ScintillaEditor::initFont(const QString& fontName, uint size)
{
  this->currentFont = QFont(fontName, size);
  this->currentFont.setFixedPitch(true);
  this->lexer->setFont(this->currentFont);
  qsci->setMarginsFont(this->currentFont);
  onTextChanged();  // Update margin width
}

void ScintillaEditor::initMargin()
{
  connect(qsci, &QsciScintilla::textChanged, this, &ScintillaEditor::onTextChanged);
}

void ScintillaEditor::onTextChanged()
{
  auto enableLineNumbers = Settings::Settings::enableLineNumbers.value();
  if (enableLineNumbers) {
    qsci->setMarginWidth(numberMargin, QString(trunc(log10(qsci->lines()) + 2), '0'));
  } else {
    qsci->setMarginWidth(numberMargin, 6);
  }
  qsci->setMarginLineNumbers(numberMargin, enableLineNumbers);
}

int ScintillaEditor::updateFindIndicators(const QString& findText, bool visibility)
{
  int findwordcount{0};

  qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, findIndicatorNumber);
  qsci->SendScintilla(qsci->SCI_INDICATORCLEARRANGE, 0, qsci->length());

  const auto txt = qsci->text().toUtf8().toLower();
  const auto findTextUtf8 = findText.toUtf8().toLower();
  auto pos = txt.indexOf(findTextUtf8);
  auto len = findTextUtf8.length();
  if (visibility && len > 0) {
    while (pos != -1) {
      findwordcount++;
      qsci->SendScintilla(qsci->SCI_SETINDICATORCURRENT, findIndicatorNumber);
      qsci->SendScintilla(qsci->SCI_INDICATORFILLRANGE, pos, len);
      pos = txt.indexOf(findTextUtf8, pos + len);
    }
  }
  return findwordcount;
}

bool ScintillaEditor::find(const QString& expr, bool findNext, bool findBackwards)
{
  int startline = -1, startindex = -1;

  // If findNext, start from the end of the current selection
  if (qsci->hasSelectedText()) {
    int lineFrom, indexFrom, lineTo, indexTo;
    qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

    startline = !(findBackwards xor findNext) ? std::min(lineFrom, lineTo) : std::max(lineFrom, lineTo);
    startindex =
      !(findBackwards xor findNext) ? std::min(indexFrom, indexTo) : std::max(indexFrom, indexTo);
  }

  return qsci->findFirst(expr, false, false, false, true, !findBackwards, startline, startindex);
}

bool ScintillaEditor::replaceSelectedText(const QString& newText)
{
  if ((qsci->selectedText() != newText) && (qsci->hasSelectedText())) {
    qsci->replaceSelectedText(newText);
    return true;
  }
  return false;
}

void ScintillaEditor::insertOrReplaceText(const QString& newText)
{
  if (!replaceSelectedText(newText)) {
    qsci->insert(newText);
  }
}

void ScintillaEditor::replaceAll(const QString& findText, const QString& replaceText)
{
  // We need to issue a Select All first due to a bug in QScintilla:
  // It doesn't update the find range when just doing findFirst() + findNext() causing the search
  // to end prematurely if the replaced string is larger than the selected string.
#if QSCINTILLA_VERSION >= 0x020903
  // QScintilla bug seems to be fixed in 2.9.3
  if (qsci->findFirst(findText, false /*re*/, false /*cs*/, false /*wo*/, false /*wrap*/,
                      true /*forward*/, 0, 0)) {
#elif QSCINTILLA_VERSION >= 0x020700
  qsci->selectAll();
  if (qsci->findFirstInSelection(findText, false /*re*/, false /*cs*/, false /*wo*/, false /*wrap*/,
                                 true /*forward*/)) {
#else
  // findFirstInSelection() was introduced in QScintilla 2.7
  if (qsci->findFirst(findText, false /*re*/, false /*cs*/, false /*wo*/, false /*wrap*/,
                      true /*forward*/, 0, 0)) {
#endif  // if QSCINTILLA_VERSION >= 0x020903
    qsci->replace(replaceText);
    while (qsci->findNext()) {
      qsci->replace(replaceText);
    }
  }
}

void ScintillaEditor::getRange(int *lineFrom, int *lineTo)
{
  int indexFrom, indexTo;
  if (qsci->hasSelectedText()) {
    qsci->getSelection(lineFrom, &indexFrom, lineTo, &indexTo);
    if (indexTo == 0) {
      *lineTo = *lineTo - 1;
    }
  } else {
    qsci->getCursorPosition(lineFrom, &indexFrom);
    *lineTo = *lineFrom;
  }
}

void ScintillaEditor::indentSelection()
{
  int lineFrom, lineTo;
  qsci->beginUndoAction();
  getRange(&lineFrom, &lineTo);
  for (int line = lineFrom; line <= lineTo; ++line) {
    if (qsci->SendScintilla(QsciScintilla::SCI_GETLINEENDPOSITION, line) -
          qsci->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, line) ==
        0) {
      continue;
    }
    qsci->indent(line);
  }
  int nextLine = lineTo + 1;
  while (qsci->SendScintilla(QsciScintilla::SCI_GETLINEVISIBLE, nextLine) == 0) {
    if (qsci->SendScintilla(QsciScintilla::SCI_GETLINEENDPOSITION, nextLine) -
          qsci->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, nextLine) ==
        0) {
      nextLine++;
      continue;
    }
    qsci->indent(nextLine);
    nextLine++;
  }
  qsci->endUndoAction();
}

void ScintillaEditor::unindentSelection()
{
  int lineFrom, lineTo;
  qsci->beginUndoAction();
  getRange(&lineFrom, &lineTo);
  for (int line = lineFrom; line <= lineTo; ++line) {
    if (qsci->SendScintilla(QsciScintilla::SCI_GETLINEENDPOSITION, line) -
          qsci->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, line) ==
        0) {
      continue;
    }
    qsci->unindent(line);
  }
  int nextLine = lineTo + 1;
  while (qsci->SendScintilla(QsciScintilla::SCI_GETLINEVISIBLE, nextLine) == 0) {
    if (qsci->SendScintilla(QsciScintilla::SCI_GETLINEENDPOSITION, nextLine) -
          qsci->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, nextLine) ==
        0) {
      nextLine++;
      continue;
    }
    qsci->unindent(nextLine);
    nextLine++;
  }
  qsci->endUndoAction();
}

void ScintillaEditor::commentSelection()
{
  auto hasSelection = qsci->hasSelectedText();

  int lineFrom, lineTo;
  getRange(&lineFrom, &lineTo);
  for (int line = lineFrom; line <= lineTo; ++line) {
    qsci->insertAt("//", line, 0);
  }

  if (hasSelection) {
    qsci->setSelection(lineFrom, 0, lineTo, std::max(0, qsci->lineLength(lineTo) - 1));
  }
}

void ScintillaEditor::uncommentSelection()
{
  auto hasSelection = qsci->hasSelectedText();

  int lineFrom, lineTo;
  getRange(&lineFrom, &lineTo);
  for (int line = lineFrom; line <= lineTo; ++line) {
    QString lineText = qsci->text(line);
    if (lineText.startsWith("//")) {
      qsci->setSelection(line, 0, line, 2);
      qsci->removeSelectedText();
    }
  }
  if (hasSelection) {
    qsci->setSelection(lineFrom, 0, lineTo, std::max(0, qsci->lineLength(lineTo) - 1));
  }
}

QString ScintillaEditor::selectedText() { return qsci->selectedText(); }

bool ScintillaEditor::eventFilter(QObject *obj, QEvent *e)
{
  if (e->type() == QEvent::KeyPress) {
    auto keyEvent = static_cast<QKeyEvent *>(e);
    if (keyEvent->key() == Qt::Key_Escape) {
      emit escapePressed();
    }
  }
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ||
      QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
    if (!this->indicatorsActive) {
      this->indicatorsActive = true;
      qsci->setIndicatorHoverStyle(QsciScintilla::PlainIndicator, hyperlinkIndicatorNumber);
    }
  } else {
    if (this->indicatorsActive) {
      this->indicatorsActive = false;
      qsci->setIndicatorHoverStyle(QsciScintilla::HiddenIndicator, hyperlinkIndicatorNumber);
    }
  }

  if (obj == qsci->viewport()) {
    if (e->type() == QEvent::Wheel) {
      auto *wheelEvent = static_cast<QWheelEvent *>(e);
      PRINTDB("%s - modifier: %s",
              (e->type() == QEvent::Wheel ? "Wheel Event" : "") %
                (wheelEvent->modifiers() & Qt::AltModifier ? "Alt" : "Other Button"));
      bool enableNumberScrollWheel = Settings::Settings::enableNumberScrollWheel.value();
      if (enableNumberScrollWheel && handleWheelEventNavigateNumber(wheelEvent)) {
        qsci->SendScintilla(QsciScintilla::SCI_SETCARETWIDTH, 1);
        return true;
      }
      bool wheelzoom_enabled = GlobalPreferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
      if ((wheelEvent->modifiers() == Qt::ControlModifier) && !wheelzoom_enabled) {
        return true;
      }
    }
    return false;
  } else if (obj == qsci) {
    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
      auto *keyEvent = static_cast<QKeyEvent *>(e);

      PRINTDB("%10s - modifiers: %s %s %s %s %s %s",
              (e->type() == QEvent::KeyPress ? "KeyPress" : "KeyRelease") %
                (keyEvent->modifiers() & Qt::ShiftModifier ? "SHIFT" : "shift") %
                (keyEvent->modifiers() & Qt::ControlModifier ? "CTRL" : "ctrl") %
                (keyEvent->modifiers() & Qt::AltModifier ? "ALT" : "alt") %
                (keyEvent->modifiers() & Qt::MetaModifier ? "META" : "meta") %
                (keyEvent->modifiers() & Qt::KeypadModifier ? "KEYPAD" : "keypad") %
                (keyEvent->modifiers() & Qt::GroupSwitchModifier ? "GROUP" : "group"));

      if (handleKeyEventNavigateNumber(keyEvent)) {
        return true;
      }
      if (handleKeyEventBlockCopy(keyEvent)) {
        return true;
      }
      if (handleKeyEventBlockMove(keyEvent)) {
        return true;
      }
    }
    return false;
  } else return EditorInterface::eventFilter(obj, e);

  return false;
}

bool ScintillaEditor::handleKeyEventBlockMove(QKeyEvent *keyEvent)
{
  unsigned int modifiers = Qt::ControlModifier | Qt::GroupSwitchModifier;

  if (keyEvent->type() != QEvent::KeyRelease) {
    return false;
  }

  if (keyEvent->modifiers() != modifiers) {
    return false;
  }

  if (keyEvent->key() != Qt::Key_Up && keyEvent->key() != Qt::Key_Down) {
    return false;
  }

  int line, index;
  qsci->getCursorPosition(&line, &index);
  int lineFrom, indexFrom, lineTo, indexTo;
  qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  if (lineFrom < 0) {
    lineTo = lineFrom = line;
    indexFrom = indexTo = 0;
  }
  int selectionLineTo = lineTo;
  if (lineTo > lineFrom && indexTo == 0) {
    lineTo--;
  }

  bool up = keyEvent->key() == Qt::Key_Up;
  int directionOffset = up ? -1 : 1;
  int lineToMove = up ? lineFrom - 1 : lineTo + 1;
  if (lineToMove < 0) {
    return false;
  }

  qsci->beginUndoAction();
  QString textToMove = qsci->text(lineToMove);
  QString text;
  for (int idx = lineFrom; idx <= lineTo; ++idx) {
    text.append(qsci->text(idx));
  }
  if (lineToMove >= qsci->lines() - 1) {
    textToMove.append('\n');
  }
  text.insert(up ? text.length() : 0, textToMove);
  qsci->setSelection(std::min(lineToMove, lineFrom), 0, std::max(lineToMove, lineTo) + 1, 0);
  qsci->replaceSelectedText(text);
  qsci->setCursorPosition(line + directionOffset, index);
  qsci->setSelection(lineFrom + directionOffset, indexFrom, selectionLineTo + directionOffset, indexTo);
  qsci->endUndoAction();
  return true;
}

bool ScintillaEditor::handleKeyEventBlockCopy(QKeyEvent *keyEvent)
{
  unsigned int modifiers = Qt::ControlModifier | Qt::ShiftModifier;

  if (keyEvent->type() != QEvent::KeyRelease) {
    return false;
  }

  if (keyEvent->modifiers() != modifiers) {
    return false;
  }

  if (keyEvent->key() != Qt::Key_Up && keyEvent->key() != Qt::Key_Down) {
    return false;
  }

  int line, index;
  qsci->getCursorPosition(&line, &index);
  int lineFrom, indexFrom, lineTo, indexTo;
  qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  if (lineFrom < 0) {
    lineTo = lineFrom = line;
    indexFrom = indexTo = 0;
  }

  bool up = keyEvent->key() == Qt::Key_Up;
  int selectionLineTo = 0;
  if (lineTo > lineFrom && indexTo == 0) {
    lineTo--;
    selectionLineTo++;
  }
  int cursorLine = up ? line : lineTo + 1;
  int selectionLineFrom = up ? lineFrom : lineTo + 1;
  selectionLineTo += up ? lineTo : 2 * lineTo - lineFrom + 1;

  qsci->beginUndoAction();
  QString text;
  for (int line = lineFrom; line <= lineTo; ++line) {
    text += qsci->text(line);
  }
  if (lineTo + 1 >= qsci->lines()) {
    text.insert(0, '\n');
  }
  qsci->insertAt(text, lineTo + 1, 0);
  qsci->setCursorPosition(cursorLine, index);
  qsci->setSelection(selectionLineFrom, indexFrom, selectionLineTo, indexTo);
  qsci->endUndoAction();
  return true;
}

bool ScintillaEditor::handleKeyEventNavigateNumber(QKeyEvent *keyEvent)
{
  static bool previewAfterUndo = false;

#ifdef Q_OS_MACOS
  unsigned int navigateOnNumberModifiers = Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier;
#else
  unsigned int navigateOnNumberModifiers = Qt::AltModifier;
#endif
  if (keyEvent->modifiers() == navigateOnNumberModifiers) {
    switch (keyEvent->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
      if (keyEvent->type() == QEvent::KeyPress) {
        navigateOnNumber(keyEvent->key());
      }
      return true;

    case Qt::Key_Up:
    case Qt::Key_Down:
      if (keyEvent->type() == QEvent::KeyPress) {
        if (modifyNumber(keyEvent->key())) {
          previewAfterUndo = true;
        }
      }
      return true;
    }
  }
  if (previewAfterUndo && keyEvent->type() == QEvent::KeyPress) {
    int k = keyEvent->key() | keyEvent->modifiers();
    auto *cmd = qsci->standardCommands()->boundTo(k);
    if (cmd && (cmd->command() == QsciCommand::Undo || cmd->command() == QsciCommand::Redo))
      QTimer::singleShot(0, this, &ScintillaEditor::previewRequest);
    else if (cmd || !keyEvent->text().isEmpty()) {
      // any insert or command (but not undo/redo) cancels the preview after undo
      previewAfterUndo = false;
    }
  }
  return false;
}

bool ScintillaEditor::handleWheelEventNavigateNumber(QWheelEvent *wheelEvent)
{
  const auto& modifierNumberScrollWheel = Settings::Settings::modifierNumberScrollWheel.value();
  bool modifier;
  static bool previewAfterUndo = false;

  if (modifierNumberScrollWheel == "Alt") {
    modifier = wheelEvent->modifiers() & Qt::AltModifier;
  } else if (modifierNumberScrollWheel == "Left Mouse Button") {
    modifier = wheelEvent->buttons() & Qt::LeftButton;
  } else {
    modifier = (wheelEvent->buttons() & Qt::LeftButton) | (wheelEvent->modifiers() & Qt::AltModifier);
  }

  if (modifier) {
    int delta =
      wheelEvent->angleDelta().y() != 0 ? wheelEvent->angleDelta().y() : wheelEvent->angleDelta().x();

    if (delta < 0) {
      if (modifyNumber(Qt::Key_Down)) {
        previewAfterUndo = true;
      }
    } else {
      // delta > 0
      if (modifyNumber(Qt::Key_Up)) {
        previewAfterUndo = true;
      }
    }

    return true;
  }

  if (previewAfterUndo) {
    int k = wheelEvent->buttons() & Qt::LeftButton;
    auto *cmd = qsci->standardCommands()->boundTo(k);
    if (cmd && (cmd->command() == QsciCommand::Undo || cmd->command() == QsciCommand::Redo))
      QTimer::singleShot(0, this, &ScintillaEditor::previewRequest);
    else if (cmd || wheelEvent->angleDelta().y()) {
      // any insert or command (but not undo/redo) cancels the preview after undo
      previewAfterUndo = false;
    }
  }
  return false;
}

void ScintillaEditor::navigateOnNumber(int key)
{
  int line, index;
  qsci->getCursorPosition(&line, &index);
  auto text = qsci->text(line);
  auto left = text.left(index);
  auto dotOnLeft = left.contains(QRegularExpression("\\.\\d*$"));
  auto dotJustLeft = index > 1 && text[index - 2] == '.';
  auto dotJustRight = text[index] == '.';
  auto numOnLeft = left.contains(QRegularExpression("\\d\\.?$")) || left.endsWith("-.");
  auto numOnRight = text.indexOf(QRegularExpression("\\.?\\d"), index) == index;

  switch (key) {
  case Qt::Key_Left:
    if (numOnLeft) qsci->setCursorPosition(line, index - (dotJustLeft ? 2 : 1));
    break;

  case Qt::Key_Right:
    if (numOnRight) qsci->setCursorPosition(line, index + (dotJustRight ? 2 : 1));
    else if (numOnLeft) {
      // add trailing zero
      if (!dotOnLeft) {
        qsci->insert(".0");
        index++;
      } else {
        qsci->insert("0");
      }
      qsci->setCursorPosition(line, index + 1);
    }
    break;
  }
}

bool ScintillaEditor::modifyNumber(int key)
{
  int line, index;
  qsci->getCursorPosition(&line, &index);
  auto text = qsci->text(line);
  int lineFrom, indexFrom, lineTo, indexTo;
  qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  auto hadSelection = qsci->hasSelectedText();
  qsci->SendScintilla(QsciScintilla::SCI_SETEMPTYSELECTION);
  qsci->setCursorPosition(line, index);

  auto begin = text.left(index).indexOf(QRegularExpression(R"([-+]?\d*\.?\d*$)"));

  QRegularExpression rx(QRegularExpression::anchoredPattern(QString("[_a-zA-Z]")));
  auto check = text.mid(begin - 1, 1);
  if (rx.match(check).hasMatch()) return false;

  auto end = text.indexOf(QRegularExpression("[^0-9.]"), index);
  if (end < 0) end = text.length();
  auto nr = text.mid(begin, end - begin);
  if (!(nr.contains(QRegularExpression(R"(^[-+]?\d*\.?\d+$)")) &&
        nr.contains(QRegularExpression("\\d"))))
    return false;
  auto sign = nr[0] == '+' || nr[0] == '-';
  if (nr.endsWith('.')) nr = nr.left(nr.length() - 1);
  auto curpos = index - begin;
  if (curpos == 0 || (curpos == 1 && (nr[0] == '+' || nr[0] == '-'))) return false;
  auto dotpos = nr.indexOf('.');
  auto decimals = dotpos < 0 ? 0 : nr.length() - dotpos - 1;
  auto number = (dotpos < 0) ? nr.toLongLong() : (nr.left(dotpos) + nr.mid(dotpos + 1)).toLongLong();
  auto tail = nr.length() - curpos;
  auto exponent = tail - ((dotpos >= curpos) ? 1 : 0);
  long long int step = GlobalPreferences::inst()->getValue("editor/stepSize").toInt();
  for (int i = exponent; i > 0; i--) step *= 10;

  switch (key) {
  case Qt::Key_Up:   number += step; break;
  case Qt::Key_Down: number -= step; break;
  }
  auto negative = number < 0;
  if (negative) number = -number;
  auto newnr = QString::number(number);
  if (decimals) {
    if (newnr.length() <= decimals) newnr.prepend(QString(decimals - newnr.length() + 1, '0'));
    newnr = newnr.left(newnr.length() - decimals) + "." + newnr.right(decimals);
  }
  if (tail > newnr.length()) {
    newnr.prepend(QString(tail - newnr.length(), '0'));
  }
  if (negative) newnr.prepend('-');
  else if (sign) newnr.prepend('+');
  qsci->beginUndoAction();
  qsci->setSelection(line, begin, line, end);
  qsci->replaceSelectedText(newnr);

  qsci->selectAll(false);
  if (hadSelection) {
    qsci->setSelection(lineFrom, indexFrom, lineTo, indexTo);
  }
  qsci->setCursorPosition(line, begin + newnr.length() - tail);
  qsci->endUndoAction();
  emit previewRequest();
  return true;
}

void ScintillaEditor::onUserListSelected(const int, const QString& text)
{
  if (!templateMap.contains(text)) {
    return;
  }

  QString tabReplace = "";
  if (Settings::Settings::indentStyle.value() == "Spaces") {
    auto spCount = Settings::Settings::indentationWidth.value();
    tabReplace = QString(spCount, ' ');
  }

  ScadTemplate& t = templateMap[text];
  QString content = t.get_text();
  int cursor_offset = t.get_cursor_offset();

  if (cursor_offset < 0) {
    if (tabReplace.size() != 0) content.replace("\t", tabReplace);

    cursor_offset = content.indexOf(ScintillaEditor::cursorPlaceHolder);
    content.remove(cursorPlaceHolder);

    if (cursor_offset == -1) cursor_offset = content.size();
  } else {
    if (tabReplace.size() != 0) {
      int tbCount = content.left(cursor_offset).count("\t");
      cursor_offset += tbCount * (tabReplace.size() - 1);
      content.replace("\t", tabReplace);
    }
  }

  qsci->insert(content);

  int line, index;
  qsci->getCursorPosition(&line, &index);
  int pos = qsci->positionFromLineIndex(line, index);

  pos += cursor_offset;
  int indent_line = line;
  int indent_width = index;
  qsci->lineIndexFromPosition(pos, &line, &index);
  qsci->setCursorPosition(line, index);

  int lines = t.get_text().count("\n");
  QString indent_char = " ";
  if (Settings::Settings::indentStyle.value() == "Tabs") indent_char = "\t";

  for (int a = 0; a < lines; ++a) {
    qsci->insertAt(indent_char.repeated(indent_width), indent_line + a + 1, 0);
  }
}

void ScintillaEditor::onAutocompleteChanged(bool state)
{
  if (state) {
    qsci->setAutoCompletionSource(QsciScintilla::AcsAPIs);
    qsci->setAutoCompletionFillupsEnabled(true);
    qsci->setCallTipsVisible(10);
    qsci->setCallTipsStyle(QsciScintilla::CallTipsContext);
  } else {
    qsci->setAutoCompletionSource(QsciScintilla::AcsNone);
    qsci->setAutoCompletionFillupsEnabled(false);
    qsci->setCallTipsStyle(QsciScintilla::CallTipsNone);
  }
}

void ScintillaEditor::onCharacterThresholdChanged(int val)
{
  qsci->setAutoCompletionThreshold(val <= 0 ? 1 : val);
}

void ScintillaEditor::resetHighlighting()
{
  qsci->recolor();  // lex and restyle the whole text

  // remove all indicators
  qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, hyperlinkIndicatorNumber);
  qsci->SendScintilla(QsciScintilla::SCI_INDICATORCLEARRANGE, 0, qsci->length());
}

void ScintillaEditor::setIndicator(const std::vector<IndicatorData>& indicatorData)
{
  this->indicatorData = indicatorData;

  int idx = 0;
  for (const auto& data : indicatorData) {
    int startPos = qsci->positionFromLineIndex(data.first_line - 1, data.first_col - 1);
    int stopPos = qsci->positionFromLineIndex(data.last_line - 1, data.last_col - 1);

    int nrOfChars = stopPos - startPos;
    qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, idx + hyperlinkIndicatorOffset);

    qsci->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, startPos, nrOfChars);

    idx++;
  }
}

void ScintillaEditor::onIndicatorClicked(int line, int col, Qt::KeyboardModifiers /*state*/)
{
  qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, hyperlinkIndicatorNumber);

  int pos = qsci->positionFromLineIndex(line, col);
  int val = qsci->SendScintilla(QsciScintilla::SCI_INDICATORVALUEAT,
                                ScintillaEditor::hyperlinkIndicatorNumber, pos);

  // checking if indicator clicked is hyperlinkIndicator
  if (val >= hyperlinkIndicatorOffset &&
      val <= hyperlinkIndicatorOffset + static_cast<int>(indicatorData.size())) {
    if (indicatorsActive) {
      emit hyperlinkIndicatorClicked(val - hyperlinkIndicatorOffset);
    }
  }
}

void ScintillaEditor::onIndicatorReleased(int line, int col, Qt::KeyboardModifiers /*state*/)
{
  qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, hyperlinkIndicatorNumber);

  int pos = qsci->positionFromLineIndex(line, col);
  int val = qsci->SendScintilla(QsciScintilla::SCI_INDICATORVALUEAT,
                                ScintillaEditor::hyperlinkIndicatorNumber, pos);

  // checking if indicator clicked is hyperlinkIndicator
  if (val >= hyperlinkIndicatorOffset &&
      val <= hyperlinkIndicatorOffset + static_cast<int>(indicatorData.size())) {
    if (!indicatorsActive) {
      QTimer::singleShot(0, this, [this] {
        QToolTip::showText(QCursor::pos(), "Use <b>CTRL + Click</b> to open the file", this, rect(),
                           toolTipDuration());
      });
    }
  }
}

void ScintillaEditor::setCursorPosition(int line, int col)
{
  qsci->ensureLineVisible(std::max(line - setCursorPositionVisibleLines, 0));
  qsci->ensureLineVisible(std::min(line + setCursorPositionVisibleLines, qsci->lines() - 1));

  qsci->setCursorPosition(line, col);
}

void ScintillaEditor::updateSymbolMarginVisibility()
{
  if (qsci->markerFindNext(
        0, 1 << bmMarkerNumber | 1 << errMarkerNumber | 1 << selectionMarkerLevelNumber |
             1 << (selectionMarkerLevelNumber + 1) | 1 << (selectionMarkerLevelNumber + 2) |
             1 << (selectionMarkerLevelNumber + 3) | 1 << (selectionMarkerLevelNumber + 4) |
             1 << (selectionMarkerLevelNumber + 5)) < 0) {
    qsci->setMarginWidth(symbolMargin, 0);
  } else {
    qsci->setMarginWidth(symbolMargin, "0");
  }
}

void ScintillaEditor::toggleBookmark()
{
  int line, index;
  qsci->getCursorPosition(&line, &index);

  unsigned int state = qsci->markersAtLine(line);

  if ((state & (1 << bmMarkerNumber)) == 0) qsci->markerAdd(line, bmMarkerNumber);
  else qsci->markerDelete(line, bmMarkerNumber);

  updateSymbolMarginVisibility();
}

void ScintillaEditor::findMarker(int findStartOffset, int wrapStart,
                                 const std::function<int(int)>& findMarkerFunc)
{
  int line, index;
  qsci->getCursorPosition(&line, &index);
  line = findMarkerFunc(line + findStartOffset);
  if (line == -1) {
    line = findMarkerFunc(wrapStart);  // wrap around
  }
  if (line != -1) {
    // make sure we don't wrap into new line
    int len = qsci->text(line).remove(QRegularExpression("[\n\r]$")).length();
    int col = std::min(index, len);
    qsci->setCursorPosition(line, col);
  }
}

void ScintillaEditor::nextBookmark()
{
  findMarker(1, 0, [this](int line) { return qsci->markerFindNext(line, 1 << bmMarkerNumber); });
}

void ScintillaEditor::prevBookmark()
{
  findMarker(-1, qsci->lines() - 1,
             [this](int line) { return qsci->markerFindPrevious(line, 1 << bmMarkerNumber); });
}

void ScintillaEditor::jumpToNextError()
{
  findMarker(1, 0, [this](int line) { return qsci->markerFindNext(line, 1 << errMarkerNumber); });
}

void ScintillaEditor::setFocus()
{
  qsci->setFocus();
  qsci->SendScintilla(QsciScintilla::SCI_SETFOCUS, true);
}

/**
 * @brief Highlights a part of the text according to the limits described in the parameters
 */
void ScintillaEditor::setSelectionIndicatorStatus(EditorSelectionIndicatorStatus status, int level,
                                                  int lineFrom, int colFrom, int lineTo, int colTo)
{
  // replace all the indicators at given lines/column with the new one
  clearSelectionIndicators(lineFrom, colFrom, lineTo, colTo);

  int indicator_base_index = 0;
  int indicator_level = 0;
  int mark_level = 0;
  if (status == EditorSelectionIndicatorStatus::SELECTED) {
    indicator_base_index = selectionIndicatorIsActiveNumber;
    mark_level = (level > 5) ? 5 : level;
    indicator_level = (level > 1) ? 1 : level;
  } else {
    indicator_base_index = selectionIndicatorIsImpactedNumber;
    indicator_level = (level > 2) ? 2 : level;
  }

  clearSelectionIndicators(lineFrom, colFrom, lineTo, colTo);
  qsci->fillIndicatorRange(lineFrom, colFrom, lineTo, colTo, indicator_base_index + indicator_level);

  if (status == EditorSelectionIndicatorStatus::SELECTED) {
    qsci->ensureLineVisible(std::max(lineFrom - setCursorPositionVisibleLines, 0));
    qsci->ensureLineVisible(std::min(lineFrom + setCursorPositionVisibleLines, qsci->lines() - 1));

    // replace the marker at provide line with a new one.
    qsci->markerDelete(lineFrom);

    qsci->markerAdd(lineFrom, selectionMarkerLevelNumber + mark_level);
    updateSymbolMarginVisibility();
  }
}

/**
 * @brief Unhighlight all the selection indicators.
 */
void ScintillaEditor::clearAllSelectionIndicators()
{
  // remove all the indicator in the document.
  int line, column;
  qsci->lineIndexFromPosition(qsci->length(), &line, &column);
  clearSelectionIndicators(0, 0, line, column);

  // remove all the markers
  qsci->markerDeleteAll(selectionMarkerLevelNumber);
  qsci->markerDeleteAll(selectionMarkerLevelNumber + 1);
  qsci->markerDeleteAll(selectionMarkerLevelNumber + 2);
  qsci->markerDeleteAll(selectionMarkerLevelNumber + 3);
  qsci->markerDeleteAll(selectionMarkerLevelNumber + 4);
  qsci->markerDeleteAll(selectionMarkerLevelNumber + 5);
}

/**
 * @brief Unhighlight all the texts for DM
 */
void ScintillaEditor::clearSelectionIndicators(int lineFrom, int colFrom, int lineTo, int colTo)
{
  qsci->clearIndicatorRange(lineFrom, colFrom, lineTo, colTo, selectionIndicatorIsImpactedNumber);
  qsci->clearIndicatorRange(lineFrom, colFrom, lineTo, colTo, selectionIndicatorIsImpactedNumber + 1);
  qsci->clearIndicatorRange(lineFrom, colFrom, lineTo, colTo, selectionIndicatorIsImpactedNumber + 2);

  qsci->clearIndicatorRange(lineFrom, colFrom, lineTo, colTo, selectionIndicatorIsActiveNumber);
  qsci->clearIndicatorRange(lineFrom, colFrom, lineTo, colTo, selectionIndicatorIsActiveNumber + 1);
}
