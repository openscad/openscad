#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <QObject>
#include <QSet>
#include "gui/Editor.h"

class MainWindow; // for circular dependency

class TabManager : public QObject
{
  Q_OBJECT

public:
  TabManager(MainWindow *o);
  QWidget *getTabContent();
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
  EditorInterface* editor() const;

public:
  static constexpr const int FIND_HIDDEN = 0;
  static constexpr const int FIND_VISIBLE = 1;
  static constexpr const int FIND_REPLACE_VISIBLE = 2;

signals:
  // emitted when the currently displayed editor is changed and a new one is one focus.
  // the passed parameter can be nullptr, when the editor changed because of closing of the last
  // opened on.
  void currentEditorChanged(EditorInterface *editor);
  void tabCountChanged(int);

private:
  MainWindow *par;
  QTabWidget *tabWidget;

  bool maybeSave(int);
  bool save(EditorInterface *edt, const QString& path);
  void saveError(const QIODevice& file, const std::string& msg, const QString& filepath);
  void applyAction(QObject *object, const std::function<void(int, EditorInterface *)>& func);
  void setTabsCloseButtonVisibility(int tabIndice, bool isVisible);
  void setCloseButtonForAllTabsExcept(int indice);

  QTabBar::ButtonPosition getClosingButtonPosition();

  // Internal function to factorize the building of menu actions.
  template<class Function>
  QAction* buildMenuAction(const QString& actionText, int idx, bool isEnabled, Function onTriggeredFunction);

private slots:
  void tabSwitched(int);
  void closeTabRequested(int);
  void updateActionUndoState();
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
  void toggleBookmark();
  void nextBookmark();
  void prevBookmark();
  void jumpToNextError();
};
