#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "openscad.h"

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
  static QPointer<MainWindow> current_win;
	static void requestOpenFile(const QString &filename);

	QString filename;
	class Highlighter *highlighter;

	QTimer *animate_timer;
	double tval, fps, fsteps;

	Context root_ctx;
	AbstractModule *root_module;
	AbstractNode *absolute_root_node;
	CSGTerm *root_raw_term;
	CSGTerm *root_norm_term;
	CSGChain *root_chain;
#ifdef ENABLE_CGAL
	CGAL_Nef_polyhedron *root_N;
	bool recreate_cgal_ogl_p;
	void *cgal_ogl_p;
#endif

	QVector<CSGTerm*> highlight_terms;
	CSGChain *highlights_chain;
	QVector<CSGTerm*> background_terms;
	CSGChain *background_chain;
	AbstractNode *root_node;
	QString last_compiled_doc;
	bool enableOpenCSG;

	MainWindow(const char *filename = 0);
	~MainWindow();

private slots:
	void updatedFps();
	void updateTVal();

private:
	void openFile(const QString &filename);
	void load();
	void maybe_change_dir();
	void find_root_tag(AbstractNode *n);
	void compile(bool procevents);

private slots:
	void actionNew();
	void actionOpen();
	void actionSave();
	void actionSaveAs();
	void actionReload();

private slots:
	void editIndent();
	void editUnindent();
	void editComment();
	void editUncomment();
	void pasteViewportTranslation();
	void pasteViewportRotation();

private slots:
	void actionReloadCompile();
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

public:
	void viewModeActionsUncheck();

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
	void animateUpdateDocChanged();
	void animateUpdate();
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void helpAbout();
};

#endif
