#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <QObject>
#include <QSet>
#include "gui/Editor.h"
#include "gui/TabWidget.h"

class MainWindow; // for circular dependency

class TabManager : public QObject
{
  Q_OBJECT

public:
  TabManager(MainWindow *o, const QString& filename);
  QWidget *getTabHeader();
  QWidget *getTabContent();
  EditorInterface *editor;
  QSet<EditorInterface *> editorList;

  void createTab(const QString& filename);
  void openTabFile(const QString& filename);
  void setTabName(const QString& filename, EditorInterface *edt = nullptr);
  bool refreshDocument(); // returns false if the file could not be opened
  bool shouldClose();
  bool save(EditorInterface *edt);
  bool saveAs(EditorInterface *edt);
  bool saveACopy(EditorInterface *edt);
  void open(const QString& filename);
  size_t count();

public:
  static constexpr const int FIND_HIDDEN = 0;
  static constexpr const int FIND_VISIBLE = 1;
  static constexpr const int FIND_REPLACE_VISIBLE = 2;

signals:
  void tabCountChanged(int);

private:
  MainWindow *par;
  TabWidget *tabWidget;

  bool maybeSave(int);
  bool save(EditorInterface *edt, const QString& path);
  void saveError(const QIODevice& file, const std::string& msg, const QString& filepath);
  void applyAction(QObject *object, const std::function<void(int, EditorInterface *)>& func);

private slots:
  void tabSwitched(int);
  void closeTabRequested(int);
  void middleMouseClicked(int);

private slots:
  void highlightError(int);
  void unhighlightLastError();
  void undo();
  void redo();
  void cut();
  void paste();
  void indentSelection();
  void unindentSelection();
  void commentSelection();
  void uncommentSelection();
  void updateActionUndoState();
  void toggleBookmark();
  void nextBookmark();
  void prevBookmark();
  void jumpToNextError();
  void copyFileName();
  void copyFilePath();
  void openFolder();
  void closeTab();
  void showContextMenuEvent(const QPoint&);
  void showTabHeaderContextMenu(const QPoint&);

  void stopAnimation();
  void updateFindState();

  void onHyperlinkIndicatorClicked(int pos);

public slots:
  void actionNew();
  void copy();
  void setContentRenderState(); // since last render
  void setTabModified(EditorInterface *);
  bool saveAll();
  void closeCurrentTab();
  void nextTab();
  void prevTab();
  void setFocus();
};
