#pragma once

#include <ctime>
#include <tuple>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include <QAction>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#include <QMainWindow>
#include <QElapsedTimer>
#include <QIcon>
#include <QIODevice>
#include <QMutex>
#include <QSoundEffect>
#include <QTime>
#include <QSignalMapper>
#include <QShortcut>
#include "core/Context.h"
#include "glview/Renderer.h"
#include "core/SourceFile.h"
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

#include "core/Tree.h"
#include "geometry/Geometry.h"
#include "gui/Editor.h"
#include "gui/input/InputDriverEvent.h"
#include "gui/Measurement.h"
#include "gui/qt-obsolete.h"  // IWYU pragma: keep
#include "gui/qtgettext.h"    // IWYU pragma: keep
#include "gui/RubberBandManager.h"
#include "gui/TabManager.h"
#include "gui/UIUtils.h"
#include "io/export_enums.h"
#include "io/export.h"
#include "io/export.h"
#include "RenderStatistic.h"
#include "ui_MainWindow.h"
#include "utils/printutils.h"

class UXTest;
class MainWindow : public QMainWindow, public Ui::MainWindow, public InputEventHandler
{
  Q_OBJECT

  friend UXTest;

public:
  Preferences *prefs;

  QTimer *consoleUpdater;

  bool isPreview;

  QTimer *autoReloadTimer;
  QTimer *waitAfterReloadTimer;
  RenderStatistic renderStatistic;

  std::shared_ptr<SourceFile> rootFile;            // Result of parsing
  std::shared_ptr<SourceFile> parsedFile;          // Last parse for include list
  std::shared_ptr<AbstractNode> absoluteRootNode;  // Result of tree evaluation
  std::shared_ptr<AbstractNode> rootNode;          // Root if the root modifier (!) is used
#ifdef ENABLE_PYTHON
  bool python_active;
  std::string trusted_edit_document_name;
  std::string untrusted_edit_document_name;
  bool trust_python_file(const std::string& file, const std::string& content);
#endif
  Tree tree;
  EditorInterface *activeEditor;
  TabManager *tabManager;

  std::shared_ptr<const Geometry> rootGeom;
  std::shared_ptr<Renderer> geomRenderer;
#ifdef ENABLE_OPENCSG
  std::shared_ptr<Renderer> previewRenderer;
#endif
  std::shared_ptr<Renderer> thrownTogetherRenderer;

  QString lastCompiledDoc;

  QAction *actionRecentFile[UIUtils::maxRecentFiles];
  QShortcut *shortcutNextWindow{nullptr};
  QShortcut *shortcutPreviousWindow{nullptr};

  QLabel *versionLabel;

  Measurement meas;

  int compileErrors;
  int compileWarnings;

  MainWindow(const QStringList& filenames);
  ~MainWindow() override;

private:
  RubberBandManager rubberBandManager;

  std::vector<std::tuple<Dock *, QString, QString>> docks;

  volatile bool isClosing = false;
  void consoleOutputRaw(const QString& msg);
  void clearAllSelectionIndicators();
  void setSelectionIndicatorStatus(EditorInterface *editor, int nodeIndex,
                                   EditorSelectionIndicatorStatus status);

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void updateUndockMode(bool undockMode);
  void updateReorderMode(bool reorderMode);
  void setFont(const QString& family, uint size);
  void setColorScheme(const QString& cs);
  void showProgress();
  void openCSGSettingsChanged();
  void consoleOutput(const Message& msgObj);
  void setSelection(int index);

  // implements the actions to be done when the selection menu is closing
  // the seclection menu is the one that show up when right click on the geometry in the 3d view.
  void onHoveredObjectInSelectionMenu();

  void measureFinished();
  void errorLogOutput(const Message& log_msg);
  void onNavigationOpenContextMenu();
  void onNavigationCloseContextMenu();
  void onNavigationHoveredContextMenuEntry();
  void onNavigationTriggerContextMenuEntry();
  void setAllMouseViewActions();

  // implement the different actions needed when
  // the tab manager editor is changed.
  void onTabManagerEditorChanged(EditorInterface *);

  // implement the different actions needed when
  // the tab manager editor is about to close one of the tab
  void onTabManagerAboutToCloseEditor(EditorInterface *);

  // implement the different actions needed when an editor
  // has its content replaced (because of load)
  void onTabManagerEditorContentReloaded(EditorInterface *reloadedEditor);

public:
  static void consoleOutput(const Message& msgObj, void *userdata);
  static void errorLogOutput(const Message& log_msg, void *userdata);
  static void noOutputConsole(const Message&, void *) {}   // /dev/null
  static void noOutputErrorLog(const Message&, void *) {}  // /dev/null

  bool fileChangedOnDisk();

  // Parse the document contained in the editor, update the editors's parameters and returns a SourceFile
  // object if parsing suceeded. Nullptr otherwise.
  std::shared_ptr<SourceFile> parseDocument(EditorInterface *editor);

  void parseTopLevelDocument();
  void exceptionCleanup();
  void setLastFocus(QWidget *widget);
  void UnknownExceptionCleanup(std::string msg = "");
  void showFind(bool doFindAndReplace);

private:
  [[nodiscard]] QString getCurrentFileName() const;

  void setRenderVariables(ContextHandle<BuiltinContext>& context);
  void updateCompileResult();
  void compile(bool reload, bool forcedone = false);
  void compileCSG();
  bool checkEditorModified();
  QString dumpCSGTree(const std::shared_ptr<AbstractNode>& root);

  // Opens an independent windows with a text area showing the text given in argument
  // The "type" is used to specify the type of content with the title of the window,
  void showTextInWindow(const QString& type, const QString& textToShow);

  // Change the perspective mode of the 3D view.
  typedef Camera::ProjectionType ProjectionType;
  void setProjectionType(ProjectionType mode);

  void loadViewSettings();
  void loadDesignSettings();
  void prepareCompile(const char *afterCompileSlot, bool procevents, bool preview);
  void updateWindowSettings(bool isEditorToolbarVisible, bool isViewToolbarVisible);
  void saveBackup();
  void writeBackup(QFile *file);
  void show_examples();
  void addKeyboardShortCut(const QList<QAction *>& actions);
  void updateStatusBar(ProgressWidget *progressWidget);
  void activateDock(Dock *);
  Dock *findVisibleDockToActivate(int offset) const;
  Dock *getNextDockFromSender(QObject *sender);
  void addExportActions(QToolBar *toolbar, QAction *action) const;
  QAction *formatIdentifierToAction(const std::string& identifier) const;

  LibraryInfoDialog *libraryInfoDialog{nullptr};
  FontListDialog *fontListDialog{nullptr};
  QSignalMapper *exportFormatMapper;

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
  void actionPythonRevokeTrustedFiles();
  void actionPythonCreateVenv();
  void actionPythonSelectVenv();
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

  // Handle the Next/Prev dock menu action when the is hovered, currently this activate the rubberband
  void onWindowActionNextPrevHovered();

  // Handle the Next/Prev dock menu action when the is validatee, currently switch to the targetted dock
  // and remove the rubberband
  void onWindowActionNextPrevTriggered();

  // Handle the Next/Prev shortcut, currently switch to the targetted dock
  // and adds the rubberband, the rubbreband is removed on shortcut key release.
  void onWindowShortcutNextPrevActivated();
  void onWindowShortcutExport3DActivated();

  void onEditorDockVisibilityChanged(bool isVisible);
  void onConsoleDockVisibilityChanged(bool isVisible);
  void onErrorLogDockVisibilityChanged(bool isVisible);
  void onAnimateDockVisibilityChanged(bool isVisible);
  void onFontListDockVisibilityChanged(bool isVisible);
  void onColorListDockVisibilityChanged(bool isVisible);
  void onViewportControlDockVisibilityChanged(bool isVisible);
  void onParametersDockVisibilityChanged(bool isVisible);

  void onColorListColorSelected(const QString&);

  void on_editActionInsertTemplate_triggered();
  void on_editActionFoldAll_triggered();

public slots:
  void hideFind();
  void actionShowFind();
  void actionShowFindAndReplace();

private slots:
  void actionSelectFind(int);
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
  void sendToExternalTool(class ExternalToolInterface& externalToolService);
  void actionRender();
  void actionRenderDone(const std::shared_ptr<const Geometry>&);
  void cgalRender();
  void handleMeasurementClicked(QAction *clickedAction);
  void actionCheckValidity();
  void actionDisplayAST();
  void actionDisplayCSGTree();
  void actionDisplayCSGProducts();
  bool canExport(unsigned int dim);
  void actionExport(unsigned int dim, ExportInfo& exportInfo);
  void actionExportFileFormat(int fmt);
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

  QList<double> getTranslation() const;
  QList<double> getRotation() const;
  std::unordered_map<FileFormat, QAction *> exportMap;

public slots:
  void actionReloadRenderPreview();
  void on_toolButtonCompileResultClose_clicked();
  void processEvents();
  void jumpToLine(int, int);
  void openFileFromPath(const QString&, int);

  void viewModeRender();
#ifdef ENABLE_OPENCSG
  void viewModePreview();
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
  void leftClick(QPoint coordinate);
  void rightClick(QPoint coordinate);
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void helpAbout();
  void helpHomepage();
  void helpManual();
  void helpOfflineManual();
  void helpCheatSheet();
  void helpOfflineCheatSheet();
  void helpLibrary();
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
  QWidget *lastFocus;  // keep track of active copyable widget (Editor|Console) for global menu action
                       // Edit->Copy

  std::shared_ptr<CSGNode> csgRoot;         // Result of the CSGTreeEvaluator
  std::shared_ptr<CSGNode> normalizedRoot;  // Normalized CSG tree
  std::shared_ptr<CSGProducts> rootProduct;
  std::shared_ptr<CSGProducts> highlightsProducts;
  std::shared_ptr<CSGProducts> backgroundProducts;
  int currentlySelectedObject{-1};

  char const *afterCompileSlot;
  bool procevents{false};
  QTemporaryFile *tempFile{nullptr};
  ProgressWidget *progresswidget{nullptr};
  CGALWorker *cgalworker;
  QMutex consolemutex;
  EditorInterface *renderedEditor;  // stores pointer to editor which has been most recently rendered
  time_t includesMTime{0};          // latest include mod time
  time_t depsMTime{0};              // latest dependency mod time
  std::unordered_map<QString, QString> exportPaths;  // for each file type, where it was exported to last
  QString exportPath(
    const QString& suffix);    // look up the last export path and generate one if not found
  int lastParserErrorPos{-1};  // last highlighted error position
  int tabCount = 0;
  ExportPdfPaperSize sizeString2Enum(const QString& current);
  ExportPdfPaperOrientation orientationsString2Enum(const QString& current);

  QMenu *navigationMenu{nullptr};
  QSoundEffect *renderCompleteSoundEffect;
  std::vector<std::unique_ptr<QTemporaryFile>> allTempFiles;

  void resetMeasurementsState(bool enable, const QString& tooltipMessage);
  QActionGroup *measurementGroup;
  QAction *activeMeasurement = nullptr;

signals:
  void highlightError(int);
  void unhighlightLastError();

#ifdef ENABLE_GUI_TESTS
public:
  std::shared_ptr<AbstractNode> instantiateRootFromSource(SourceFile *file);
signals:
  // This is a new signal introduced while drafting the testing framework, while in experimental mode
  // we protected it using the #ifdef/endif so it should not be considered as part of the MainWindow API.
  void compilationDone(SourceFile *);
#endif  //
};

class GuiLocker
{
public:
  GuiLocker() { GuiLocker::lock(); }
  ~GuiLocker() { GuiLocker::unlock(); }
  static bool isLocked() { return guiLocked > 0; }
  static void lock() { guiLocked++; }
  static void unlock() { guiLocked--; }

private:
  static unsigned int guiLocked;
};
