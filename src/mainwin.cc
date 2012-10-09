/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PolySetCache.h"
#include "ModuleCache.h"
#include "MainWindow.h"
#include "openscad.h" // examplesdir
#include "parsersettings.h"
#include "Preferences.h"
#include "printutils.h"
#include "node.h"
#include "polyset.h"
#include "csgterm.h"
#include "highlighter.h"
#include "export.h"
#include "builtin.h"
#include "progress.h"
#include "dxfdim.h"
#include "AboutDialog.h"
#ifdef ENABLE_OPENCSG
#include "CSGTermEvaluator.h"
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ProgressWidget.h"
#include "ThrownTogetherRenderer.h"
#include "csgtermnormalizer.h"

#include <QMenu>
#include <QTime>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QTextStream>
#include <QStatusBar>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QProgressDialog>
#include <QMutexLocker>
#ifdef _QCODE_EDIT_
#include "qdocument.h"
#include "qformatscheme.h"
#include "qlanguagefactory.h"
#endif

#include <fstream>

#include <algorithm>
#include <boost/version.hpp>
#include <boost/foreach.hpp>
#include <boost/version.hpp>
#include <sys/stat.h>

#ifdef ENABLE_CGAL

#include "CGALCache.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"
#include "CGALRenderer.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalworker.h"

#endif // ENABLE_CGAL

#ifndef OPENCSG_VERSION_STRING
#define OPENCSG_VERSION_STRING "unknown, <1.3.2"
#endif

extern QString examplesdir;

// Global application state
unsigned int GuiLocker::gui_locked = 0;

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

static char helptitle[] =
	"OpenSCAD " QUOTED(OPENSCAD_VERSION)
#ifdef OPENSCAD_COMMIT
	" (git " QUOTED(OPENSCAD_COMMIT) ")"
#endif
	"\nhttp://www.openscad.org\n\n";
static char copyrighttext[] =
	"Copyright (C) 2009-2012 Marius Kintel <marius@kintel.net> and Clifford Wolf <clifford@clifford.at>\n"
	"\n"
	"This program is free software; you can redistribute it and/or modify "
	"it under the terms of the GNU General Public License as published by "
	"the Free Software Foundation; either version 2 of the License, or "
	"(at your option) any later version.";

static void
settings_setValueList(const QString &key,const QList<int> &list)
{
	QSettings settings;
	settings.beginWriteArray(key);
	for (int i=0;i<list.size(); ++i) {
		settings.setArrayIndex(i);
		settings.setValue("entry",list[i]);
	}
	settings.endArray();
}

QList<int>
settings_valueList(const QString &key, const QList<int> &defaultList = QList<int>())
{
	QSettings settings;
	QList<int> result;
	if (settings.contains(key+"/size")){
		int length = settings.beginReadArray(key);
		for (int i = 0; i < length; ++i) {
			settings.setArrayIndex(i);
			result += settings.value("entry").toInt();
		}
		settings.endArray();
		return result;
	} else {
		return defaultList;
	}

}

MainWindow::MainWindow(const QString &filename)
	: progresswidget(NULL)
{
	setupUi(this);

	this->cgalworker = new CGALWorker();
	connect(this->cgalworker, SIGNAL(done(CGAL_Nef_polyhedron *)), 
					this, SLOT(actionRenderCGALDone(CGAL_Nef_polyhedron *)));

	register_builtin(root_ctx);

	this->openglbox = NULL;
	root_module = NULL;
	absolute_root_node = NULL;
	this->root_chain = NULL;
#ifdef ENABLE_CGAL
	this->root_N = NULL;
	this->cgalRenderer = NULL;
#endif
#ifdef ENABLE_OPENCSG
	this->opencsgRenderer = NULL;
#endif
	this->thrownTogetherRenderer = NULL;

	highlights_chain = NULL;
	background_chain = NULL;
	root_node = NULL;

	tval = 0;
	fps = 0;
	fsteps = 1;

	highlighter = NULL;
#ifdef _QCODE_EDIT_
	QFormatScheme *formats = new QFormatScheme("qxs/openscad.qxf");
	QDocument::setDefaultFormatScheme(formats);
	QLanguageFactory *languages = new QLanguageFactory(formats,this);
	languages->addDefinitionPath("qxs");
	languages->setLanguage(editor, "openscad");
#else
	editor->setTabStopWidth(30);
#endif
	editor->setLineWrapping(true); // Not designable

	this->glview->statusLabel = new QLabel(this);
	statusBar()->addWidget(this->glview->statusLabel);

	animate_timer = new QTimer(this);
	connect(animate_timer, SIGNAL(timeout()), this, SLOT(updateTVal()));

	autoReloadTimer = new QTimer(this);
	autoReloadTimer->setSingleShot(false);
	connect(autoReloadTimer, SIGNAL(timeout()), this, SLOT(checkAutoReload()));

	connect(this->e_tval, SIGNAL(textChanged(QString)), this, SLOT(actionCompile()));
	connect(this->e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedFps()));

	animate_panel->hide();

	// File menu
	connect(this->fileActionNew, SIGNAL(triggered()), this, SLOT(actionNew()));
	connect(this->fileActionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(this->fileActionSave, SIGNAL(triggered()), this, SLOT(actionSave()));
	connect(this->fileActionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs()));
	connect(this->fileActionReload, SIGNAL(triggered()), this, SLOT(actionReload()));
	connect(this->fileActionQuit, SIGNAL(triggered()), this, SLOT(quit()));
#ifndef __APPLE__
	QList<QKeySequence> shortcuts = this->fileActionSave->shortcuts();
	shortcuts.push_back(QKeySequence(Qt::Key_F2));
	this->fileActionSave->setShortcuts(shortcuts);
	shortcuts = this->fileActionReload->shortcuts();
	shortcuts.push_back(QKeySequence(Qt::Key_F3));
	this->fileActionReload->setShortcuts(shortcuts);
#endif
	// Open Recent
	for (int i = 0;i<maxRecentFiles; i++) {
		this->actionRecentFile[i] = new QAction(this);
		this->actionRecentFile[i]->setVisible(false);
		this->menuOpenRecent->addAction(this->actionRecentFile[i]);
		connect(this->actionRecentFile[i], SIGNAL(triggered()),
						this, SLOT(actionOpenRecent()));
	}
	this->menuOpenRecent->addSeparator();
	this->menuOpenRecent->addAction(this->fileActionClearRecent);
	connect(this->fileActionClearRecent, SIGNAL(triggered()),
					this, SLOT(clearRecentFiles()));

	if (!examplesdir.isEmpty()) {
		bool found_example = false;
		QStringList examples = QDir(examplesdir).entryList(QStringList("*.scad"), 
		QDir::Files | QDir::Readable, QDir::Name);
		foreach (const QString &ex, examples) {
			this->menuExamples->addAction(ex, this, SLOT(actionOpenExample()));
			found_example = true;
		}
		if (!found_example) {
			delete this->menuExamples;
			this->menuExamples = NULL;
		}
	} else {
		delete this->menuExamples;
		this->menuExamples = NULL;
	}

	// Edit menu
	connect(this->editActionUndo, SIGNAL(triggered()), editor, SLOT(undo()));
	connect(this->editActionRedo, SIGNAL(triggered()), editor, SLOT(redo()));
	connect(this->editActionCut, SIGNAL(triggered()), editor, SLOT(cut()));
	connect(this->editActionCopy, SIGNAL(triggered()), editor, SLOT(copy()));
	connect(this->editActionPaste, SIGNAL(triggered()), editor, SLOT(paste()));
	connect(this->editActionIndent, SIGNAL(triggered()), editor, SLOT(indentSelection()));
	connect(this->editActionUnindent, SIGNAL(triggered()), editor, SLOT(unindentSelection()));
	connect(this->editActionComment, SIGNAL(triggered()), editor, SLOT(commentSelection()));
	connect(this->editActionUncomment, SIGNAL(triggered()), editor, SLOT(uncommentSelection()));
	connect(this->editActionPasteVPT, SIGNAL(triggered()), this, SLOT(pasteViewportTranslation()));
	connect(this->editActionPasteVPR, SIGNAL(triggered()), this, SLOT(pasteViewportRotation()));
	connect(this->editActionZoomIn, SIGNAL(triggered()), editor, SLOT(zoomIn()));
	connect(this->editActionZoomOut, SIGNAL(triggered()), editor, SLOT(zoomOut()));
	connect(this->editActionHide, SIGNAL(triggered()), this, SLOT(hideEditor()));
	connect(this->editActionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));

	// Design menu
	connect(this->designActionAutoReload, SIGNAL(toggled(bool)), this, SLOT(autoReloadSet(bool)));
	connect(this->designActionReloadAndCompile, SIGNAL(triggered()), this, SLOT(actionReloadCompile()));
	connect(this->designActionCompile, SIGNAL(triggered()), this, SLOT(actionCompile()));
#ifdef ENABLE_CGAL
	connect(this->designActionCompileAndRender, SIGNAL(triggered()), this, SLOT(actionRenderCGAL()));
#else
	this->designActionCompileAndRender->setVisible(false);
#endif
	connect(this->designActionDisplayAST, SIGNAL(triggered()), this, SLOT(actionDisplayAST()));
	connect(this->designActionDisplayCSGTree, SIGNAL(triggered()), this, SLOT(actionDisplayCSGTree()));
	connect(this->designActionDisplayCSGProducts, SIGNAL(triggered()), this, SLOT(actionDisplayCSGProducts()));
	connect(this->designActionExportSTL, SIGNAL(triggered()), this, SLOT(actionExportSTL()));
	connect(this->designActionExportOFF, SIGNAL(triggered()), this, SLOT(actionExportOFF()));
	connect(this->designActionExportDXF, SIGNAL(triggered()), this, SLOT(actionExportDXF()));
	connect(this->designActionExportCSG, SIGNAL(triggered()), this, SLOT(actionExportCSG()));
	connect(this->designActionExportImage, SIGNAL(triggered()), this, SLOT(actionExportImage()));
	connect(this->designActionFlushCaches, SIGNAL(triggered()), this, SLOT(actionFlushCaches()));

	// View menu
#ifndef ENABLE_OPENCSG
	this->viewActionOpenCSG->setVisible(false);
#else
	connect(this->viewActionOpenCSG, SIGNAL(triggered()), this, SLOT(viewModeOpenCSG()));
	if (!this->glview->hasOpenCSGSupport()) {
		this->viewActionOpenCSG->setEnabled(false);
	}
#endif

#ifdef ENABLE_CGAL
	connect(this->viewActionCGALSurfaces, SIGNAL(triggered()), this, SLOT(viewModeCGALSurface()));
	connect(this->viewActionCGALGrid, SIGNAL(triggered()), this, SLOT(viewModeCGALGrid()));
#else
	this->viewActionCGALSurfaces->setVisible(false);
	this->viewActionCGALGrid->setVisible(false);
#endif
	connect(this->viewActionThrownTogether, SIGNAL(triggered()), this, SLOT(viewModeThrownTogether()));
	connect(this->viewActionShowEdges, SIGNAL(triggered()), this, SLOT(viewModeShowEdges()));
	connect(this->viewActionShowAxes, SIGNAL(triggered()), this, SLOT(viewModeShowAxes()));
	connect(this->viewActionShowCrosshairs, SIGNAL(triggered()), this, SLOT(viewModeShowCrosshairs()));
	connect(this->viewActionAnimate, SIGNAL(triggered()), this, SLOT(viewModeAnimate()));
	connect(this->viewActionTop, SIGNAL(triggered()), this, SLOT(viewAngleTop()));
	connect(this->viewActionBottom, SIGNAL(triggered()), this, SLOT(viewAngleBottom()));
	connect(this->viewActionLeft, SIGNAL(triggered()), this, SLOT(viewAngleLeft()));
	connect(this->viewActionRight, SIGNAL(triggered()), this, SLOT(viewAngleRight()));
	connect(this->viewActionFront, SIGNAL(triggered()), this, SLOT(viewAngleFront()));
	connect(this->viewActionBack, SIGNAL(triggered()), this, SLOT(viewAngleBack()));
	connect(this->viewActionDiagonal, SIGNAL(triggered()), this, SLOT(viewAngleDiagonal()));
	connect(this->viewActionCenter, SIGNAL(triggered()), this, SLOT(viewCenter()));
	connect(this->viewActionPerspective, SIGNAL(triggered()), this, SLOT(viewPerspective()));
	connect(this->viewActionOrthogonal, SIGNAL(triggered()), this, SLOT(viewOrthogonal()));
	connect(this->viewActionHide, SIGNAL(triggered()), this, SLOT(hideConsole()));

	// Help menu
	connect(this->helpActionAbout, SIGNAL(triggered()), this, SLOT(helpAbout()));
	connect(this->helpActionHomepage, SIGNAL(triggered()), this, SLOT(helpHomepage()));
	connect(this->helpActionManual, SIGNAL(triggered()), this, SLOT(helpManual()));
	connect(this->helpActionLibraryInfo, SIGNAL(triggered()), this, SLOT(helpLibrary()));


	setCurrentOutput();

	PRINT(helptitle);
	PRINT(copyrighttext);
	PRINT("");

	if (!filename.isEmpty()) {
		openFile(filename);
	} else {
		setFileName("");
	}
	updateRecentFileActions();

	connect(editor->document(), SIGNAL(contentsChanged()), this, SLOT(animateUpdateDocChanged()));
#ifdef _QCODE_EDIT_
	connect(editor, SIGNAL(contentModified(bool)), this, SLOT(setWindowModified(bool)));
	connect(editor, SIGNAL(contentModified(bool)), fileActionSave, SLOT(setEnabled(bool)));
#else
	connect(editor->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
	connect(editor->document(), SIGNAL(modificationChanged(bool)), fileActionSave, SLOT(setEnabled(bool)));
#endif
	connect(this->glview, SIGNAL(doAnimateUpdate()), this, SLOT(animateUpdate()));

	connect(Preferences::inst(), SIGNAL(requestRedraw()), this->glview, SLOT(updateGL()));
	connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)), 
					this, SLOT(setFont(const QString&,uint)));
	connect(Preferences::inst(), SIGNAL(openCSGSettingsChanged()),
					this, SLOT(openCSGSettingsChanged()));
	Preferences::inst()->apply();

	// make sure it looks nice..
	QSettings settings;
	resize(settings.value("window/size", QSize(800, 600)).toSize());
	move(settings.value("window/position", QPoint(0, 0)).toPoint());
	QList<int> s1sizes = settings_valueList("window/splitter1sizes",QList<int>()<<400<<400);
	QList<int> s2sizes = settings_valueList("window/splitter2sizes",QList<int>()<<400<<200);
	splitter1->setSizes(s1sizes);
	splitter2->setSizes(s2sizes);

	// display this window and check for OpenGL 2.0 (OpenCSG) support
	viewModeThrownTogether();
	show();

#ifdef ENABLE_OPENCSG
	viewModeOpenCSG();
#else
	viewModeThrownTogether();
#endif
	loadViewSettings();
	loadDesignSettings();

	setAcceptDrops(true);
	clearCurrentOutput();
}

void
MainWindow::loadViewSettings(){
	QSettings settings;
	if (settings.value("view/showEdges").toBool()) {
		viewActionShowEdges->setChecked(true);
		viewModeShowEdges();
	}
	if (settings.value("view/showAxes").toBool()) {
		viewActionShowAxes->setChecked(true);
		viewModeShowAxes();
	}
	if (settings.value("view/showCrosshairs").toBool()) {
		viewActionShowCrosshairs->setChecked(true);
		viewModeShowCrosshairs();
	}
	if (settings.value("view/orthogonalProjection").toBool()) {
		viewOrthogonal();
	} else {
		viewPerspective();
	}
	if (settings.value("view/hideConsole").toBool()) {
		viewActionHide->setChecked(true);
		hideConsole();
	}
	if (settings.value("view/hideEditor").toBool()) {
		editActionHide->setChecked(true);
		hideEditor();
	}
}

void
MainWindow::loadDesignSettings()
{
	QSettings settings;
	if (settings.value("design/autoReload").toBool()) {
		designActionAutoReload->setChecked(true);
	}
	uint polySetCacheSize = Preferences::inst()->getValue("advanced/polysetCacheSize").toUInt();
	PolySetCache::instance()->setMaxSize(polySetCacheSize);
	uint cgalCacheSize = Preferences::inst()->getValue("advanced/cgalCacheSize").toUInt();
	CGALCache::instance()->setMaxSize(cgalCacheSize);
}

MainWindow::~MainWindow()
{
	if (root_module) delete root_module;
	if (root_node) delete root_node;
#ifdef ENABLE_CGAL
	if (this->root_N) delete this->root_N;
	delete this->cgalRenderer;
#endif
#ifdef ENABLE_OPENCSG
	delete this->opencsgRenderer;
#endif
}

void MainWindow::showProgress()
{
	this->statusBar()->addPermanentWidget(qobject_cast<ProgressWidget*>(sender()));
}

void MainWindow::report_func(const class AbstractNode*, void *vp, int mark)
{
	MainWindow *thisp = static_cast<MainWindow*>(vp);
	int v = (int)((mark*100.0) / progress_report_count);
	int percent = v < 100 ? v : 99; 
	
	if (percent > thisp->progresswidget->value()) {
		QMetaObject::invokeMethod(thisp->progresswidget, "setValue", Qt::QueuedConnection,
															Q_ARG(int, percent));
		QApplication::processEvents();
	}

	if (thisp->progresswidget->wasCanceled()) throw ProgressCancelException();
}

/*!
	Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
#ifdef ENABLE_MDI
void MainWindow::requestOpenFile(const QString &filename)
{
	new MainWindow(filename);
}
#else
void MainWindow::requestOpenFile(const QString &)
{
}
#endif

void
MainWindow::openFile(const QString &new_filename)
{
#ifdef ENABLE_MDI
#ifdef _QCODE_EDIT_
	if (this->editor->document()->lines() > 1 ||
			!this->editor->document()->text(true, false).trimmed().isEmpty()) {
#else
	if (!editor->toPlainText().isEmpty()) {
#endif
		new MainWindow(new_filename);
		clearCurrentOutput();
		return;
	}
#endif
	setFileName(new_filename);

	refreshDocument();
	updateRecentFiles();
}

void
MainWindow::setFileName(const QString &filename)
{
	if (filename.isEmpty()) {
		this->fileName.clear();
		this->root_ctx.setDocumentPath(currentdir);
		setWindowTitle("OpenSCAD - New Document[*]");
	}
	else {
		QFileInfo fileinfo(filename);
		setWindowTitle("OpenSCAD - " + fileinfo.fileName() + "[*]");

		// Check that the canonical file path exists - only update recent files
		// if it does. Should prevent empty list items on initial open etc.
		QString infoFileName = fileinfo.absoluteFilePath();
		if (!infoFileName.isEmpty()) {
			this->fileName = infoFileName;
		} else {
			this->fileName = fileinfo.fileName();
		}
		
		this->root_ctx.setDocumentPath(fileinfo.dir().absolutePath().toStdString());
		QDir::setCurrent(fileinfo.dir().absolutePath());
	}

}

void MainWindow::updateRecentFiles()
{
	// Check that the canonical file path exists - only update recent files
	// if it does. Should prevent empty list items on initial open etc.
	QFileInfo fileinfo(this->fileName);
	QString infoFileName = fileinfo.absoluteFilePath();
	QSettings settings; // already set up properly via main.cpp
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(infoFileName);
	files.prepend(infoFileName);
	while (files.size() > maxRecentFiles) files.removeLast();
	settings.setValue("recentFileList", files);

	foreach(QWidget *widget, QApplication::topLevelWidgets()) {
		MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
		if (mainWin) {
			mainWin->updateRecentFileActions();
		}
	}
}


void MainWindow::updatedFps()
{
	bool fps_ok;
	double fps = this->e_fps->text().toDouble(&fps_ok);
	animate_timer->stop();
	if (fps_ok && fps > 0) {
		animate_timer->setSingleShot(false);
		animate_timer->setInterval(int(1000 / this->e_fps->text().toDouble()));
		animate_timer->start();
	}
}

void MainWindow::updateTVal()
{
	bool fps_ok;
	double fps = this->e_fps->text().toDouble(&fps_ok);
	if (fps_ok) {
		if (fps <= 0) {
			actionCompile();
		} else {
			double s = this->e_fsteps->text().toDouble();
			double t = this->e_tval->text().toDouble() + 1/s;
			QString txt;
			txt.sprintf("%.5f", t >= 1.0 ? 0.0 : t);
			this->e_tval->setText(txt);
		}
	}
}

void MainWindow::refreshDocument()
{
	setCurrentOutput();
	if (!this->fileName.isEmpty()) {
		QFile file(this->fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			PRINTB("Failed to open file %s: %s", 
						 this->fileName.toStdString() % file.errorString().toStdString());
		}
		else {
			QString text = QTextStream(&file).readAll();
			PRINTB("Loaded design '%s'.", this->fileName.toStdString());
			editor->setPlainText(text);
		}
	}
	setCurrentOutput();
}

AbstractNode *MainWindow::find_root_tag(AbstractNode *n)
{
	BOOST_FOREACH (AbstractNode *v, n->children) {
		if (v->modinst->isRoot()) return v;
		if (AbstractNode *vroot = find_root_tag(v)) return vroot;
	}
	return NULL;
}

/*!
	Parse and evaluate the design => this->root_node

	Returns true if something was compiled, false if nothing was changed
  and the root_node was left untouched.
*/
bool MainWindow::compile(bool reload, bool procevents)
{
	if (!compileTopLevelDocument(reload)) return false;

  // Invalidate renderers before we kill the CSG tree
	this->glview->setRenderer(NULL);
	delete this->opencsgRenderer;
	this->opencsgRenderer = NULL;
	delete this->thrownTogetherRenderer;
	this->thrownTogetherRenderer = NULL;

	// Remove previous CSG tree
	delete this->absolute_root_node;
	this->absolute_root_node = NULL;

	this->root_raw_term.reset();
	this->root_norm_term.reset();

	delete this->root_chain;
	this->root_chain = NULL;

	this->highlight_terms.clear();
	delete this->highlights_chain;
	this->highlights_chain = NULL;

	this->background_terms.clear();
	delete this->background_chain;
	this->background_chain = NULL;

	this->root_node = NULL;
	this->tree.setRoot(NULL);

	if (this->root_module) {
		// Evaluate CSG tree
		PRINT("Compiling design (CSG Tree generation)...");
		if (procevents) QApplication::processEvents();
		
		AbstractNode::resetIndexCounter();
		this->root_inst = ModuleInstantiation();
		this->absolute_root_node = this->root_module->evaluate(&this->root_ctx, &this->root_inst);
		
		if (this->absolute_root_node) {
			// Do we have an explicit root node (! modifier)?
			if (!(this->root_node = find_root_tag(this->absolute_root_node))) {
				this->root_node = this->absolute_root_node;
			}
			// FIXME: Consider giving away ownership of root_node to the Tree, or use reference counted pointers
			this->tree.setRoot(this->root_node);
			// Dump the tree (to initialize caches).
			// FIXME: We shouldn't really need to do this explicitly..
			this->tree.getString(*this->root_node);
		}
	}

	if (!this->root_node) {
		if (parser_error_pos < 0) {
			PRINT("ERROR: Compilation failed! (no top level object found)");
		} else {
			PRINT("ERROR: Compilation failed!");
		}
		if (procevents) QApplication::processEvents();
	}

	return true;
}

/*!
	Generates CSG tree for OpenCSG evaluation.
	Assumes that the design has been parsed and evaluated (this->root_node is set)
*/
void MainWindow::compileCSG(bool procevents)
{
	assert(this->root_node);
	PRINT("Compiling design (CSG Products generation)...");
	if (procevents)
		QApplication::processEvents();

	// Main CSG evaluation
	QTime t;
	t.start();

	this->progresswidget = new ProgressWidget(this);
	connect(this->progresswidget, SIGNAL(requestShow()), this, SLOT(showProgress()));

	progress_report_prep(this->root_node, report_func, this);
	try {
		CGALEvaluator cgalevaluator(this->tree);
		PolySetCGALEvaluator psevaluator(cgalevaluator);
		CSGTermEvaluator csgrenderer(this->tree, &psevaluator);
		this->root_raw_term = csgrenderer.evaluateCSGTerm(*root_node, highlight_terms, background_terms);
		if (!root_raw_term) {
			PRINT("ERROR: CSG generation failed! (no top level object found)");
			if (procevents)
				QApplication::processEvents();
		}
		PolySetCache::instance()->print();
		CGALCache::instance()->print();
	}
	catch (const ProgressCancelException &e) {
		PRINT("CSG generation cancelled.");
	}
	progress_report_fin();
	this->statusBar()->removeWidget(this->progresswidget);
	delete this->progresswidget;
	this->progresswidget = NULL;

	if (root_raw_term) {
		PRINT("Compiling design (CSG Products normalization)...");
		if (procevents)
			QApplication::processEvents();
		
		size_t normalizelimit = 2 * Preferences::inst()->getValue("advanced/openCSGLimit").toUInt();
		CSGTermNormalizer normalizer(normalizelimit);
		this->root_norm_term = normalizer.normalize(this->root_raw_term);
		if (this->root_norm_term) {
			this->root_chain = new CSGChain();
			this->root_chain->import(this->root_norm_term);
		}
		else {
			this->root_chain = NULL;
			PRINT("WARNING: CSG normalization resulted in an empty tree");
			if (procevents)
				QApplication::processEvents();
		}
		
		if (highlight_terms.size() > 0)
		{
			PRINTB("Compiling highlights (%d CSG Trees)...", highlight_terms.size());
			if (procevents)
				QApplication::processEvents();
			
			highlights_chain = new CSGChain();
			for (unsigned int i = 0; i < highlight_terms.size(); i++) {
				highlight_terms[i] = normalizer.normalize(highlight_terms[i]);
				highlights_chain->import(highlight_terms[i]);
			}
		}
		
		if (background_terms.size() > 0)
		{
			PRINTB("Compiling background (%d CSG Trees)...", background_terms.size());
			if (procevents)
				QApplication::processEvents();
			
			background_chain = new CSGChain();
			for (unsigned int i = 0; i < background_terms.size(); i++) {
				background_terms[i] = normalizer.normalize(background_terms[i]);
				background_chain->import(background_terms[i]);
			}
		}

		if (this->root_chain && 
				(this->root_chain->polysets.size() > 
				 Preferences::inst()->getValue("advanced/openCSGLimit").toUInt())) {
			PRINTB("WARNING: Normalized tree has %d elements!", this->root_chain->polysets.size());
			PRINT("WARNING: OpenCSG rendering has been disabled.");
		}
		else {
			PRINTB("Normalized CSG tree has %d elements", 
						 (this->root_chain ? this->root_chain->polysets.size() : 0));
			this->opencsgRenderer = new OpenCSGRenderer(this->root_chain, 
																									this->highlights_chain, 
																									this->background_chain, 
																									this->glview->shaderinfo);
		}
		this->thrownTogetherRenderer = new ThrownTogetherRenderer(this->root_chain, 
																															this->highlights_chain, 
																															this->background_chain);
		PRINT("CSG generation finished.");
		int s = t.elapsed() / 1000;
		PRINTB("Total rendering time: %d hours, %d minutes, %d seconds", (s / (60*60)) % ((s / 60) % 60) % (s % 60));
		if (procevents)
			QApplication::processEvents();
	}
}

void MainWindow::actionNew()
{
#ifdef ENABLE_MDI
	new MainWindow(QString());
#else
	if (!maybeSave())
		return;

	setFileName("");
	editor->setPlainText("");
#endif
}

void MainWindow::actionOpen()
{
	QString new_filename = QFileDialog::getOpenFileName(this, "Open File", "",
																											"OpenSCAD Designs (*.scad *.csg)");
#ifdef ENABLE_MDI
	if (!new_filename.isEmpty()) {
		new MainWindow(new_filename);
	}
#else
	if (!new_filename.isEmpty()) {
		if (!maybeSave())
			return;
		
		setCurrentOutput();
		openFile(new_filename);
		clearCurrentOutput();
	}
#endif
}

void MainWindow::actionOpenRecent()
{
	QAction *action = qobject_cast<QAction *>(sender());

#ifdef ENABLE_MDI
	new MainWindow(action->data().toString());
#else
	if (!maybeSave())
		return;

	if (action) {
		openFile(action->data().toString());
	}
#endif
}

void MainWindow::clearRecentFiles()
{
	QSettings settings; // already set up properly via main.cpp
	QStringList files;
	settings.setValue("recentFileList", files);

	updateRecentFileActions();
}

void MainWindow::updateRecentFileActions()
{
	QSettings settings; // set up project and program properly in main.cpp
	QStringList files = settings.value("recentFileList").toStringList();

	int originalNumRecentFiles = files.size();

	// Remove any duplicate or empty entries from the list
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
	files.removeDuplicates();
#endif
	files.removeAll(QString());
	// Now remove any entries which do not exist
	for(int i = files.size()-1; i >= 0; --i) {
		QFileInfo fileInfo(files[i]);
		if (!QFile(fileInfo.absoluteFilePath()).exists())
			files.removeAt(i);
	}

	int numRecentFiles = qMin(files.size(),
														static_cast<int>(maxRecentFiles));

	for (int i = 0; i < numRecentFiles; ++i) {
		this->actionRecentFile[i]->setText(QFileInfo(files[i]).fileName());
		this->actionRecentFile[i]->setData(files[i]);
		this->actionRecentFile[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < maxRecentFiles; ++j)
		this->actionRecentFile[j]->setVisible(false);

	// If we had to prune the list, then save the cleaned list
	if (originalNumRecentFiles != numRecentFiles)
		settings.setValue("recentFileList", files);
}

void MainWindow::actionOpenExample()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		openFile(examplesdir + QDir::separator() + action->text());
	}
}

void MainWindow::actionSave()
{
	if (this->fileName.isEmpty()) {
		actionSaveAs();
	}
	else {
		setCurrentOutput();
		QFile file(this->fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
			PRINTB("Failed to open file for writing: %s (%s)", this->fileName.toStdString() % file.errorString().toStdString());
		}
		else {
			QTextStream(&file) << this->editor->toPlainText();
			PRINTB("Saved design '%s'.", this->fileName.toStdString());
			this->editor->setContentModified(false);
		}
		clearCurrentOutput();
		updateRecentFiles();
	}
}

void MainWindow::actionSaveAs()
{
	QString new_filename = QFileDialog::getSaveFileName(this, "Save File",
			this->fileName.isEmpty()?"Untitled.scad":this->fileName,
			"OpenSCAD Designs (*.scad)");
	if (!new_filename.isEmpty()) {
		if (QFileInfo(new_filename).suffix().isEmpty()) {
			new_filename.append(".scad");

			// Manual overwrite check since Qt doesn't do it, when using the
			// defaultSuffix property
			QFileInfo info(new_filename);
			if (info.exists()) {
				if (QMessageBox::warning(this, windowTitle(),
						 tr("%1 already exists.\nDo you want to replace it?").arg(info.fileName()),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
					return;
				}
			}
		}
		setFileName(new_filename);
		actionSave();
	}
}

void MainWindow::actionReload()
{
	if (checkEditorModified()) refreshDocument();
}

void MainWindow::hideEditor()
{
	QSettings settings;
	if (editActionHide->isChecked()) {
		editor->hide();
		settings.setValue("view/hideEditor",true);
	} else {
		editor->show();
		settings.setValue("view/hideEditor",false);
	}
}

void MainWindow::pasteViewportTranslation()
{
#ifdef _QCODE_EDIT_
	QDocumentCursor cursor = editor->cursor();
#else
	QTextCursor cursor = editor->textCursor();
#endif
	QString txt;
	txt.sprintf("[ %.2f, %.2f, %.2f ]", -this->glview->object_trans_x, -this->glview->object_trans_y, -this->glview->object_trans_z);
	cursor.insertText(txt);
}

void MainWindow::pasteViewportRotation()
{
#ifdef _QCODE_EDIT_
	QDocumentCursor cursor = editor->cursor();
#else
	QTextCursor cursor = editor->textCursor();
#endif
	QString txt;
	txt.sprintf("[ %.2f, %.2f, %.2f ]",
		fmodf(360 - this->glview->object_rot_x + 90, 360), fmodf(360 - this->glview->object_rot_y, 360), fmodf(360 - this->glview->object_rot_z, 360));
	cursor.insertText(txt);
}

void MainWindow::updateTemporalVariables()
{
	this->root_ctx.set_variable("$t", Value(this->e_tval->text().toDouble()));
	
	Value::VectorType vpt;
	vpt.push_back(Value(-this->glview->object_trans_x));
	vpt.push_back(Value(-this->glview->object_trans_y));
	vpt.push_back(Value(-this->glview->object_trans_z));
	this->root_ctx.set_variable("$vpt", Value(vpt));
	
	Value::VectorType vpr;
	vpr.push_back(Value(fmodf(360 - this->glview->object_rot_x + 90, 360)));
	vpr.push_back(Value(fmodf(360 - this->glview->object_rot_y, 360)));
	vpr.push_back(Value(fmodf(360 - this->glview->object_rot_z, 360)));
	root_ctx.set_variable("$vpr", Value(vpr));
}

bool MainWindow::fileChangedOnDisk()
{
	if (!this->fileName.isEmpty()) {
		struct stat st;
		memset(&st, 0, sizeof(struct stat));
		stat(this->fileName.toLocal8Bit(), &st);
		std::string newid = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

		if (newid != this->autoReloadId) {
			this->autoReloadId = newid;
			return true;
		}
	}
	return false;
}

// FIXME: The following two methods are duplicated in ModuleCache.cc - refactor
static bool is_modified(const std::string &filename, const time_t &mtime)
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.c_str(), &st);
	return (st.st_mtime > mtime);
}

bool MainWindow::includesChanged()
{
	if (this->root_module) {
		BOOST_FOREACH(const Module::IncludeContainer::value_type &item, this->root_module->includes) {
			if (is_modified(item.first, item.second)) return true;
		}
	}
	return false;
}

/*!
	If reload is true, does a timestamp check on the document and tries to reload it.
	Otherwise, just reparses the current document and any dependencies, updates the 
	GUI accordingly and populates this->root_module.

	Returns true if anything was compiled.
*/
bool MainWindow::compileTopLevelDocument(bool reload)
{
	bool shouldcompiletoplevel = !reload;

	if ((reload && fileChangedOnDisk() && checkEditorModified()) ||
			includesChanged()) {
		shouldcompiletoplevel = true;
		refreshDocument();
	}
	
	if (shouldcompiletoplevel) {
		console->clear();

		updateTemporalVariables();
		
		this->last_compiled_doc = editor->toPlainText();
		std::string fulltext = 
			this->last_compiled_doc.toStdString() + "\n" + commandline_commands;
		
		delete this->root_module;
		this->root_module = NULL;
		
		this->root_module = parse(fulltext.c_str(),
															this->fileName.isEmpty() ? 
															"" : 
															QFileInfo(this->fileName).absolutePath().toLocal8Bit(), 
															false);

		// Error highlighting
		delete this->highlighter;
		this->highlighter = NULL;
		
		if (!this->root_module) {
			this->highlighter = new Highlighter(editor->document());

			if (!animate_panel->isVisible()) {
#ifdef _QCODE_EDIT_
				QDocumentCursor cursor = editor->cursor();
				cursor.setPosition(parser_error_pos);
#else
				QTextCursor cursor = editor->textCursor();
				cursor.setPosition(parser_error_pos);
				editor->setTextCursor(cursor);
#endif
			}
		}
	}

	bool changed = shouldcompiletoplevel;
	if (this->root_module) {
		changed |= this->root_module->handleDependencies();
		if (changed) PRINTB("Module cache size: %d modules", ModuleCache::instance()->size());
	}

	return changed;
}

void MainWindow::checkAutoReload()
{
	if (!this->fileName.isEmpty()) actionReloadCompile();
}

void MainWindow::autoReloadSet(bool on)
{
	QSettings settings;
	settings.setValue("design/autoReload",designActionAutoReload->isChecked());
	if (on) {
		autoReloadId = "";
		autoReloadTimer->start(200);
	} else {
		autoReloadTimer->stop();
	}
}

bool MainWindow::checkEditorModified()
{
	if (editor->isContentModified()) {
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, "Application",
				"The document has been modified.\n"
				"Do you really want to reload the file?",
				QMessageBox::Yes | QMessageBox::No);
		if (ret != QMessageBox::Yes) {
			designActionAutoReload->setChecked(false);
			return false;
		}
	}
	return true;
}

void MainWindow::actionReloadCompile()
{
	if (GuiLocker::isLocked()) return;
	GuiLocker lock;
	setCurrentOutput();

	// PRINT("Parsing design (AST generation)...");
	// QApplication::processEvents();
	if (!compile(true, true)) return;
	if (this->root_node) compileCSG(true);

	// Go to non-CGAL view mode
	if (viewActionThrownTogether->isChecked()) {
		viewModeThrownTogether();
	}
	else {
#ifdef ENABLE_OPENCSG
		viewModeOpenCSG();
#else
		viewModeThrownTogether();
#endif
	}

	clearCurrentOutput();
}

void MainWindow::actionCompile()
{
	if (GuiLocker::isLocked()) return;
	GuiLocker lock;
	setCurrentOutput();
	console->clear();

	PRINT("Parsing design (AST generation)...");
	QApplication::processEvents();
	compile(false, !viewActionAnimate->isChecked());
	if (this->root_node) compileCSG(!viewActionAnimate->isChecked());

	// Go to non-CGAL view mode
	if (viewActionThrownTogether->isChecked()) {
		viewModeThrownTogether();
	}
	else {
#ifdef ENABLE_OPENCSG
		viewModeOpenCSG();
#else
		viewModeThrownTogether();
#endif
	}

	if (viewActionAnimate->isChecked() && e_dump->isChecked()) {
		QImage img = this->glview->grabFrameBuffer();
		QString filename;
		double s = this->e_fsteps->text().toDouble();
		double t = this->e_tval->text().toDouble();
		filename.sprintf("frame%05d.png", int(round(s*t)));
		img.save(filename, "PNG");
	}
	
	clearCurrentOutput();
}

#ifdef ENABLE_CGAL

void MainWindow::actionRenderCGAL()
{
	if (GuiLocker::isLocked()) return;
	GuiLocker lock;

	setCurrentOutput();
	console->clear();

	PRINT("Parsing design (AST generation)...");
	QApplication::processEvents();
	compile(false, true);

	if (!this->root_module || !this->root_node) {
		return;
	}

	this->glview->setRenderer(NULL);
	delete this->cgalRenderer;
	this->cgalRenderer = NULL;
	if (this->root_N) {
		delete this->root_N;
		this->root_N = NULL;
	}

	PRINT("Rendering Polygon Mesh using CGAL...");

	this->progresswidget = new ProgressWidget(this);
	connect(this->progresswidget, SIGNAL(requestShow()), this, SLOT(showProgress()));

	progress_report_prep(this->root_node, report_func, this);

	GuiLocker::lock(); // Will be unlocked in actionRenderCGALDone()
	this->cgalworker->start(this->tree);
}

void MainWindow::actionRenderCGALDone(CGAL_Nef_polyhedron *root_N)
{
	progress_report_fin();

	if (root_N) {
		PolySetCache::instance()->print();
		CGALCache::instance()->print();

		if (root_N->dim == 2) {
			PRINT("   Top level object is a 2D object:");
			PRINTB("   Empty:      %6s", (root_N->p2->is_empty() ? "yes" : "no"));
			PRINTB("   Plane:      %6s", (root_N->p2->is_plane() ? "yes" : "no"));
			PRINTB("   Vertices:   %6d", root_N->p2->explorer().number_of_vertices());
			PRINTB("   Halfedges:  %6d", root_N->p2->explorer().number_of_halfedges());
			PRINTB("   Edges:      %6d", root_N->p2->explorer().number_of_edges());
			PRINTB("   Faces:      %6d", root_N->p2->explorer().number_of_faces());
			PRINTB("   FaceCycles: %6d", root_N->p2->explorer().number_of_face_cycles());
			PRINTB("   ConnComp:   %6d", root_N->p2->explorer().number_of_connected_components());
		}

		if (root_N->dim == 3) {
			PRINT("   Top level object is a 3D object:");
			PRINTB("   Simple:     %6s", (root_N->p3->is_simple() ? "yes" : "no"));
			PRINTB("   Valid:      %6s", (root_N->p3->is_valid() ? "yes" : "no"));
			PRINTB("   Vertices:   %6d", root_N->p3->number_of_vertices());
			PRINTB("   Halfedges:  %6d", root_N->p3->number_of_halfedges());
			PRINTB("   Edges:      %6d", root_N->p3->number_of_edges());
			PRINTB("   Halffacets: %6d", root_N->p3->number_of_halffacets());
			PRINTB("   Facets:     %6d", root_N->p3->number_of_facets());
			PRINTB("   Volumes:    %6d", root_N->p3->number_of_volumes());
		}

		int s = this->progresswidget->elapsedTime() / 1000;
		PRINTB("Total rendering time: %d hours, %d minutes, %d seconds", (s / (60*60)) % ((s / 60) % 60) % (s % 60));

		this->root_N = root_N;
		if (!this->root_N->empty()) {
			this->cgalRenderer = new CGALRenderer(*this->root_N);
			// Go to CGAL view mode
			if (viewActionCGALGrid->isChecked()) {
				viewModeCGALGrid();
			}
			else {
				viewModeCGALSurface();
			}
			
			PRINT("Rendering finished.");
		}
		else {
			PRINT("WARNING: No top level geometry to render");
		}
	}

	this->statusBar()->removeWidget(this->progresswidget);
	delete this->progresswidget;
	this->progresswidget = NULL;
	clearCurrentOutput();

	GuiLocker::unlock();
}

#endif /* ENABLE_CGAL */

void MainWindow::actionDisplayAST()
{
	setCurrentOutput();
	QTextEdit *e = new QTextEdit(this);
	e->setWindowFlags(Qt::Window);
	e->setTabStopWidth(30);
	e->setWindowTitle("AST Dump");
	e->setReadOnly(true);
	if (root_module) {
		e->setPlainText(QString::fromStdString(root_module->dump("", "")));
	} else {
		e->setPlainText("No AST to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
	clearCurrentOutput();
}

void MainWindow::actionDisplayCSGTree()
{
	setCurrentOutput();
	QTextEdit *e = new QTextEdit(this);
	e->setWindowFlags(Qt::Window);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Tree Dump");
	e->setReadOnly(true);
	if (this->root_node) {
		e->setPlainText(QString::fromStdString(this->tree.getString(*this->root_node)));
	} else {
		e->setPlainText("No CSG to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
	clearCurrentOutput();
}

void MainWindow::actionDisplayCSGProducts()
{
	setCurrentOutput();
	QTextEdit *e = new QTextEdit(this);
	e->setWindowFlags(Qt::Window);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Products Dump");
	e->setReadOnly(true);
	e->setPlainText(QString("\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering chain:\n%3\n\n\nHighlights CSG rendering chain:\n%4\n\n\nBackground CSG rendering chain:\n%5\n").arg(root_raw_term ? QString::fromStdString(root_raw_term->dump()) : "N/A", root_norm_term ? QString::fromStdString(root_norm_term->dump()) : "N/A", this->root_chain ? QString::fromStdString(this->root_chain->dump()) : "N/A", highlights_chain ? QString::fromStdString(highlights_chain->dump()) : "N/A", background_chain ? QString::fromStdString(background_chain->dump()) : "N/A"));
	e->show();
	e->resize(600, 400);
	clearCurrentOutput();
}

#ifdef ENABLE_CGAL
void MainWindow::actionExportSTLorOFF(bool stl_mode)
#else
void MainWindow::actionExportSTLorOFF(bool)
#endif
{
	if (GuiLocker::isLocked()) return;
	GuiLocker lock;
#ifdef ENABLE_CGAL
	setCurrentOutput();

	if (!this->root_N) {
		PRINT("Nothing to export! Try building first (press F6).");
		clearCurrentOutput();
		return;
	}

	if (this->root_N->dim != 3) {
		PRINT("Current top level object is not a 3D object.");
		clearCurrentOutput();
		return;
	}

	if (!this->root_N->p3->is_simple()) {
		PRINT("Object isn't a valid 2-manifold! Modify your design. See http://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
		clearCurrentOutput();
		return;
	}

	QString suffix = stl_mode ? ".stl" : ".off";
	QString stl_filename = QFileDialog::getSaveFileName(this,
			stl_mode ? "Export STL File" : "Export OFF File", 
			this->fileName.isEmpty() ? "Untitled"+suffix : QFileInfo(this->fileName).baseName()+suffix,
			stl_mode ? "STL Files (*.stl)" : "OFF Files (*.off)");
	if (stl_filename.isEmpty()) {
		PRINTB("No filename specified. %s export aborted.", (stl_mode ? "STL" : "OFF"));
		clearCurrentOutput();
		return;
	}

	std::ofstream fstream(stl_filename.toUtf8());
	if (!fstream.is_open()) {
		PRINTB("Can't open file \"%s\" for export", stl_filename.toStdString());
	}
	else {
		if (stl_mode) export_stl(this->root_N, fstream);
		else export_off(this->root_N, fstream);
		fstream.close();

		PRINTB("%s export finished.", (stl_mode ? "STL" : "OFF"));
	}

	clearCurrentOutput();
#endif /* ENABLE_CGAL */
}

void MainWindow::actionExportSTL()
{
	actionExportSTLorOFF(true);
}

void MainWindow::actionExportOFF()
{
	actionExportSTLorOFF(false);
}

void MainWindow::actionExportDXF()
{
#ifdef ENABLE_CGAL
	setCurrentOutput();

	if (!this->root_N) {
		PRINT("Nothing to export! Try building first (press F6).");
		clearCurrentOutput();
		return;
	}

	if (this->root_N->dim != 2) {
		PRINT("Current top level object is not a 2D object.");
		clearCurrentOutput();
		return;
	}

	QString dxf_filename = QFileDialog::getSaveFileName(this,
			"Export DXF File", 
			this->fileName.isEmpty() ? "Untitled.dxf" : QFileInfo(this->fileName).baseName()+".dxf",
			"DXF Files (*.dxf)");
	if (dxf_filename.isEmpty()) {
		PRINT("No filename specified. DXF export aborted.");
		clearCurrentOutput();
		return;
	}

	std::ofstream fstream(dxf_filename.toUtf8());
	if (!fstream.is_open()) {
		PRINTB("Can't open file \"%s\" for export", dxf_filename.toStdString());
	}
	else {
		export_dxf(this->root_N, fstream);
		fstream.close();
		PRINT("DXF export finished.");
	}

	clearCurrentOutput();
#endif /* ENABLE_CGAL */
}

void MainWindow::actionExportCSG()
{
	setCurrentOutput();

	if (!this->root_node) {
		PRINT("Nothing to export. Please try compiling first...");
		clearCurrentOutput();
		return;
	}

	QString csg_filename = QFileDialog::getSaveFileName(this, "Export CSG File", 
																											this->fileName.isEmpty() ? "Untitled.csg" : QFileInfo(this->fileName).baseName()+".csg",
																											"CSG Files (*.csg)");
	if (csg_filename.isEmpty()) {
		PRINT("No filename specified. CSG export aborted.");
		clearCurrentOutput();
		return;
	}

	std::ofstream fstream(csg_filename.toUtf8());
	if (!fstream.is_open()) {
		PRINTB("Can't open file \"%s\" for export", csg_filename.toStdString());
	}
	else {
		fstream << this->tree.getString(*this->root_node) << "\n";
		fstream.close();
		PRINT("CSG export finished.");
	}

	clearCurrentOutput();
}

void MainWindow::actionExportImage()
{
	QImage img = this->glview->grabFrameBuffer();
	setCurrentOutput();

	QString img_filename = QFileDialog::getSaveFileName(this,
			"Export Image", "", "PNG Files (*.png)");
	if (img_filename.isEmpty()) {
		PRINT("No filename specified. Image export aborted.");
		clearCurrentOutput();
		return;
	}

	img.save(img_filename, "PNG");

	clearCurrentOutput();
}

void MainWindow::actionFlushCaches()
{
	PolySetCache::instance()->clear();
#ifdef ENABLE_CGAL
	CGALCache::instance()->clear();
#endif
	dxf_dim_cache.clear();
	dxf_cross_cache.clear();
	ModuleCache::instance()->clear();
}

void MainWindow::viewModeActionsUncheck()
{
	viewActionOpenCSG->setChecked(false);
#ifdef ENABLE_CGAL
	viewActionCGALSurfaces->setChecked(false);
	viewActionCGALGrid->setChecked(false);
#endif
	viewActionThrownTogether->setChecked(false);
}

#ifdef ENABLE_OPENCSG

/*!
	Go to the OpenCSG view mode.
	Falls back to thrown together mode if OpenCSG is not available
*/
void MainWindow::viewModeOpenCSG()
{
	if (this->glview->hasOpenCSGSupport()) {
		viewModeActionsUncheck();
		viewActionOpenCSG->setChecked(true);
		this->glview->setRenderer(this->opencsgRenderer ? (Renderer *)this->opencsgRenderer : (Renderer *)this->thrownTogetherRenderer);
	} else {
		viewModeThrownTogether();
	}
}

#endif /* ENABLE_OPENCSG */

#ifdef ENABLE_CGAL

void MainWindow::viewModeCGALSurface()
{
	viewModeActionsUncheck();
	viewActionCGALSurfaces->setChecked(true);
	this->glview->setShowFaces(true);
	this->glview->setRenderer(this->cgalRenderer);
	this->glview->updateGL();
}

void MainWindow::viewModeCGALGrid()
{
	viewModeActionsUncheck();
	viewActionCGALGrid->setChecked(true);
	this->glview->setShowFaces(false);
	this->glview->setRenderer(this->cgalRenderer);
}

#endif /* ENABLE_CGAL */

void MainWindow::viewModeThrownTogether()
{
	viewModeActionsUncheck();
	viewActionThrownTogether->setChecked(true);
	this->glview->setRenderer(this->thrownTogetherRenderer);
}

void MainWindow::viewModeShowEdges()
{
	QSettings settings;
	settings.setValue("view/showEdges",viewActionShowEdges->isChecked());
	this->glview->setShowEdges(viewActionShowEdges->isChecked());
	this->glview->updateGL();
}

void MainWindow::viewModeShowAxes()
{
	QSettings settings;
	settings.setValue("view/showAxes",viewActionShowAxes->isChecked());
	this->glview->setShowAxes(viewActionShowAxes->isChecked());
	this->glview->updateGL();
}

void MainWindow::viewModeShowCrosshairs()
{
	QSettings settings;
	settings.setValue("view/showCrosshairs",viewActionShowCrosshairs->isChecked());
	this->glview->setShowCrosshairs(viewActionShowCrosshairs->isChecked());
	this->glview->updateGL();
}

void MainWindow::viewModeAnimate()
{
	if (viewActionAnimate->isChecked()) {
		animate_panel->show();
		actionCompile();
		updatedFps();
	} else {
		animate_panel->hide();
		animate_timer->stop();
	}
}

void MainWindow::animateUpdateDocChanged()
{
	QString current_doc = editor->toPlainText();
	if (current_doc != last_compiled_doc)
		animateUpdate();
}

void MainWindow::animateUpdate()
{
	if (animate_panel->isVisible()) {
		bool fps_ok;
		double fps = this->e_fps->text().toDouble(&fps_ok);
		if (fps_ok && fps <= 0 && !animate_timer->isActive()) {
			animate_timer->stop();
			animate_timer->setSingleShot(true);
			animate_timer->setInterval(50);
			animate_timer->start();
		}
	}
}

void MainWindow::viewAngleTop()
{
	this->glview->object_rot_x = 90;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 0;
	this->glview->updateGL();
}

void MainWindow::viewAngleBottom()
{
	this->glview->object_rot_x = 270;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 0;
	this->glview->updateGL();
}

void MainWindow::viewAngleLeft()
{
	this->glview->object_rot_x = 0;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 90;
	this->glview->updateGL();
}

void MainWindow::viewAngleRight()
{
	this->glview->object_rot_x = 0;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 270;
	this->glview->updateGL();
}

void MainWindow::viewAngleFront()
{
	this->glview->object_rot_x = 0;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 0;
	this->glview->updateGL();
}

void MainWindow::viewAngleBack()
{
	this->glview->object_rot_x = 0;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = 180;
	this->glview->updateGL();
}

void MainWindow::viewAngleDiagonal()
{
	this->glview->object_rot_x = 35;
	this->glview->object_rot_y = 0;
	this->glview->object_rot_z = -25;
	this->glview->updateGL();
}

void MainWindow::viewCenter()
{
	this->glview->object_trans_x = 0;
	this->glview->object_trans_y = 0;
	this->glview->object_trans_z = 0;
	this->glview->updateGL();
}

void MainWindow::viewPerspective()
{
	QSettings settings;
	settings.setValue("view/orthogonalProjection",false);
	viewActionPerspective->setChecked(true);
	viewActionOrthogonal->setChecked(false);
	this->glview->setOrthoMode(false);
	this->glview->updateGL();
}

void MainWindow::viewOrthogonal()
{
	QSettings settings;
	settings.setValue("view/orthogonalProjection",true);
	viewActionPerspective->setChecked(false);
	viewActionOrthogonal->setChecked(true);
	this->glview->setOrthoMode(true);
	this->glview->updateGL();
}

void MainWindow::hideConsole()
{
	QSettings settings;
	if (viewActionHide->isChecked()) {
		console->hide();
		settings.setValue("view/hideConsole",true);
	} else {
		console->show();
		settings.setValue("view/hideConsole",false);
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	setCurrentOutput();
	const QList<QUrl> urls = event->mimeData()->urls();
	for (int i = 0; i < urls.size(); i++) {
		if (urls[i].scheme() != "file")
			continue;
		openFile(urls[i].toLocalFile());
	}
	clearCurrentOutput();
}

void
MainWindow::helpAbout()
{
	qApp->setWindowIcon(QApplication::windowIcon());
	AboutDialog *dialog = new AboutDialog(this);
	dialog->exec();
	//QMessageBox::information(this, "About OpenSCAD", QString(helptitle) + QString(copyrighttext));
}

void
MainWindow::helpHomepage()
{
	QDesktopServices::openUrl(QUrl("http://openscad.org/"));
}

void
MainWindow::helpManual()
{
	QDesktopServices::openUrl(QUrl("http://en.wikibooks.org/wiki/OpenSCAD_User_Manual"));
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
void MainWindow::helpLibrary()
{
	QString libinfo;
	libinfo.sprintf("Boost version: %s\n"
									"Eigen version: %d.%d.%d\n"
									"CGAL version: %s\n"
									"OpenCSG version: %s\n\n",
									BOOST_LIB_VERSION,
									EIGEN_WORLD_VERSION, EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION,
									TOSTRING(CGAL_VERSION),
									OPENCSG_VERSION_STRING);

	if (!this->openglbox) {
		this->openglbox = new QMessageBox(QMessageBox::Information, 
																			"OpenGL Info", "Detailed Library Info",
																			QMessageBox::Ok, this);
		
	}

	this->openglbox->setDetailedText(libinfo + this->glview->getRendererInfo());
	this->openglbox->show();
}

/*!
	FIXME: In MDI mode, should this be called on both reload functions?
 */
bool
MainWindow::maybeSave()
{
	if (editor->isContentModified()) {
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, "Application",
				"The document has been modified.\n"
				"Do you want to save your changes?",
				QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (ret == QMessageBox::Save) {
			actionSave();
			return true; // FIXME: Should return false on error
		}
		else if (ret == QMessageBox::Cancel) {
			return false;
		}
	}
	return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (maybeSave()) {
		QSettings settings;
		settings.setValue("window/size", size());
		settings.setValue("window/position", pos());
		settings_setValueList("window/splitter1sizes",splitter1->sizes());
		settings_setValueList("window/splitter2sizes",splitter2->sizes());
		event->accept();
	} else {
		event->ignore();
	}
}

void
MainWindow::preferences()
{
	Preferences::inst()->show();
	Preferences::inst()->activateWindow();
	Preferences::inst()->raise();
}

void MainWindow::setFont(const QString &family, uint size)
{
	QFont font;
	if (!family.isEmpty()) font.setFamily(family);
	else font.setFixedPitch(true);
	if (size > 0)	font.setPointSize(size);
	font.setStyleHint(QFont::TypeWriter);
	editor->setFont(font);
}

void MainWindow::quit()
{
	QCloseEvent ev;
	QApplication::sendEvent(QApplication::instance(), &ev);
	if (ev.isAccepted()) QApplication::instance()->quit();
}

void MainWindow::consoleOutput(const std::string &msg, void *userdata)
{
	// Invoke the append function in the main thread in case the output
  // originates in a worker thread.
	MainWindow *thisp = static_cast<MainWindow*>(userdata);
	QMetaObject::invokeMethod(thisp->console, "append", Qt::QueuedConnection,
														 Q_ARG(QString, QString::fromStdString(msg)));
}

void MainWindow::setCurrentOutput()
{
	set_output_handler(&MainWindow::consoleOutput, this);
}

void MainWindow::clearCurrentOutput()
{
	set_output_handler(NULL, NULL);
}

void MainWindow::openCSGSettingsChanged()
{
#ifdef ENABLE_OPENCSG
	OpenCSG::setOption(OpenCSG::AlgorithmSetting, Preferences::inst()->getValue("advanced/forceGoldfeather").toBool() ? OpenCSG::Goldfeather : OpenCSG::Automatic);
#endif
}
