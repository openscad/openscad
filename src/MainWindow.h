#pragma once

#include "qtgettext.h"
#include <QMainWindow>
#include <QIcon>
#include "ui_MainWindow.h"
#include "UIUtils.h"
#include "openscad.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Tree.h"
#include "memory.h"
#include "editor.h"
#include "export.h"
#include <vector>
#include <QMutex>
#include <QTime>
#include <QIODevice>

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	QString fileName;

	class Preferences *prefs;

	QTimer *animate_timer;
	int anim_step;
	int anim_numsteps;
	double anim_tval;
	bool anim_dumping;
	int anim_dump_start_step;

	QTimer *autoReloadTimer;
	std::string autoReloadId;
	QTimer *waitAfterReloadTimer;

	QTime renderingTime;

	ModuleContext top_ctx;
	FileModule *root_module;		  // Result of parsing
	FileModule *parsed_module;		// Last parse for include list
	ModuleInstantiation root_inst;	// Top level instance
	AbstractNode *absolute_root_node; // Result of tree evaluation
	AbstractNode *root_node;		  // Root if the root modifier (!) is used
	Tree tree;

#ifdef ENABLE_CGAL
	shared_ptr<const class Geometry> root_geom;
	class CGALRenderer *cgalRenderer;
#endif
#ifdef ENABLE_OPENCSG
	class OpenCSGRenderer *opencsgRenderer;
#endif
	class ThrownTogetherRenderer *thrownTogetherRenderer;

	QString last_compiled_doc;

	QAction *actionRecentFile[UIUtils::maxRecentFiles];
		QMap<QString, QString> knownFileExtensions;

		QLabel *versionLabel;
		QWidget *editorDockTitleWidget;
		QWidget *consoleDockTitleWidget;
		QWidget *parameterDockTitleWidget;

	QString editortype;	
	bool useScintilla;

		int compileErrors;
		int compileWarnings;

	MainWindow(const QString &filename);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void updatedAnimTval();
	void updatedAnimFps();
	void updatedAnimSteps();
	void updatedAnimDump(bool checked);
	void updateTVal();
		void updateMdiMode(bool mdi);
		void updateUndockMode(bool undockMode);
		void updateReorderMode(bool reorderMode);
	void setFileName(const QString &filename);
	void setFont(const QString &family, uint size);
	void setColorScheme(const QString &cs);
	void showProgress();
	void openCSGSettingsChanged();
	void consoleOutput(const QString &msg);
	void updateActionUndoState();

private:
		void initActionIcon(QAction *action, const char *darkResource, const char *lightResource);
		void handleFileDrop(const QString &filename);
	void refreshDocument();
	void updateCamera(const class FileContext &ctx);
	void updateTemporalVariables();
	bool fileChangedOnDisk();
	void compileTopLevelDocument();
		void updateCompileResult();
	void compile(bool reload, bool forcedone = false);
	void compileCSG(bool procevents);
	bool maybeSave();
		void saveError(const QIODevice &file, const std::string &msg);
	bool checkEditorModified();
	QString dumpCSGTree(AbstractNode *root);
	static void consoleOutput(const std::string &msg, void *userdata);
	void loadViewSettings();
	void loadDesignSettings();
	void updateWindowSettings(bool console, bool editor, bool customizer, bool toolbar);
	void saveBackup();
	void writeBackup(class QFile *file);
	QString get2dExportFilename(QString format, QString extension);
	void show_examples();
	void setDockWidgetTitle(QDockWidget *dockWidget, QString prefix, bool topLevel);
		void addKeyboardShortCut(const QList<QAction *> &actions);
		void updateStatusBar(class ProgressWidget *progressWidget);

	EditorInterface *editor;

  class LibraryInfoDialog* library_info_dialog;
  class FontListDialog *font_list_dialog;

private slots:
	void actionNew();
	void actionOpen();
	void actionOpenRecent();
	void actionOpenExample();
	void updateRecentFiles();
	void clearRecentFiles();
	void updateRecentFileActions();
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
	void pasteViewportTranslation();
	void pasteViewportRotation();
	void preferences();
	void hideToolbars();
	void hideEditor();
	void hideConsole();
	void showConsole();
	void hideParameters();

private slots:
	void selectFindType(int);
	void hideFind();
	void showFind();
	void findString(QString);
	void showFindAndReplace();
	void findNext();
	void findPrev();
	void useSelectionForFind();
	void replace();
	void replaceAll();

	// Mac OSX FindBuffer support
	void findBufferChanged();
	void updateFindBuffer(QString);
protected:
	virtual bool eventFilter(QObject* obj, QEvent *event);

private slots:
	void actionRenderPreview();
	void csgRender();
	void csgReloadRender();
#ifdef ENABLE_CGAL
	void actionRender();
	void actionRenderDone(shared_ptr<const class Geometry>);
	void cgalRender();
#endif
	void actionCheckValidity();
	void actionDisplayAST();
	void actionDisplayCSGTree();
	void actionDisplayCSGProducts();
	void actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim);
	void actionExportSTL();
	void actionExportOFF();
	void actionExportAMF();
	void actionExportDXF();
	void actionExportSVG();
	void actionExportCSG();
	void actionExportImage();
	void actionCopyViewport();
	void actionFlushCaches();

public:
	void viewModeActionsUncheck();
	void setCurrentOutput();
	void clearCurrentOutput();
  bool isEmpty();

public slots:
	void openFile(const QString &filename);
	void actionReloadRenderPreview();
		void on_editorDock_visibilityChanged(bool);
		void on_consoleDock_visibilityChanged(bool);
		void on_parameterDock_visibilityChanged(bool);
		void on_toolButtonCompileResultClose_clicked();
		void editorTopLevelChanged(bool);
		void consoleTopLevelChanged(bool);
		void parameterTopLevelChanged(bool);

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
	void viewResetView();
	void viewAll();
	void animateUpdateDocChanged();
	void animateUpdate();
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
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
	void setContentsChanged();

private:
	static void report_func(const class AbstractNode*, void *vp, int mark);
	static bool mdiMode;
	static bool undockMode;
	static bool reorderMode;
	static const int tabStopWidth;

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
	bool contentschanged; // Set if the source code has changes since the last render (F6)
	time_t includes_mtime;   // latest include mod time
	time_t deps_mtime;	  // latest dependency mod time

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
