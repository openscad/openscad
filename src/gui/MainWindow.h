#pragma once

#include "Editor.h"
#include "Geometry.h"
#include "export.h"
#include "ExportPdfDialog.h"
#include "memory.h"
#include "RenderStatistic.h"
#include "TabManager.h"
#include "Tree.h"
#include "UIUtils.h"
#include "qtgettext.h" // IWYU pragma: keep
#include "ui_MainWindow.h"

#include <memory>
#include <string>
#include <vector>
#include <QMainWindow>
#include <QElapsedTimer>
#include <QIcon>
#include <QIODevice>
#include <QMutex>
#include <QTime>

#ifdef STATIC_QT_SVG_PLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QSvgPlugin)
#endif

class BuiltinContext;
class CGALWorker;
class CSGNode;
class CSGProducts;
class FontListDialog;
class LibraryInfoDialog;
class Preferences;
class ProgressWidget;
class ThrownTogetherRenderer;

class MainWindow : public QMainWindow, public Ui::MainWindow, public InputEventHandler
{
  Q_OBJECT

public:
  Preferences *prefs;

  QTimer *consoleUpdater;

  bool is_preview;

  QTimer *autoReloadTimer;
  QTimer *waitAfterReloadTimer;
  RenderStatistic renderStatistic;

  SourceFile *root_file; // Result of parsing
  SourceFile *parsed_file; // Last parse for include list
  std::shared_ptr<AbstractNode> absolute_root_node; // Result of tree evaluation
  std::shared_ptr<AbstractNode> root_node; // Root if the root modifier (!) is used
  Tree tree;
  EditorInterface *activeEditor;
  TabManager *tabManager;

#ifdef ENABLE_CGAL
  shared_ptr<const Geometry> root_geom;
  class CGALRenderer *cgalRenderer;
#endif
#ifdef ENABLE_OPENCSG
  class OpenCSGRenderer *opencsgRenderer;
  std::unique_ptr<class MouseSelector> selector;
#endif
  ThrownTogetherRenderer *thrownTogetherRenderer;

  QString last_compiled_doc;

  QAction *actionRecentFile[UIUtils::maxRecentFiles];
  QMap<QString, QString> knownFileExtensions;

  QLabel *versionLabel;
  QWidget *editorDockTitleWidget;
  QWidget *consoleDockTitleWidget;
  QWidget *parameterDockTitleWidget;
  QWidget *errorLogDockTitleWidget;
  QWidget *animateDockTitleWidget;
  QWidget *viewportControlTitleWidget;

  int compileErrors;
  int compileWarnings;

  MainWindow(const QStringList& filenames);
  ~MainWindow() override;

private:
  volatile bool isClosing = false;
  void consoleOutputRaw(const QString& msg);

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void setTabToolBarVisible(int);
  void updateUndockMode(bool undockMode);
  void updateReorderMode(bool reorderMode);
  void setFont(const QString& family, uint size);
  void setColorScheme(const QString& cs);
  void showProgress();
  void openCSGSettingsChanged();
  void consoleOutput(const Message& msgObj);
  void setCursor();
  void errorLogOutput(const Message& log_msg);

public:
  static void consoleOutput(const Message& msgObj, void *userdata);
  static void errorLogOutput(const Message& log_msg, void *userdata);
  static void noOutputConsole(const Message&, void *) {} // /dev/null
  static void noOutputErrorLog(const Message&, void *) {} // /dev/null

  bool fileChangedOnDisk();
  void parseTopLevelDocument();
  void exceptionCleanup();
  void setLastFocus(QWidget *widget);
  void UnknownExceptionCleanup(std::string msg = "");

  bool isLightTheme();

private:
  void initActionIcon(QAction *action, const char *darkResource, const char *lightResource);
  void setRenderVariables(ContextHandle<BuiltinContext>& context);
  void updateCompileResult();
  void compile(bool reload, bool forcedone = false);
  void compileCSG();
  bool checkEditorModified();
  QString dumpCSGTree(const std::shared_ptr<AbstractNode>& root);

  void loadViewSettings();
  void loadDesignSettings();
  void prepareCompile(const char *afterCompileSlot, bool procevents, bool preview);
  void updateWindowSettings(bool console, bool editor, bool customizer, bool errorLog, bool editorToolbar, bool viewToolbar, bool animate, bool ViewportControlWidget);
  void saveBackup();
  void writeBackup(QFile *file);
  void show_examples();
  void setDockWidgetTitle(QDockWidget *dockWidget, QString prefix, bool topLevel);
  void addKeyboardShortCut(const QList<QAction *>& actions);
  void updateStatusBar(ProgressWidget *progressWidget);
  void activateWindow(int);

  LibraryInfoDialog *library_info_dialog{nullptr};
  FontListDialog *font_list_dialog{nullptr};

public slots:
  void updateExportActions();
  void updateRecentFiles(const QString& FileSavedOrOpened);
  void updateRecentFileActions();
  void handleFileDrop(const QUrl& url);

private slots:
  void actionOpen();
  void actionNewWindow();
  void actionOpenWindow();
  void actionOpenRecent();
  void actionOpenExample();
  void clearRecentFiles();
  void actionSave();
  void actionSaveAs();
  void actionSaveACopy();
  void actionReload();
  void actionShowLibraryFolder();
  void convertTabsToSpaces();
  void copyText();

  void instantiateRoot();
  void compileDone(bool didchange);
  void compileEnded();
  void changeParameterWidget();

private slots:
  void copyViewportTranslation();
  void copyViewportRotation();
  void copyViewportDistance();
  void copyViewportFov();
  void preferences();
  void hideEditorToolbar();
  void hide3DViewToolbar();
  void showLink(const QString&);
  void showEditor();
  void hideEditor();
  void showConsole();
  void hideConsole();
  void showErrorLog();
  void hideErrorLog();
  void showViewportControl();
  void hideViewportControl();
  void showParameters();
  void hideParameters();
  void showAnimate();
  void hideAnimate();
  void on_windowActionSelectEditor_triggered();
  void on_windowActionSelectConsole_triggered();
  void on_windowActionSelectCustomizer_triggered();
  void on_windowActionSelectErrorLog_triggered();
  void on_windowActionSelectAnimate_triggered();
  void on_windowActionSelectViewportControl_triggered();
  void on_windowActionNextWindow_triggered();
  void on_windowActionPreviousWindow_triggered();
  void on_editActionInsertTemplate_triggered();
  void on_editActionFoldAll_triggered();

public slots:
  void hideFind();
  void showFind();
  void showFindAndReplace();

private slots:
  void selectFindType(int);
  void findString(const QString&);
  void findNext();
  void findPrev();
  void useSelectionForFind();
  void replace();
  void replaceAll();

  // Mac OSX FindBuffer support
  void findBufferChanged();
  void updateFindBuffer(const QString&);
  bool event(QEvent *event) override;
protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
  void actionRenderPreview();
private slots:
  void csgRender();
  void csgReloadRender();
  void action3DPrint();
  void sendToOctoPrint();
  void sendToPrintService();
#ifdef ENABLE_CGAL
  void actionRender();
  void actionRenderDone(const shared_ptr<const Geometry>&);
  void cgalRender();
#endif
  void actionCheckValidity();
  void actionDisplayAST();
  void actionDisplayCSGTree();
  void actionDisplayCSGProducts();
  bool canExport(unsigned int dim);
  void actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim);
  void actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim, ExportPdfOptions *options);
  void actionExportSTL();
  void actionExport3MF();
  void actionExportOBJ();
  void actionExportOFF();
  void actionExportWRL();
  void actionExportAMF();
  void actionExportDXF();
  void actionExportSVG();
  void actionExportPDF();
  void actionExportCSG();
  void actionExportImage();
  void actionCopyViewport();
  void actionFlushCaches();

public:
  void viewModeActionsUncheck();
  void setCurrentOutput();
  void clearCurrentOutput();
  void hideCurrentOutput();
  bool isEmpty();

  void onAxisChanged(InputEventAxisChanged *event) override;
  void onButtonChanged(InputEventButtonChanged *event) override;

  void onTranslateEvent(InputEventTranslate *event) override;
  void onRotateEvent(InputEventRotate *event) override;
  void onRotate2Event(InputEventRotate2 *event) override;
  void onActionEvent(InputEventAction *event) override;
  void onZoomEvent(InputEventZoom *event) override;

  void changedTopLevelConsole(bool);
  void changedTopLevelEditor(bool);
  void changedTopLevelErrorLog(bool);
  void changedTopLevelAnimate(bool);
  void changedTopLevelViewportControl(bool);

  QList<double> getTranslation() const;
  QList<double> getRotation() const;

public slots:
  void actionReloadRenderPreview();
  void on_editorDock_visibilityChanged(bool);
  void on_consoleDock_visibilityChanged(bool);
  void on_parameterDock_visibilityChanged(bool);
  void on_errorLogDock_visibilityChanged(bool);
  void on_animateDock_visibilityChanged(bool);
  void on_viewportControlDock_visibilityChanged(bool);
  void on_toolButtonCompileResultClose_clicked();
  void editorTopLevelChanged(bool);
  void consoleTopLevelChanged(bool);
  void parameterTopLevelChanged(bool);
  void errorLogTopLevelChanged(bool);
  void animateTopLevelChanged(bool);
  void viewportControlTopLevelChanged(bool);
  void processEvents();
  void jumpToLine(int, int);
  void openFileFromPath(const QString&, int);

#ifdef ENABLE_OPENCSG
  void viewModePreview();
#endif
#ifdef ENABLE_CGAL
  void viewModeSurface();
  void viewModeWireframe();
#endif
  void viewModeThrownTogether();
  void viewModeShowEdges();
  void viewModeShowAxes();
  void viewModeShowCrosshairs();
  void viewModeShowScaleProportional();
  void viewAngleTop();
  void viewAngleBottom();
  void viewAngleLeft();
  void viewAngleRight();
  void viewAngleFront();
  void viewAngleBack();
  void viewAngleDiagonal();
  void viewCenter();
  void viewPerspective();
  void viewOrthogonal();
  void viewTogglePerspective();
  void viewResetView();
  void viewAll();
  void editorContentChanged();
  void selectObject(QPoint coordinate);
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void helpAbout();
  void helpHomepage();
  void helpManual();
  void helpOfflineManual();
  void helpCheatSheet();
  void helpOfflineCheatSheet();
  void helpLibrary();
  void helpFontInfo();
  void quit();
  void checkAutoReload();
  void waitAfterReload();
  void autoReloadSet(bool);

private:
  bool network_progress_func(const double permille);
  static void report_func(const std::shared_ptr<const AbstractNode>&, void *vp, int mark);
  static bool undockMode;
  static bool reorderMode;
  static const int tabStopWidth;
  static QElapsedTimer *progressThrottle;
  QWidget *lastFocus; // keep track of active copyable widget (Editor|Console) for global menu action Edit->Copy

  shared_ptr<CSGNode> csgRoot; // Result of the CSGTreeEvaluator
  shared_ptr<CSGNode> normalizedRoot; // Normalized CSG tree
  shared_ptr<CSGProducts> root_products;
  shared_ptr<CSGProducts> highlights_products;
  shared_ptr<CSGProducts> background_products;

  char const *afterCompileSlot;
  bool procevents{false};
  QTemporaryFile *tempFile{nullptr};
  ProgressWidget *progresswidget{nullptr};
  CGALWorker *cgalworker;
  QMutex consolemutex;
  EditorInterface *renderedEditor; // stores pointer to editor which has been most recently rendered
  time_t includes_mtime{0}; // latest include mod time
  time_t deps_mtime{0}; // latest dependency mod time
  std::unordered_map<std::string, QString> export_paths; // for each file type, where it was exported to last
  QString exportPath(const char *suffix); // look up the last export path and generate one if not found
  int last_parser_error_pos{-1}; // last highlighted error position
  int tabCount = 0;
  paperSizes sizeString2Enum(QString current);
  paperOrientations orientationsString2Enum(QString current);
  
signals:
  void highlightError(int);
  void unhighlightLastError();
};

class GuiLocker
{
public:
  GuiLocker() {
    GuiLocker::lock();
  }
  ~GuiLocker() {
    GuiLocker::unlock();
  }
  static bool isLocked() { return gui_locked > 0; }
  static void lock() {
    gui_locked++;
  }
  static void unlock() {
    gui_locked--;
  }

private:
  static unsigned int gui_locked;
};
