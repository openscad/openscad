#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "openscad.h"
#include "modcontext.h"
#include "module.h"
#include "Tree.h"
#include "memory.h"
#include <vector>
#include <QMutex>

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	static void requestOpenFile(const QString &filename);

	QString fileName;
	class Highlighter *highlighter;

	class Preferences *prefs;

	QTimer *animate_timer;
	double tval, fps, fsteps;

	QTimer *autoReloadTimer;
	std::string autoReloadId;

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
	class CGAL_Nef_polyhedron *root_N;
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

	static const int maxRecentFiles = 10;
	QAction *actionRecentFile[maxRecentFiles];

	MainWindow(const QString &filename);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void updatedFps();
	void updateTVal();
	void setFileName(const QString &filename);
	void setFont(const QString &family, uint size);
	void showProgress();
	void openCSGSettingsChanged();

private:
	void openFile(const QString &filename);
	void refreshDocument();
	void updateTemporalVariables();
	bool fileChangedOnDisk();
	bool compileTopLevelDocument(bool reload);
	bool compile(bool reload, bool procevents);
	void compileCSG(bool procevents);
	bool maybeSave();
	bool checkEditorModified();
	QString dumpCSGTree(AbstractNode *root);
	static void consoleOutput(const std::string &msg, void *userdata);
	void loadViewSettings();
	void loadDesignSettings();

  class QMessageBox *openglbox;

private slots:
	void actionUpdateCheck();
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

private slots:
	void pasteViewportTranslation();
	void pasteViewportRotation();
	void hideEditor();
	void preferences();

private slots:
	void actionCompile();
#ifdef ENABLE_CGAL
	void actionRenderCGAL();
	void actionRenderCGALDone(class CGAL_Nef_polyhedron *);
#endif
	void actionDisplayAST();
	void actionDisplayCSGTree();
	void actionDisplayCSGProducts();
	void actionExportSTLorOFF(bool stl_mode);
	void actionExportSTL();
	void actionExportOFF();
	void actionExportDXF();
	void actionExportCSG();
	void actionExportImage();
	void actionFlushCaches();

public:
	void viewModeActionsUncheck();
	void setCurrentOutput();
	void clearCurrentOutput();

public slots:
#ifdef ENABLE_OPENCSG
	void viewModeOpenCSG();
#endif
#ifdef ENABLE_CGAL
	void viewModeCGALSurface();
	void viewModeCGALGrid();
#endif
	void viewModeThrownTogether();
	void viewModeShowEdges();
	void viewModeShowAxes();
	void viewModeShowCrosshairs();
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
	void hideConsole();
	void animateUpdateDocChanged();
	void animateUpdate();
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void helpAbout();
	void helpHomepage();
	void helpManual();
	void helpLibrary();
	void quit();
	void actionReloadCompile();
	void checkAutoReload();
	void autoReloadSet(bool);

private:
	static void report_func(const class AbstractNode*, void *vp, int mark);

	class ProgressWidget *progresswidget;
	class CGALWorker *cgalworker;
	QMutex consolemutex;
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

#endif
