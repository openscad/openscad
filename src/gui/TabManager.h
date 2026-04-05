#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <tuple>

#include "gui/Editor.h"

class MainWindow;  // for circular dependency

class TabManager : public QObject
{
  Q_OBJECT

public:
  TabManager(MainWindow *o, const QString& filename);
  QWidget *getTabContent();
  EditorInterface *editor;
  QSet<EditorInterface *> editorList;

  /// When \p filename is empty and \p initializeEmptyEditor is true, apply the default new-tab
  /// content (e.g. Python template). Session restore passes \c false so \c setTabSessionData
  /// supplies text without an intermediate parse.
  void createTab(const QString& filename, bool initializeEmptyEditor = true);
  void openTabFile(const QString& filename);

  // returns the name and tooltip of the tab for the given provided editor
  // if there is a path associated with an editor this is the filepath
  // otherwise Untitled.scad
  std::tuple<QString, QString> getEditorTabName(EditorInterface *edt);

  // returns the name and tooltip of the tab for the given provided editor with
  // the extra symbols used to indicate the file has changed.
  std::tuple<QString, QString> getEditorTabNameWithModifier(EditorInterface *edt);

  void setEditorTabName(const QString& tabName, const QString& tabTooltip,
                        EditorInterface *edt = nullptr);
  void updateTabIcon(EditorInterface *edt);
  bool refreshDocument();  // returns false if the file could not be opened
  bool shouldClose();
  bool save(EditorInterface *edt);
  bool saveAs(EditorInterface *edt);
  bool saveAs(EditorInterface *edt, const QString& filepath);
  bool saveACopy(EditorInterface *edt);
  void open(const QString& filename);
  size_t count();
  void switchToEditor(EditorInterface *editor);

  void saveSession(const QString& path);
  bool restoreSession(const QString& path, int windowIndex = 0);
  static bool saveGlobalSession(const QString& path, QString *error = nullptr, bool showWarning = true);
  static int sessionWindowCount(const QString& path);
  /// Index into the session file's \c windows array of the last active main window (0 if unknown).
  static int sessionActiveWindowIndex(const QString& path);
  /// True if \a path describes exactly one window with one tab: no filepath and editor not modified.
  static bool sessionHasOnlyEmptyTab(const QString& path);
  static void removeSessionFile();
  static QString getSessionFilePath();
  static QString getAutosaveFilePath();
  static bool hasDirtyTabs();
  static void bumpSessionDirtyGeneration();
  static uint64_t sessionDirtyGeneration();
  static void setSkipSessionSave(bool skip);
  static bool shouldSkipSessionSave();

  // Session file schema version. Increment when the format changes and add a
  // migration step in migrateSession().  Old files without a version field are
  // treated as version 1.
  static constexpr int SESSION_VERSION = 4;

public:
  static constexpr const int FIND_HIDDEN = 0;
  static constexpr const int FIND_VISIBLE = 1;
  static constexpr const int FIND_REPLACE_VISIBLE = 2;

signals:
  // emitted when the currently displayed editor is changed and a new one is one focus.
  // the passed parameter can be nullptr, when the editor changed because of closing of the last
  // opened on.
  void currentEditorChanged(EditorInterface *editor);
  void editorAboutToClose(EditorInterface *editor);

  void tabCountChanged(int);

  // emitted when the content of an editor is reloaded
  void editorContentReloaded(EditorInterface *editor);

private:
  MainWindow *parent;
  QTabWidget *tabWidget;
  bool use_gvim = false;

  bool maybeSave(int);
  /// Tab title for message boxes (no `&` doubling; see getEditorTabName for QTabWidget labels).
  QString plainEditorTitleForMessages(EditorInterface *edt) const;
  bool save(EditorInterface *edt, const QString& path);
  void saveError(const QIODevice& file, const std::string& msg, const QString& filepath);
  void applyAction(QObject *object, const std::function<void(int, EditorInterface *)>& func);
  void setTabsCloseButtonVisibility(int tabIndice, bool isVisible);
  void setTabSessionData(EditorInterface *edt, const QString& filepath, const QString& content,
                         bool contentModified, bool parameterModified,
                         const QByteArray& customizerState = QByteArray(),
                         std::optional<int> sessionLanguage = std::nullopt);
  static bool migrateSession(QJsonObject& root, int fromVersion);

  enum class SessionFileReadStatus {
    Ok,
    OpenFailed,
    InvalidJson,
    TooNew,
    MigrateFailed,
  };
  /// Read session JSON from disk; on \c Ok, \a outRoot holds the normalized object (migrated).
  static SessionFileReadStatus readSessionFileRoot(const QString& path, QJsonObject *outRoot,
                                                   QString *openError = nullptr,
                                                   int *tooNewVersion = nullptr,
                                                   int *migrateFailedAtVersion = nullptr);

  QTabBar::ButtonPosition getClosingButtonPosition();
  void zoomIn();
  void zoomOut();

private slots:
  void tabSwitched(int);
  void closeTabRequested(int);
  void updateActionUndoState();
  void copyFileName();
  void copyFilePath();
  void openFolder();
  void closeTab();
  void closeAllButThisTab();

  void showContextMenuEvent(const QPoint&);
  void showTabHeaderContextMenu(const QPoint&);

  void stopAnimation();
  void updateFindState();

  void onHyperlinkIndicatorClicked(int pos);

public slots:
  void actionNew();
  void copy();
  void setContentRenderState();  // since last render
  void onTabModified(EditorInterface *);
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
