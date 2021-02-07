#pragma once

#include "qtgettext.h"
#include <QMainWindow>
#include <QIcon>
#include "ui_MainWindow.h"
#include "UIUtils.h"
#include "openscad.h"
#include "builtincontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Tree.h"
#include "memory.h"
#include "editor.h"
#include "export.h"
#include <vector>
#include <QMutex>
#include <QElapsedTimer>
#include <QTime>
#include <QIODevice>
#include "input/InputDriver.h"
#include "editor.h"
#include "tabmanager.h"
#include <memory>

class MouseSelector;

class MainWindow : public QMainWindow, public Ui::MainWindow, public InputEventHandler
{
	Q_OBJECT

public:
	class Preferences *prefs;

	QTimer *consoleUpdater;
	QTimer *animate_timer;
	int anim_step;
	int anim_numsteps;
	double anim_tval;
	bool anim_dumping;
	int anim_dump_start_step;

	QTimer *autoReloadTimer;
	QTimer *waitAfterReloadTimer;
	QTime renderingTime;
	EditorInterface *customizerEditor;

	ContextHandle<BuiltinContext> top_ctx;
	FileModule *root_module;      // Result of parsing
	FileModule *parsed_module;		// Last parse for include list
	ModuleInstantiation root_inst;	// Top level instance
	AbstractNode *absolute_root_node; // Result of tree evaluation
	AbstractNode *root_node;		  // Root if the root modifier (!) is used
	Tree tree;
	EditorInterface *activeEditor;
	TabManager *tabManager;

#ifdef ENABLE_CGAL
	shared_ptr<const class Geometry> root_geom;
	class CGALRenderer *cgalRenderer;
#endif
#ifdef ENABLE_OPENCSG
	class OpenCSGRenderer *opencsgRenderer;
	std::unique_ptr<MouseSelector> selector;
#endif
	class ThrownTogetherRenderer *thrownTogetherRenderer;

	QString last_compiled_doc;

	QAction *actionRecentFile[UIUtils::maxRecentFiles];
	QMap<QString, QString> knownFileExtensions;

	QLabel *versionLabel;
	QWidget *editorDockTitleWidget;
	QWidget *consoleDockTitleWidget;
	QWidget *parameterDockTitleWidget;
	QWidget *errorLogDockTitleWidget;

	int compileErrors;
	int compileWarnings;

	MainWindow(const QStringList &filenames);
	~MainWindow();

private:
	void consoleOutputRaw(const QString& msg);

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void setTabToolBarVisible(int);
	void updatedAnimTval();
	void updatedAnimFps();
	void updatedAnimSteps();
	void updatedAnimDump(bool checked);
	void updateTVal();
	void updateUndockMode(bool undockMode);
	void updateReorderMode(bool reorderMode);
	void setFont(const QString &family, uint size);
	void setColorScheme(const QString &cs);
	void showProgress();
	void openCSGSettingsChanged();
	void consoleOutput(const Message& msgObj);
	void setCursor();
	void errorLogOutput(const Message &log_msg);

public:
	static void consoleOutput(const Message &msgObj, void *userdata);
	static void errorLogOutput(const Message &log_msg, void *userdata);
	static void noOutputConsole(const Message &, void*) {};  // /dev/null
	static void noOutputErrorLog(const Message &, void*) {};  // /dev/null

	bool fileChangedOnDisk();
	void parseTopLevelDocument(bool rebuildParameterWidget);
	void exceptionCleanup();

private:
	void initActionIcon(QAction *action, const char *darkResource, const char *lightResource);
	void updateTemporalVariables();
	void updateCompileResult();
	void compile(bool reload, bool forcedone = false, bool rebuildParameterWidget=true);
	void compileCSG();
	bool checkEditorModified();
	QString dumpCSGTree(AbstractNode *root);

	void loadViewSettings();
	void loadDesignSettings();
	void prepareCompile(const char *afterCompileSlot, bool procevents, bool preview);
    void updateWindowSettings(bool console, bool editor, bool customizer, bool errorLog, bool editorToolbar, bool viewToolbar);
	void saveBackup();
	void writeBackup(class QFile *file);
	void show_examples();
	void setDockWidgetTitle(QDockWidget *dockWidget, QString prefix, bool topLevel);
	void addKeyboardShortCut(const QList<QAction *> &actions);
	void updateStatusBar(class ProgressWidget *progressWidget);
	void activateWindow(int);

  class LibraryInfoDialog* library_info_dialog;
  class FontListDialog *font_list_dialog;

public slots:
	void updateRecentFiles(EditorInterface *edt);
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
	void actionReload();
	void actionShowLibraryFolder();
	void convertTabsToSpaces();

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
	void showLink(const QString);
	void showEditor();
	void hideEditor();
	void showConsole();
	void hideConsole();
	void showErrorLog();
	void hideErrorLog();
	void showParameters();
	void hideParameters();
	void on_windowActionSelectEditor_triggered();
	void on_windowActionSelectConsole_triggered();
	void on_windowActionSelectCustomizer_triggered();
	void on_windowActionSelectErrorLog_triggered();
	void on_windowActionNextWindow_triggered();
	void on_windowActionPreviousWindow_triggered();
	void on_editActionInsertTemplate_triggered();

public slots:
	void hideFind();
	void showFind();
	void showFindAndReplace();

private slots:
	void selectFindType(int);
	void findString(QString);
	void findNext();
	void findPrev();
	void useSelectionForFind();
	void replace();
	void replaceAll();

	// Mac OSX FindBuffer support
	void findBufferChanged();
	void updateFindBuffer(QString);
	bool event(QEvent* event) override;
protected:
	bool eventFilter(QObject* obj, QEvent *event) override;

private slots:
	void actionRenderPreview(bool rebuildParameterWidget=true);
	void csgRender();
	void csgReloadRender();
	void action3DPrint();
	void sendToOctoPrint();
	void sendToPrintService();
#ifdef ENABLE_CGAL
	void actionRender();
	void actionRenderDone(shared_ptr<const class Geometry>);
	void cgalRender();
#endif
	void actionCheckValidity();
	void actionDisplayAST();
	void actionDisplayCSGTree();
	void actionDisplayCSGProducts();
	bool canExport(unsigned int dim);
	void actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim);
	void actionExportSTL();
	void actionExport3MF();
	void actionExportOFF();
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

	QList<double> getTranslation() const;
	QList<double> getRotation() const;

public slots:
	void actionReloadRenderPreview();
	void on_editorDock_visibilityChanged(bool);
	void on_consoleDock_visibilityChanged(bool);
	void on_parameterDock_visibilityChanged(bool);
	void on_errorLogDock_visibilityChanged(bool);
	void on_toolButtonCompileResultClose_clicked();
	void editorTopLevelChanged(bool);
	void consoleTopLevelChanged(bool);
	void parameterTopLevelChanged(bool);
	void errorLogTopLevelChanged(bool);
	void processEvents();
	void jumpToLine(int,int);
	void openFileFromPath(QString,int);

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
	void viewModeAnimate();
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
	void animateUpdateDocChanged();
	void animateUpdate();
	void selectObject(QPoint coordinate);
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void helpAbout();
	void helpHomepage();
	void helpManual();
	void helpCheatSheet();
	void helpLibrary();
	void helpFontInfo();
	void quit();
	void checkAutoReload();
	void waitAfterReload();
	void autoReloadSet(bool);

private:
	bool network_progress_func(const double permille);
	static void report_func(const class AbstractNode*, void *vp, int mark);
	static bool undockMode;
	static bool reorderMode;
	static const int tabStopWidth;
	static QElapsedTimer *progressThrottle;

	shared_ptr<class CSGNode> csgRoot;		   // Result of the CSGTreeEvaluator
	shared_ptr<CSGNode> normalizedRoot;		  // Normalized CSG tree
 	shared_ptr<class CSGProducts> root_products;
	shared_ptr<CSGProducts> highlights_products;
	shared_ptr<CSGProducts> background_products;

	char const * afterCompileSlot;
	bool procevents;
	class QTemporaryFile *tempFile;
	class ProgressWidget *progresswidget;
	class CGALWorker *cgalworker;
	QMutex consolemutex;
	EditorInterface *renderedEditor; // stores pointer to editor which has been most recently rendered
	time_t includes_mtime;   // latest include mod time
	time_t deps_mtime;	  // latest dependency mod time
	std::unordered_map<std::string, QString> export_paths; // for each file type, where it was exported to last
	QString exportPath(const char *suffix); // look up the last export path and generate one if not found
	int last_parser_error_pos; // last highlighted error position
	int tabCount = 0;

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
