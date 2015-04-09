#pragma once

#include "qtgettext.h"
#include <QMainWindow>
#include <QIcon>
#include "ui_MainWindow.h"
#include "UIUtils.h"
#include "openscad.h"
#include "modcontext.h"
#include "module.h"
#include "Tree.h"
#include "memory.h"
#include "editor.h"
#include <vector>
#include <QMutex>
#include <QSet>
#include <QTime>

enum export_type_e {
	EXPORT_TYPE_UNKNOWN,
	EXPORT_TYPE_STL,
	EXPORT_TYPE_AMF,
	EXPORT_TYPE_OFF
};

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	static void requestOpenFile(const QString &filename);

	QString fileName;

	class Preferences *prefs;

	QTimer *animate_timer;
	double tval, fps, fsteps;

	QTimer *autoReloadTimer;
	std::string autoReloadId;
	QTimer *waitAfterReloadTimer;

	QTime renderingTime;

	ModuleContext top_ctx;
	FileModule *root_module;      // Result of parsing
	ModuleInstantiation root_inst;    // Top level instance
	AbstractNode *absolute_root_node; // Result of tree evaluation
	AbstractNode *root_node;          // Root if the root modifier (!) is used
	Tree tree;

	shared_ptr<class CSGTerm> root_raw_term;           // Result of CSG term rendering
	shared_ptr<CSGTerm> root_norm_term;          // Normalized CSG products
	class CSGChain *root_chain;
#ifdef ENABLE_CGAL
	shared_ptr<const class Geometry> root_geom;
	class CGALRenderer *cgalRenderer;
#endif
#ifdef ENABLE_OPENCSG
	class OpenCSGRenderer *opencsgRenderer;
#endif
	class ThrownTogetherRenderer *thrownTogetherRenderer;

	std::vector<shared_ptr<CSGTerm> > highlight_terms;
	CSGChain *highlights_chain;
	std::vector<shared_ptr<CSGTerm> > background_terms;
	CSGChain *background_chain;
	QString last_compiled_doc;

	QAction *actionRecentFile[UIUtils::maxRecentFiles];
        QMap<QString, QString> knownFileExtensions;

        QLabel *versionLabel;
        QWidget *editorDockTitleWidget;
        QWidget *consoleDockTitleWidget;
        
	QString editortype;	
	bool useScintilla;

        int compileErrors;
        int compileWarnings;

	MainWindow(const QString &filename);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void updatedFps();
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

private:
        void initActionIcon(QAction *action, const char *darkResource, const char *lightResource);
	void openFile(const QString &filename);
        void handleFileDrop(const QString &filename);
	void refreshDocument();
        void updateCamera();
	void updateTemporalVariables();
	bool fileChangedOnDisk();
	void compileTopLevelDocument();
        void updateCompileResult();
	void compile(bool reload, bool forcedone = false);
	void compileCSG(bool procevents);
	bool maybeSave();
	bool checkEditorModified();
	QString dumpCSGTree(AbstractNode *root);
	static void consoleOutput(const std::string &msg, void *userdata);
	void loadViewSettings();
	void loadDesignSettings();
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

private slots:
	void pasteViewportTranslation();
	void pasteViewportRotation();
	void preferences();
	void hideToolbars();
	void hideEditor();
	void hideConsole();
	void showConsole();

private slots:
	void selectFindType(int);
	void find();
	void findString(QString);
	void findAndReplace();
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
	void actionExport(export_type_e, const char *, const char *);
	void actionExportSTL();
	void actionExportOFF();
	void actionExportAMF();
	void actionExportDXF();
	void actionExportSVG();
	void actionExportCSG();
	void actionExportImage();
	void actionFlushCaches();

public:
	static QSet<MainWindow*> *getWindows();
	void viewModeActionsUncheck();
	void setCurrentOutput();
	void clearCurrentOutput();

public slots:
	void actionReloadRenderPreview();
        void on_editorDock_visibilityChanged(bool);
        void on_consoleDock_visibilityChanged(bool);
        void on_toolButtonCompileResultClose_clicked();
        void editorTopLevelChanged(bool);
        void consoleTopLevelChanged(bool);
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
	void showFontCacheDialog();
	void hideFontCacheDialog();

private:
	static void report_func(const class AbstractNode*, void *vp, int mark);
	static bool mdiMode;
	static bool undockMode;
	static bool reorderMode;
	static QSet<MainWindow*> *windows;
	static class QProgressDialog *fontCacheDialog;

	char const * afterCompileSlot;
	bool procevents;
	class QTemporaryFile *tempFile;
	class ProgressWidget *progresswidget;
	class CGALWorker *cgalworker;
	QMutex consolemutex;
	bool contentschanged; // Set if the source code has changes since the last render (F6)

signals:
	void highlightError(int);
	void unhighlightLastError();
};

class GuiLocker
{
public:
	GuiLocker() {
		gui_locked++;
	}
	~GuiLocker() {
		gui_locked--;
	}
	static bool isLocked() { return gui_locked > 0; }
	static void lock() { gui_locked++; }
	static void unlock() { gui_locked--; }

private:
 	static unsigned int gui_locked;
};
