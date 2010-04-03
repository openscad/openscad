#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "openscad.h"
#include "context.h"
#include "module.h"
#include "polyset.h"
#include <QPointer>

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

	Context root_ctx;
	AbstractModule *root_module;      // Result of parsing
	ModuleInstantiation root_inst;    // Top level instance
	AbstractNode *absolute_root_node; // Result of tree evaluation
	AbstractNode *root_node;          // Root if the root modifier (!) is used

	class CSGTerm *root_raw_term;           // Result of CSG term rendering
	CSGTerm *root_norm_term;          // Normalized CSG products
	class CSGChain *root_chain;
#ifdef ENABLE_CGAL
	class CGAL_Nef_polyhedron *root_N;
	bool recreate_cgal_ogl_p;
	void *cgal_ogl_p;
	PolySet *cgal_ogl_ps;
#endif

	QVector<CSGTerm*> highlight_terms;
	CSGChain *highlights_chain;
	QVector<CSGTerm*> background_terms;
	CSGChain *background_chain;
	QString last_compiled_doc;
	bool enableOpenCSG;

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
#ifdef USE_PROGRESSWIDGET
	void showProgress();
#endif

private:
	void openFile(const QString &filename);
	void load();
	AbstractNode *find_root_tag(AbstractNode *n);
	void compile(bool procevents);
	void compileCSG(bool procevents);
	bool maybeSave();
	static void consoleOutput(const QString &msg, void *userdata) {
		static_cast<MainWindow*>(userdata)->console->append(msg);
	}

private slots:
	void actionNew();
	void actionOpen();
	void actionOpenRecent();
	void actionOpenExample();
	void clearRecentFiles();
	void updateRecentFileActions();
	void actionSave();
	void actionSaveAs();
	void actionReload();

private slots:
	void pasteViewportTranslation();
	void pasteViewportRotation();
	void hideEditor();
	void preferences();

private slots:
	void actionCompile();
#ifdef ENABLE_CGAL
	void actionRenderCGAL();
#endif
	void actionDisplayAST();
	void actionDisplayCSGTree();
	void actionDisplayCSGProducts();
	void actionExportSTLorOFF(bool stl_mode);
	void actionExportSTL();
	void actionExportOFF();
	void actionExportDXF();
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
	void quit();
	void actionReloadCompile();
};

#endif
