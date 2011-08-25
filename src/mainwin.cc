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

#include "MainWindow.h"
#include "openscad.h" // examplesdir
#include "Preferences.h"
#include "printutils.h"
#include "node.h"
#include "polyset.h"
#include "csgterm.h"
#include "highlighter.h"
#include "grid.h"
#include "dxfdata.h"
#include "dxfdim.h"
#include "export.h"
#include "builtin.h"
#include "dxftess.h"
#include "progress.h"
#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#endif
#ifdef USE_PROGRESSWIDGET
#include "ProgressWidget.h"
#endif
#include "ThrownTogetherRenderer.h"

#include <QMenu>
#include <QTime>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QProgressDialog>
#include <QProgressBar>
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
#ifdef _QCODE_EDIT_
#include "qdocument.h"
#include "qformatscheme.h"
#include "qlanguagefactory.h"
#endif

#ifdef ENABLE_CGAL

#include "cgalrenderer.h"

#endif // ENABLE_CGAL

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

static char helptitle[] =
	"OpenSCAD "
	QUOTED(OPENSCAD_VERSION)
  " (www.openscad.org)\n";
static char copyrighttext[] =
	"Copyright (C) 2009-2011 Marius Kintel <marius@kintel.net> and Clifford Wolf <clifford@clifford.at>\n"
	"\n"
	"This program is free software; you can redistribute it and/or modify"
	"it under the terms of the GNU General Public License as published by"
	"the Free Software Foundation; either version 2 of the License, or"
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
{
	setupUi(this);

	root_ctx.functions_p = &builtin_functions;
	root_ctx.modules_p = &builtin_modules;
	root_ctx.set_variable("$fn", Value(0.0));
	root_ctx.set_variable("$fs", Value(1.0));
	root_ctx.set_variable("$fa", Value(12.0));
	root_ctx.set_variable("$t", Value(0.0));

	Value zero3;
	zero3.type = Value::VECTOR;
	zero3.vec.append(new Value(0.0));
	zero3.vec.append(new Value(0.0));
	zero3.vec.append(new Value(0.0));
	root_ctx.set_variable("$vpt", zero3);
	root_ctx.set_variable("$vpr", zero3);

	root_module = NULL;
	absolute_root_node = NULL;
	root_raw_term = NULL;
	root_norm_term = NULL;
	root_chain = NULL;
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
	setFont("", 0); // Init default font

	screen->statusLabel = new QLabel(this);
	statusBar()->addWidget(screen->statusLabel);

	animate_timer = new QTimer(this);
	connect(animate_timer, SIGNAL(timeout()), this, SLOT(updateTVal()));

	autoReloadTimer = new QTimer(this);
	autoReloadTimer->setSingleShot(false);
	connect(autoReloadTimer, SIGNAL(timeout()), this, SLOT(checkAutoReload()));

	connect(e_tval, SIGNAL(textChanged(QString)), this, SLOT(actionCompile()));
	connect(e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedFps()));

	animate_panel->hide();

	// File menu
	connect(this->fileActionNew, SIGNAL(triggered()), this, SLOT(actionNew()));
	connect(this->fileActionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(this->fileActionSave, SIGNAL(triggered()), this, SLOT(actionSave()));
	connect(this->fileActionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs()));
	connect(this->fileActionReload, SIGNAL(triggered()), this, SLOT(actionReload()));
	connect(this->fileActionQuit, SIGNAL(triggered()), this, SLOT(quit()));
#ifndef __APPLE__
	this->fileActionSave->setShortcut(QKeySequence(Qt::Key_F2));
	this->fileActionReload->setShortcut(QKeySequence(Qt::Key_F3));
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
	connect(this->designActionExportImage, SIGNAL(triggered()), this, SLOT(actionExportImage()));
	connect(this->designActionFlushCaches, SIGNAL(triggered()), this, SLOT(actionFlushCaches()));

	// View menu
#ifndef ENABLE_OPENCSG
	this->viewActionOpenCSG->setVisible(false);
#else
	connect(this->viewActionOpenCSG, SIGNAL(triggered()), this, SLOT(viewModeOpenCSG()));
	if (!screen->hasOpenCSGSupport()) {
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


	console->setReadOnly(true);
	setCurrentOutput();

	PRINT(helptitle);
	PRINT(copyrighttext);
	PRINT("");

	if (!filename.isEmpty()) {
		openFile(filename);
	} else {
		setFileName("");
	}

	connect(editor->document(), SIGNAL(contentsChanged()), this, SLOT(animateUpdateDocChanged()));
#ifdef _QCODE_EDIT_
	connect(editor, SIGNAL(contentModified(bool)), this, SLOT(setWindowModified(bool)));
	connect(editor, SIGNAL(contentModified(bool)), fileActionSave, SLOT(setEnabled(bool)));
#else
	connect(editor->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
	connect(editor->document(), SIGNAL(modificationChanged(bool)), fileActionSave, SLOT(setEnabled(bool)));
#endif
	connect(screen, SIGNAL(doAnimateUpdate()), this, SLOT(animateUpdate()));

	connect(Preferences::inst(), SIGNAL(requestRedraw()), this->screen, SLOT(updateGL()));
	connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)), 
					this, SLOT(setFont(const QString&,uint)));
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
	if (settings.value("design/autoReload").toBool())
		designActionAutoReload->setChecked(true);
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

#ifdef USE_PROGRESSWIDGET
void MainWindow::showProgress()
{
	this->statusBar()->addPermanentWidget(qobject_cast<ProgressWidget*>(sender()));
}
#endif

static void report_func(const class AbstractNode*, void *vp, int mark)
{
#ifdef USE_PROGRESSWIDGET
	ProgressWidget *pw = static_cast<ProgressWidget*>(vp);
	int v = (int)((mark*100.0) / progress_report_count);
	pw->setValue(v < 100 ? v : 99);
	QApplication::processEvents();
	if (pw->wasCanceled()) throw ProgressCancelException();
#else
	QProgressDialog *pd = static_cast<QProgressDialog*>(vp);
	int v = (int)((mark*100.0) / progress_report_count);
	pd->setValue(v < 100 ? v : 99);
	QString label;
	label.sprintf("Rendering Polygon Mesh (%d/%d)", mark, progress_report_count);
	pd->setLabelText(label);
	QApplication::processEvents();
	if (pd->wasCanceled()) throw ProgressCancelException();
#endif
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

	load();
}

void
MainWindow::setFileName(const QString &filename)
{
	if (filename.isEmpty()) {
		this->fileName.clear();
		this->root_ctx.document_path = currentdir;
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
			QSettings settings; // already set up properly via main.cpp
			QStringList files = settings.value("recentFileList").toStringList();
			files.removeAll(this->fileName);
			files.prepend(this->fileName);
			while (files.size() > maxRecentFiles)
				files.removeLast();
			settings.setValue("recentFileList", files);
		} else {
			this->fileName = fileinfo.fileName();
		}
		
		this->root_ctx.document_path = fileinfo.dir().absolutePath();
		QDir::setCurrent(fileinfo.dir().absolutePath());
	}

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
	double fps = e_fps->text().toDouble(&fps_ok);
	animate_timer->stop();
	if (fps_ok && fps > 0) {
		animate_timer->setSingleShot(false);
		animate_timer->setInterval(int(1000 / e_fps->text().toDouble()));
		animate_timer->start();
	}
}

void MainWindow::updateTVal()
{
	bool fps_ok;
	double fps = e_fps->text().toDouble(&fps_ok);
	if (fps_ok) {
		if (fps <= 0) {
			actionCompile();
		} else {
			double s = e_fsteps->text().toDouble();
			double t = e_tval->text().toDouble() + 1/s;
			QString txt;
			txt.sprintf("%.5f", t >= 1.0 ? 0.0 : t);
			e_tval->setText(txt);
		}
	}
}

void MainWindow::load()
{
	setCurrentOutput();
	if (!this->fileName.isEmpty()) {
		QFile file(this->fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			PRINTA("Failed to open file: %1 (%2)", this->fileName, file.errorString());
		}
		else {
			QString text = QTextStream(&file).readAll();
			PRINTA("Loaded design `%1'.", this->fileName);
			editor->setPlainText(text);
		}
	}
	setCurrentOutput();
}

AbstractNode *MainWindow::find_root_tag(AbstractNode *n)
{
	foreach(AbstractNode *v, n->children) {
		if (v->modinst->tag_root) return v;
		if (AbstractNode *vroot = find_root_tag(v)) return vroot;
	}
	return NULL;
}

/*!
	Parse and evaluate the design -> this->root_node
*/
void MainWindow::compile(bool procevents)
{
	PRINT("Parsing design (AST generation)...");
	if (procevents)
		QApplication::processEvents();

  // Invalidate renderers before we kill the CSG tree
	screen->setRenderer(NULL);
	if (this->opencsgRenderer) {
		delete this->opencsgRenderer;
		this->opencsgRenderer = NULL;
	}
	if (this->thrownTogetherRenderer) {
		delete this->thrownTogetherRenderer;
		this->thrownTogetherRenderer = NULL;
	}

	// Remove previous CSG tree
	if (root_module) {
		delete root_module;
		root_module = NULL;
	}

	if (absolute_root_node) {
		delete absolute_root_node;
		absolute_root_node = NULL;
	}

	if (root_raw_term) {
		root_raw_term->unlink();
		root_raw_term = NULL;
	}

	if (root_norm_term) {
		root_norm_term->unlink();
		root_norm_term = NULL;
	}

	if (root_chain) {
		delete root_chain;
		root_chain = NULL;
	}

	foreach(CSGTerm *v, highlight_terms) {
		v->unlink();
	}
	highlight_terms.clear();
	if (highlights_chain) {
		delete highlights_chain;
		highlights_chain = NULL;
	}
	foreach(CSGTerm *v, background_terms) {
		v->unlink();
	}
	background_terms.clear();
	if (background_chain) {
		delete background_chain;
		background_chain = NULL;
	}
	root_node = NULL;

	// Initialize special variables
	root_ctx.set_variable("$t", Value(e_tval->text().toDouble()));

	Value vpt;
	vpt.type = Value::VECTOR;
	vpt.vec.append(new Value(-screen->object_trans_x));
	vpt.vec.append(new Value(-screen->object_trans_y));
	vpt.vec.append(new Value(-screen->object_trans_z));
	root_ctx.set_variable("$vpt", vpt);

	Value vpr;
	vpr.type = Value::VECTOR;
	vpr.vec.append(new Value(fmodf(360 - screen->object_rot_x + 90, 360)));
	vpr.vec.append(new Value(fmodf(360 - screen->object_rot_y, 360)));
	vpr.vec.append(new Value(fmodf(360 - screen->object_rot_z, 360)));
	root_ctx.set_variable("$vpr", vpr);

	// Parse
	last_compiled_doc = editor->toPlainText();
	root_module = parse((last_compiled_doc + "\n" + commandline_commands).toAscii().data(), this->fileName.isEmpty() ? "" : QFileInfo(this->fileName).absolutePath().toLocal8Bit(), false);

	// Error highlighting
	if (highlighter) {
		delete highlighter;
		highlighter = NULL;
	}
	if (parser_error_pos >= 0) {
		highlighter = new Highlighter(editor->document());
	}

	if (!root_module) {
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
		goto fail;
	}

	// Evaluate CSG tree
	PRINT("Compiling design (CSG Tree generation)...");
	if (procevents)
		QApplication::processEvents();

	AbstractNode::resetIndexCounter();
	root_inst = ModuleInstantiation();
	absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);

	if (!absolute_root_node)
		goto fail;

	// Do we have an explicit root node (! modifier)?
	if (!(this->root_node = find_root_tag(absolute_root_node))) {
		this->root_node = absolute_root_node;
	}
	root_node->dump("");

	if (1) {
		PRINT("Compilation finished.");
		if (procevents)
			QApplication::processEvents();
	} else {
fail:
		if (parser_error_pos < 0) {
			PRINT("ERROR: Compilation failed! (no top level object found)");
		} else {
			int line = 1;
			QByteArray pb = last_compiled_doc.toAscii();
			char *p = pb.data();
			for (int i = 0; i < parser_error_pos; i++) {
				if (p[i] == '\n')
					line++;
				if (p[i] == 0) {
					line = -1;
					break;
				}
			}
			PRINTF("ERROR: Compilation failed! (parser error in line %d)", line);
		}
		if (procevents)
			QApplication::processEvents();
	}
}

/*!
	Generates CSG tree for OpenCSG evaluation.
	Assumes that the design has been parsed and evaluated
*/
void MainWindow::compileCSG(bool procevents)
{
	assert(this->root_node);
	PRINT("Compiling design (CSG Products generation)...");
	if (procevents)
		QApplication::processEvents();

	double m[20];

	for (int i = 0; i < 16; i++)
		m[i] = i % 5 == 0 ? 1.0 : 0.0;
	for (int i = 16; i < 20; i++)
		m[i] = -1;

	// Main CSG evaluation
	QTime t;
	t.start();

#ifdef USE_PROGRESSWIDGET
	ProgressWidget *pd = new ProgressWidget(this);
	pd->setRange(0, 100);
	pd->setValue(0);
	connect(pd, SIGNAL(requestShow()), this, SLOT(showProgress()));
#else
	QProgressDialog *pd = new QProgressDialog("Rendering CSG products...", "Cancel", 0, 100);
	pd->setRange(0, 100);
	pd->setValue(0);
	pd->setAutoClose(false);
	pd->show();
#endif
	QApplication::processEvents();

	progress_report_prep(root_node, report_func, pd);
	try {
		root_raw_term = root_node->render_csg_term(m, &highlight_terms, &background_terms);
		if (!root_raw_term) {
			PRINT("ERROR: CSG generation failed! (no top level object found)");
			if (procevents)
				QApplication::processEvents();
		}
	}
	catch (ProgressCancelException e) {
		PRINT("CSG generation cancelled.");
	}
	progress_report_fin();
#ifdef USE_PROGRESSWIDGET
	this->statusBar()->removeWidget(pd);
#endif
	delete pd;

	if (root_raw_term) {
		PRINT("Compiling design (CSG Products normalization)...");
		if (procevents)
			QApplication::processEvents();
		
		root_norm_term = root_raw_term->link();
		
		// CSG normalization
		while (1) {
			CSGTerm *n = root_norm_term->normalize();
			root_norm_term->unlink();
			if (root_norm_term == n)
				break;
			root_norm_term = n;
		}
		
		assert(root_norm_term);

		root_chain = new CSGChain();
		root_chain->import(root_norm_term);
		
		if (highlight_terms.size() > 0)
		{
			PRINTF("Compiling highlights (%d CSG Trees)...", highlight_terms.size());
			if (procevents)
				QApplication::processEvents();
			
			highlights_chain = new CSGChain();
			for (int i = 0; i < highlight_terms.size(); i++) {
				while (1) {
					CSGTerm *n = highlight_terms[i]->normalize();
					highlight_terms[i]->unlink();
					if (highlight_terms[i] == n)
						break;
					highlight_terms[i] = n;
				}
				highlights_chain->import(highlight_terms[i]);
			}
		}
		
		if (background_terms.size() > 0)
		{
			PRINTF("Compiling background (%d CSG Trees)...", background_terms.size());
			if (procevents)
				QApplication::processEvents();
			
			background_chain = new CSGChain();
			for (int i = 0; i < background_terms.size(); i++) {
				while (1) {
					CSGTerm *n = background_terms[i]->normalize();
					background_terms[i]->unlink();
					if (background_terms[i] == n)
						break;
					background_terms[i] = n;
				}
				background_chain->import(background_terms[i]);
			}
		}

		if (root_chain->polysets.size() > 1000) {
			PRINTF("WARNING: Normalized tree has %d elements!", root_chain->polysets.size());
			PRINTF("WARNING: OpenCSG rendering has been disabled.");
		}
		else {
			this->opencsgRenderer = new OpenCSGRenderer(this->root_chain, 
																									this->highlights_chain, 
																									this->background_chain, 
																									this->screen->shaderinfo);
		}
		this->thrownTogetherRenderer = new ThrownTogetherRenderer(this->root_chain, 
																															this->highlights_chain, 
																															this->background_chain);
		
		PRINT("CSG generation finished.");
		int s = t.elapsed() / 1000;
		PRINTF("Total rendering time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);
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
	QString new_filename = QFileDialog::getOpenFileName(this, "Open File", "", "OpenSCAD Designs (*.scad)");
#ifdef ENABLE_MDI
	new MainWindow(new_filename);
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
			PRINTA("Failed to open file for writing: %1 (%2)", this->fileName, file.errorString());
		}
		else {
			QTextStream(&file) << this->editor->toPlainText();
			PRINTA("Saved design `%1'.", this->fileName);
			this->editor->setContentModified(false);
		}
		clearCurrentOutput();
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
	if (checkModified()) load();
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
	txt.sprintf("[ %.2f, %.2f, %.2f ]", -screen->object_trans_x, -screen->object_trans_y, -screen->object_trans_z);
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
		fmodf(360 - screen->object_rot_x + 90, 360), fmodf(360 - screen->object_rot_y, 360), fmodf(360 - screen->object_rot_z, 360));
	cursor.insertText(txt);
}

void MainWindow::checkAutoReload()
{
	QString new_stinfo;
	QFileInfo finfo(this->fileName);
	new_stinfo = QString::number(finfo.size()) + QString::number(finfo.lastModified().toTime_t());
	if (new_stinfo != autoReloadInfo)
		actionReloadCompile();
	autoReloadInfo = new_stinfo;
}

void MainWindow::autoReloadSet(bool on)
{
	QSettings settings;
	settings.setValue("design/autoReload",designActionAutoReload->isChecked());
	if (on) {
		autoReloadInfo = QString();
		autoReloadTimer->start(200);
	} else {
		autoReloadTimer->stop();
	}
}

bool MainWindow::checkModified()
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
	if (!checkModified()) return;

	console->clear();

	load();

	setCurrentOutput();
	compile(true);
	if (this->root_node) compileCSG(true);

#ifdef ENABLE_OPENCSG
	if (!(viewActionOpenCSG->isVisible() && viewActionOpenCSG->isChecked()) &&
			!viewActionThrownTogether->isChecked()) {
		viewModeOpenCSG();
	}
	else
#endif
	{
		screen->updateGL();
	}
	clearCurrentOutput();
}

void MainWindow::actionCompile()
{
	setCurrentOutput();
	console->clear();

	compile(!viewActionAnimate->isChecked());
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
		QImage img = screen->grabFrameBuffer();
		QString filename;
		double s = e_fsteps->text().toDouble();
		double t = e_tval->text().toDouble();
		filename.sprintf("frame%05d.png", int(round(s*t)));
		img.save(filename, "PNG");
	}
	
	clearCurrentOutput();
}

#ifdef ENABLE_CGAL

void MainWindow::actionRenderCGAL()
{
	setCurrentOutput();
	console->clear();

	compile(true);

	if (!root_module || !root_node)
		return;

	this->screen->setRenderer(NULL);
	delete this->cgalRenderer;
	this->cgalRenderer = NULL;
	if (this->root_N) {
		delete this->root_N;
		this->root_N = NULL;
	}

	PRINT("Rendering Polygon Mesh using CGAL...");
	QApplication::processEvents();

	QTime t;
	t.start();


#ifdef USE_PROGRESSWIDGET
	ProgressWidget *pd = new ProgressWidget(this);
	pd->setRange(0, 100);
	pd->setValue(0);
	connect(pd, SIGNAL(requestShow()), this, SLOT(showProgress()));
#else
	QProgressDialog *pd = new QProgressDialog("Rendering Polygon Mesh using CGAL...", "Cancel", 0, 100);
	pd->setRange(0, 100);
	pd->setValue(0);
	pd->setAutoClose(false);
	pd->show();
#endif

	QApplication::processEvents();

	progress_report_prep(root_node, report_func, pd);
	try {
		this->root_N = new CGAL_Nef_polyhedron(root_node->render_cgal_nef_polyhedron());
	}
	catch (ProgressCancelException e) {
		PRINT("Rendering cancelled.");
	}
	progress_report_fin();

	if (this->root_N)
	{
		PRINTF("Number of vertices currently in CGAL cache: %d", AbstractNode::cgal_nef_cache.totalCost());
		PRINTF("Number of objects currently in CGAL cache: %d", AbstractNode::cgal_nef_cache.size());
		QApplication::processEvents();

		if (this->root_N->dim == 2) {
			PRINTF("   Top level object is a 2D object:");
			QApplication::processEvents();
			PRINTF("   Empty:      %6s", this->root_N->p2.is_empty() ? "yes" : "no");
			QApplication::processEvents();
			PRINTF("   Plane:      %6s", this->root_N->p2.is_plane() ? "yes" : "no");
			QApplication::processEvents();
			PRINTF("   Vertices:   %6d", (int)this->root_N->p2.explorer().number_of_vertices());
			QApplication::processEvents();
			PRINTF("   Halfedges:  %6d", (int)this->root_N->p2.explorer().number_of_halfedges());
			QApplication::processEvents();
			PRINTF("   Edges:      %6d", (int)this->root_N->p2.explorer().number_of_edges());
			QApplication::processEvents();
			PRINTF("   Faces:      %6d", (int)this->root_N->p2.explorer().number_of_faces());
			QApplication::processEvents();
			PRINTF("   FaceCycles: %6d", (int)this->root_N->p2.explorer().number_of_face_cycles());
			QApplication::processEvents();
			PRINTF("   ConnComp:   %6d", (int)this->root_N->p2.explorer().number_of_connected_components());
			QApplication::processEvents();
		}

		if (this->root_N->dim == 3) {
			PRINTF("   Top level object is a 3D object:");
			PRINTF("   Simple:     %6s", this->root_N->p3.is_simple() ? "yes" : "no");
			QApplication::processEvents();
			PRINTF("   Valid:      %6s", this->root_N->p3.is_valid() ? "yes" : "no");
			QApplication::processEvents();
			PRINTF("   Vertices:   %6d", (int)this->root_N->p3.number_of_vertices());
			QApplication::processEvents();
			PRINTF("   Halfedges:  %6d", (int)this->root_N->p3.number_of_halfedges());
			QApplication::processEvents();
			PRINTF("   Edges:      %6d", (int)this->root_N->p3.number_of_edges());
			QApplication::processEvents();
			PRINTF("   Halffacets: %6d", (int)this->root_N->p3.number_of_halffacets());
			QApplication::processEvents();
			PRINTF("   Facets:     %6d", (int)this->root_N->p3.number_of_facets());
			QApplication::processEvents();
			PRINTF("   Volumes:    %6d", (int)this->root_N->p3.number_of_volumes());
			QApplication::processEvents();
		}

		int s = t.elapsed() / 1000;
		PRINTF("Total rendering time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);

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

#ifdef USE_PROGRESSWIDGET
	this->statusBar()->removeWidget(pd);
#endif
	delete pd;
	clearCurrentOutput();
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
		e->setPlainText(root_module->dump("", ""));
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
	if (root_node) {
		e->setPlainText(root_node->dump(""));
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
	e->setPlainText(QString("\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering chain:\n%3\n\n\nHighlights CSG rendering chain:\n%4\n\n\nBackground CSG rendering chain:\n%5\n").arg(root_raw_term ? root_raw_term->dump() : "N/A", root_norm_term ? root_norm_term->dump() : "N/A", root_chain ? root_chain->dump() : "N/A", highlights_chain ? highlights_chain->dump() : "N/A", background_chain ? background_chain->dump() : "N/A"));
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

	if (!this->root_N->p3.is_simple()) {
		PRINT("Object isn't a valid 2-manifold! Modify your design..");
		clearCurrentOutput();
		return;
	}

	QString stl_filename = QFileDialog::getSaveFileName(this,
			stl_mode ? "Export STL File" : "Export OFF File", "",
			stl_mode ? "STL Files (*.stl)" : "OFF Files (*.off)");
	if (stl_filename.isEmpty()) {
		PRINTF("No filename specified. %s export aborted.", stl_mode ? "STL" : "OFF");
		clearCurrentOutput();
		return;
	}

	QProgressDialog *pd = new QProgressDialog(
			stl_mode ? "Exporting object to STL file..." : "Exporting object to OFF file...",
			QString(), 0, this->root_N->p3.number_of_facets() + 1);
	pd->setValue(0);
	pd->setAutoClose(false);
	pd->show();
	QApplication::processEvents();

	if (stl_mode)
		export_stl(this->root_N, stl_filename, pd);
	else
		export_off(this->root_N, stl_filename, pd);

	PRINTF("%s export finished.", stl_mode ? "STL" : "OFF");

	delete pd;

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

	QString stl_filename = QFileDialog::getSaveFileName(this,
			"Export DXF File", "", "DXF Files (*.dxf)");
	if (stl_filename.isEmpty()) {
		PRINTF("No filename specified. DXF export aborted.");
		clearCurrentOutput();
		return;
	}

	export_dxf(this->root_N, stl_filename, NULL);
	PRINTF("DXF export finished.");

	clearCurrentOutput();
#endif /* ENABLE_CGAL */
}

void MainWindow::actionExportImage()
{
	QImage img = screen->grabFrameBuffer();
	setCurrentOutput();

	QString img_filename = QFileDialog::getSaveFileName(this,
			"Export Image", "", "PNG Files (*.png)");
	if (img_filename.isEmpty()) {
		PRINTF("No filename specified. Image export aborted.");
		clearCurrentOutput();
		return;
	}

	img.save(img_filename, "PNG");

	clearCurrentOutput();
}

void MainWindow::actionFlushCaches()
{
	PolySet::ps_cache.clear();
#ifdef ENABLE_CGAL
	AbstractNode::cgal_nef_cache.clear();
#endif
	dxf_dim_cache.clear();
	dxf_cross_cache.clear();
	Module::libs_cache.clear();
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
	if (screen->hasOpenCSGSupport()) {
		viewModeActionsUncheck();
		viewActionOpenCSG->setChecked(true);
		screen->setRenderer(this->opencsgRenderer ? (Renderer *)this->opencsgRenderer : (Renderer *)this->thrownTogetherRenderer);
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
	screen->setShowFaces(true);
	screen->setRenderer(this->cgalRenderer);
	screen->updateGL();
}

void MainWindow::viewModeCGALGrid()
{
	viewModeActionsUncheck();
	viewActionCGALGrid->setChecked(true);
	screen->setShowFaces(false);
	screen->setRenderer(this->cgalRenderer);
}

#endif /* ENABLE_CGAL */

void MainWindow::viewModeThrownTogether()
{
	viewModeActionsUncheck();
	viewActionThrownTogether->setChecked(true);
	screen->setRenderer(this->thrownTogetherRenderer);
}

void MainWindow::viewModeShowEdges()
{
	QSettings settings;
	settings.setValue("view/showEdges",viewActionShowEdges->isChecked());
	screen->setShowEdges(viewActionShowEdges->isChecked());
	screen->updateGL();
}

void MainWindow::viewModeShowAxes()
{
	QSettings settings;
	settings.setValue("view/showAxes",viewActionShowAxes->isChecked());
	screen->setShowAxes(viewActionShowAxes->isChecked());
	screen->updateGL();
}

void MainWindow::viewModeShowCrosshairs()
{
	QSettings settings;
	settings.setValue("view/showCrosshairs",viewActionShowCrosshairs->isChecked());
	screen->setShowCrosshairs(viewActionShowCrosshairs->isChecked());
	screen->updateGL();
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
		double fps = e_fps->text().toDouble(&fps_ok);
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
	screen->object_rot_x = 90;
	screen->object_rot_y = 0;
	screen->object_rot_z = 0;
	screen->updateGL();
}

void MainWindow::viewAngleBottom()
{
	screen->object_rot_x = 270;
	screen->object_rot_y = 0;
	screen->object_rot_z = 0;
	screen->updateGL();
}

void MainWindow::viewAngleLeft()
{
	screen->object_rot_x = 0;
	screen->object_rot_y = 0;
	screen->object_rot_z = 90;
	screen->updateGL();
}

void MainWindow::viewAngleRight()
{
	screen->object_rot_x = 0;
	screen->object_rot_y = 0;
	screen->object_rot_z = 270;
	screen->updateGL();
}

void MainWindow::viewAngleFront()
{
	screen->object_rot_x = 0;
	screen->object_rot_y = 0;
	screen->object_rot_z = 0;
	screen->updateGL();
}

void MainWindow::viewAngleBack()
{
	screen->object_rot_x = 0;
	screen->object_rot_y = 0;
	screen->object_rot_z = 180;
	screen->updateGL();
}

void MainWindow::viewAngleDiagonal()
{
	screen->object_rot_x = 35;
	screen->object_rot_y = 0;
	screen->object_rot_z = 25;
	screen->updateGL();
}

void MainWindow::viewCenter()
{
	screen->object_trans_x = 0;
	screen->object_trans_y = 0;
	screen->object_trans_z = 0;
	screen->updateGL();
}

void MainWindow::viewPerspective()
{
	QSettings settings;
	settings.setValue("view/orthogonalProjection",false);
	viewActionPerspective->setChecked(true);
	viewActionOrthogonal->setChecked(false);
	screen->setOrthoMode(false);
	screen->updateGL();
}

void MainWindow::viewOrthogonal()
{
	QSettings settings;
	settings.setValue("view/orthogonalProjection",true);
	viewActionPerspective->setChecked(false);
	viewActionOrthogonal->setChecked(true);
	screen->setOrthoMode(true);
	screen->updateGL();
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
		openFile(urls[i].path());
	}
	clearCurrentOutput();
}

void
MainWindow::helpAbout()
{
	qApp->setWindowIcon(QApplication::windowIcon());
	QMessageBox::information(this, "About OpenSCAD", QString(helptitle) + QString(copyrighttext));
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
	QFont font(editor->font());
	if (!family.isEmpty()) font.setFamily(family);
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

void MainWindow::setCurrentOutput()
{
	set_output_handler(&MainWindow::consoleOutput, this);
}

void MainWindow::clearCurrentOutput()
{
	set_output_handler(NULL, NULL);
}
