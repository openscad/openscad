#include "TestMainWindow.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QLineEdit>
#include <QScopeGuard>
#include <QString>
#include <QStringList>
#include <QTest>

#include "core/Settings.h"
#include "gui/ColorLabel.h"
#include "gui/ColorList.h"
#include "gui/EditorColorMap.h"
#include "gui/ScintillaEditor.h"
#include "platform/PlatformUtils.h"

void TestMainWindow::checkOpenTabPropagateToWindow()
{
  restoreWindowInitialState();

  QString filename = fixturePath("basic-ux/empty.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());

  filename = fixturePath("basic-ux/empty2.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());
}

void TestMainWindow::checkSaveToShouldUpdateWindowTitle()
{
  restoreWindowInitialState();

  QString filename = fixturePath("basic-ux/empty.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  window->tabManager->saveAs(window->activeEditor, "test-tmp.scad");

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), "test-tmp.scad");
}

void TestMainWindow::copyColorName_data()
{
  QTest::addColumn<QString>("caption");
  QTest::addColumn<QString>("name");
  QTest::newRow("XKCD") << QString("adobe") << QString("xkcd:adobe");
  QTest::newRow("web") << QString("aliceblue") << QString("aliceblue");
}

void TestMainWindow::copyColorName()
{
  QFETCH(QString, caption);
  QFETCH(QString, name);

  auto *list = window->colorListWidget;
  auto *swatch = list->findChild<ColorLabel *>(caption);
  auto *selectedName = list->findChild<QLineEdit *>("lineEditColorNameSelected");
  auto *copy = list->findChild<QAction *>("actionCopyColorName");
  QVERIFY(swatch && selectedName && copy);

  QTest::mouseClick(swatch, Qt::LeftButton);
  QCOMPARE(swatch->text(), caption);
  QCOMPARE(selectedName->text(), name);
  copy->trigger();
  QCOMPARE(QApplication::clipboard()->text(), name);
}

void TestMainWindow::highlightMatchingBraceContents()
{
  ScintillaEditor editor(nullptr);
  auto *qsci = editor.qsci;
  auto& mode = Settings::Settings::braceMatching;
  const auto originalMode = mode.value();
  const auto restoreMode = qScopeGuard([&]() { mode.setValue(originalMode); });
  const auto setMode = [&](BraceMatchingMode value) {
    mode.setValue(value);
    editor.applySettings();
  };
  qsci->setText(QString::fromUtf8("é{\n  [10,20]\n}"));
  editor.show();

  // Read the shaded bytes from this editor, which has no search or model selection.
  auto highlighted = [qsci]() {
    const auto bytes = qsci->text().toUtf8();
    QByteArray result;
    for (int pos = 0; pos < bytes.size(); ++pos) {
      if (qsci->SendScintilla(QsciScintilla::SCI_INDICATORALLONFOR, pos)) result += bytes.at(pos);
    }
    return result;
  };

  qsci->setCursorPosition(0, 1);  // Before the opening brace, after a multibyte character.
  setMode(BraceMatchingMode::off);
  QCOMPARE(qsci->braceMatching(), QsciScintilla::NoBraceMatch);
  QCOMPARE(highlighted(), QByteArray());
  setMode(BraceMatchingMode::braces);
  QCOMPARE(qsci->braceMatching(), QsciScintilla::SloppyBraceMatch);
  QCOMPARE(highlighted(), QByteArray());
  if (QSCINTILLA_VERSION < 0x020b02) QSKIP("Block highlighting requires QScintilla 2.11.2");
  setMode(BraceMatchingMode::block);
  QCOMPARE(qsci->braceMatching(), QsciScintilla::SloppyBraceMatch);
  QTRY_COMPARE(highlighted(), QByteArray("\n  [10,20]\n"));
  setMode(BraceMatchingMode::off);
  QCOMPARE(qsci->braceMatching(), QsciScintilla::NoBraceMatch);
  QTRY_COMPARE(highlighted(), QByteArray());
  setMode(BraceMatchingMode::block);
  QTRY_COMPARE(highlighted(), QByteArray("\n  [10,20]\n"));
  setMode(BraceMatchingMode::braces);
  QTRY_COMPARE(highlighted(), QByteArray());
  setMode(BraceMatchingMode::block);
  qsci->setCursorPosition(1, 9);  // After the inner closing bracket.
  QTRY_COMPARE(highlighted(), QByteArray("10,20"));
  QVERIFY(!qsci->hasSelectedText());
  qsci->setCursorPosition(0, 2);
  QTRY_COMPARE(highlighted(), QByteArray("\n  [10,20]\n"));
  qsci->insertAt("}", 1, 0);  // Changes the match without moving the caret.
  QTRY_COMPARE(highlighted(), QByteArray("\n"));
  qsci->setCursorPosition(2, 1);  // The original closing brace is now unmatched.
  QTRY_COMPARE(highlighted(), QByteArray());

  // Scrolling styles the comment and can change the match without an edit or caret movement.
  const QByteArray block = "\n" + QByteArray("  cube();\n").repeated(200) + "// }\n";
  qsci->setText(QString::fromLatin1("{" + block + "}"));
  qsci->setCursorPosition(0, 1);
  QTRY_VERIFY(!highlighted().isEmpty());
  qsci->setFirstVisibleLine(200);
  QTRY_COMPARE(highlighted(), block);
}

void TestMainWindow::invalidBraceBlockTintUsesFallback()
{
  if (QSCINTILLA_VERSION < 0x020b02) QSKIP("Block highlighting requires QScintilla 2.11.2");
  ScintillaEditor editor(nullptr);
  const auto base = EditorColorMap::inst()->getColorScheme("For Light Background");
  QVERIFY(base);
  auto theme = base->propertyTree();
  constexpr int indicator = 13;  // Brace block indicator.
  const auto setting = [&](unsigned int message) {
    return editor.qsci->SendScintilla(message, indicator);
  };

  theme.get_child("colors").erase("brace-block-indicator");
  editor.setColormap(EditorColorScheme("Missing tint", 0, theme));
  const auto fallback = setting(QsciScintilla::SCI_INDICGETFORE);
  const auto alpha = setting(QsciScintilla::SCI_INDICGETALPHA);
  QVERIFY(alpha > 0 && alpha < 255);

  theme.put("colors.brace-block-indicator", "#70c7f6cb");
  editor.setColormap(EditorColorScheme("Valid tint", 0, theme));
  QCOMPARE(setting(QsciScintilla::SCI_INDICGETALPHA), 0x70L);

  theme.put("colors.brace-block-indicator", "#70c7f6bg");
  editor.setColormap(EditorColorScheme("Mistyped tint", 0, theme));
  QCOMPARE(setting(QsciScintilla::SCI_INDICGETFORE), fallback);
  QCOMPARE(setting(QsciScintilla::SCI_INDICGETALPHA), alpha);
  QCOMPARE(setting(QsciScintilla::SCI_INDICGETOUTLINEALPHA), alpha);
}
