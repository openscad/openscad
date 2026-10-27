#include "gui/ScintillaEditor.h"

#include <QApplication>
#include <QKeyEvent>
#include <QStringList>
#include <catch2/catch_all.hpp>

#ifdef Q_OS_MACOS
namespace {

QApplication& editorTestApplication()
{
  static int argc = 1;
  static char name[] = "OpenSCADEditorTests";
  static char *argv[] = {name, nullptr};
  static QApplication app(argc, argv);
  return app;
}

void sendEditorKey(QWidget *target, int key, Qt::KeyboardModifiers modifiers,
                   const QString& text = QString())
{
  QKeyEvent press(QEvent::KeyPress, key, modifiers, text);
  QApplication::sendEvent(target, &press);
  QKeyEvent release(QEvent::KeyRelease, key, modifiers, text);
  QApplication::sendEvent(target, &release);
}

}  // namespace

TEST_CASE("Mac Command Shift P toggles line emphasis before requesting preview", "[editor-shortcuts]")
{
  editorTestApplication();
  ScintillaEditor editor(nullptr);
  const QString original = "sphere(1);\n\t  cube(2);\ncylinder(3);\n";
  const QString highlighted = "sphere(1);\n\t  #cube(2);\ncylinder(3);\n";
  editor.setPlainText(original);
  editor.setCursorPosition(1, 6);

  QStringList previews;
  QObject::connect(&editor, &ScintillaEditor::previewRequest, &editor,
                   [&]() { previews.append(editor.toPlainText()); });

  // On macOS Qt maps the physical Command key to ControlModifier.
  sendEditorKey(editor.qsci, Qt::Key_P, Qt::ControlModifier | Qt::ShiftModifier, "P");
  CHECK(editor.toPlainText().toStdString() == highlighted.toStdString());
  REQUIRE(previews.size() == 1);
  CHECK(previews.at(0).toStdString() == highlighted.toStdString());

  sendEditorKey(editor.qsci, Qt::Key_P, Qt::ControlModifier | Qt::ShiftModifier, "P");
  CHECK(editor.toPlainText().toStdString() == original.toStdString());
  REQUIRE(previews.size() == 2);
  CHECK(previews.at(1).toStdString() == original.toStdString());
}

TEST_CASE("Mac physical Control Shift P does not toggle line emphasis", "[editor-shortcuts]")
{
  editorTestApplication();
  ScintillaEditor editor(nullptr);
  const QString original = "cube(2);\n";
  editor.setPlainText(original);
  editor.setCursorPosition(0, 2);
  int previews = 0;
  QObject::connect(&editor, &ScintillaEditor::previewRequest, &editor, [&]() { ++previews; });

  // MetaModifier is the physical Control key, not Command, on macOS.
  sendEditorKey(editor.qsci, Qt::Key_P, Qt::MetaModifier | Qt::ShiftModifier);
  CHECK(editor.toPlainText().toStdString() == original.toStdString());
  CHECK(previews == 0);
}

TEST_CASE("Mac Option Command L formats without inserting the Option character", "[editor-shortcuts]")
{
  editorTestApplication();
  ScintillaEditor editor(nullptr);
  editor.qsci->setIndentationWidth(2);
  editor.qsci->setIndentationsUseTabs(false);
  editor.setPlainText("union() {\ncube(2);\n}\n");
  editor.setCursorPosition(1, 2);

  sendEditorKey(editor.qsci, Qt::Key_L, Qt::ControlModifier | Qt::AltModifier, "@");
  CHECK(editor.toPlainText().toStdString() == "union() {\n  cube(2);\n}\n");
}

TEST_CASE("Option arrows request preview object navigation without editing code", "[preview-navigation]")
{
  editorTestApplication();
  ScintillaEditor editor(nullptr);
  const QString original = "translate([1, 2, 3]) cube(2);\n";
  editor.setPlainText(original);
  editor.selectRange(0, 21, 0, 29);
  CHECK(editor.selectedText().toStdString() == "cube(2);");
  CHECK_FALSE(editor.isContentModified());
  std::vector<int> directions;
  QObject::connect(&editor, &ScintillaEditor::previewObjectNavigationRequest, &editor,
                   [&](int direction) { directions.push_back(direction); });
  int previews = 0;
  QObject::connect(&editor, &ScintillaEditor::previewRequest, &editor, [&]() { ++previews; });

  sendEditorKey(editor.qsci, Qt::Key_Down, Qt::AltModifier | Qt::KeypadModifier);
  sendEditorKey(editor.qsci, Qt::Key_Up, Qt::AltModifier | Qt::KeypadModifier);
  const std::vector<int> expectedDirections{1, -1};
  CHECK(directions == expectedDirections);
  CHECK(editor.toPlainText().toStdString() == original.toStdString());
  CHECK_FALSE(editor.isContentModified());
  CHECK(previews == 0);
  editor.clearSelection();
  CHECK(editor.selectedText().isEmpty());
}
#endif
