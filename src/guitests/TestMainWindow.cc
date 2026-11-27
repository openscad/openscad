#include "TestMainWindow.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QTest>

#include "gui/ColorLabel.h"
#include "gui/ColorList.h"
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
  if (QSCINTILLA_VERSION < 0x020b02) QSKIP("Requires QScintilla's public brace matcher");
  ScintillaEditor editor(nullptr);
  auto *qsci = editor.qsci;
  qsci->setBraceMatching(QsciScintilla::SloppyBraceMatch);
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
  QTRY_COMPARE(highlighted(), QByteArray("\n  [10,20]\n"));
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
