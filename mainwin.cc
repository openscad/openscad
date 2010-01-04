/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"
#include "MainWindow.h"
#include "printutils.h"

#include <QMenu>
#include <QTime>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QStatusBar>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>

//for chdir
#include <unistd.h>

#ifdef ENABLE_CGAL

// a little hackish: we need access to default-private members of
// CGAL::OGL::Nef3_Converter so we can implement our own draw function
// that does not scale the model. so we define 'class' to 'struct'
// for this header..
//
// theoretically there could be two problems:
// 1.) defining language keyword with the pre processor is illegal afair
// 2.) the compiler could use a different memory layout or name mangling for structs
//
// both does not seam to be the case with todays compilers...
//
#define class struct
#include <CGAL/Nef_3/OGL_helper.h>
#undef class

#endif

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

static char helptitle[] =
	"OpenSCAD "
	QUOTED(OPENSCAD_VERSION)
  " (www.openscad.org)\n";
static char copyrighttext[] =
	"Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>\n"
	"\n"
	"This program is free software; you can redistribute it and/or modify"
	"it under the terms of the GNU General Public License as published by"
	"the Free Software Foundation; either version 2 of the License, or"
	"(at your option) any later version.";

QPointer<MainWindow> MainWindow::current_win = NULL;

MainWindow::MainWindow(const char *filename)
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
	root_N = NULL;
	recreate_cgal_ogl_p = false;
	cgal_ogl_p = NULL;
#endif

	highlights_chain = NULL;
	background_chain = NULL;
	root_node = NULL;
	enableOpenCSG = false;

	tval = 0;
	fps = 0;
	fsteps = 1;

	highlighter = NULL;

	QFont font;
	font.setStyleHint(QFont::TypeWriter);
	editor->setFont(font);

	screen->statusLabel = new QLabel(this);
	statusBar()->addWidget(screen->statusLabel);

	animate_timer = new QTimer(this);
	connect(animate_timer, SIGNAL(timeout()), this, SLOT(updateTVal()));

	connect(e_tval, SIGNAL(textChanged(QString)), this, SLOT(actionCompile()));
	connect(e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedFps()));

	animate_panel->hide();

	// File menu
	connect(this->fileActionNew, SIGNAL(triggered()), this, SLOT(actionNew()));
	connect(this->fileActionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(this->fileActionSave, SIGNAL(triggered()), this, SLOT(actionSave()));
	connect(this->fileActionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs()));
	connect(this->fileActionReload, SIGNAL(triggered()), this, SLOT(actionReload()));
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

	// Edit menu
	connect(this->editActionUndo, SIGNAL(triggered()), editor, SLOT(undo()));
	connect(this->editActionRedo, SIGNAL(triggered()), editor, SLOT(redo()));
	connect(this->editActionCut, SIGNAL(triggered()), editor, SLOT(cut()));
	connect(this->editActionCopy, SIGNAL(triggered()), editor, SLOT(copy()));
	connect(this->editActionPaste, SIGNAL(triggered()), editor, SLOT(paste()));
	connect(this->editActionIndent, SIGNAL(triggered()), this, SLOT(editIndent()));
	connect(this->editActionUnindent, SIGNAL(triggered()), this, SLOT(editUnindent()));
	connect(this->editActionComment, SIGNAL(triggered()), this, SLOT(editComment()));
	connect(this->editActionUncomment, SIGNAL(triggered()), this, SLOT(editUncomment()));
	connect(this->editActionPasteVPT, SIGNAL(triggered()), this, SLOT(pasteViewportTranslation()));
	connect(this->editActionPasteVPR, SIGNAL(triggered()), this, SLOT(pasteViewportRotation()));
	connect(this->editActionZoomIn, SIGNAL(triggered()), editor, SLOT(zoomIn()));
	connect(this->editActionZoomOut, SIGNAL(triggered()), editor, SLOT(zoomOut()));
	connect(this->editActionHide, SIGNAL(triggered()), this, SLOT(hideEditor()));

	// Design menu
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

	// View menu
#ifndef ENABLE_OPENCSG
	this->viewActionOpenCSG->setVisible(false);
#else
	connect(this->viewActionOpenCSG, SIGNAL(triggered()), this, SLOT(viewModeOpenCSG()));
	if (!screen->opencsg_support) {
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

// #ifdef ENABLE_CGAL
// 	viewActionCGALSurface = menu->addAction("CGAL Surfaces", this, SLOT(viewModeCGALSurface()), QKeySequence(Qt::Key_F10));
// 	viewActionCGALGrid = menu->addAction("CGAL Grid Only", this, SLOT(viewModeCGALGrid()), QKeySequence(Qt::Key_F11));
// #endif

	// Help menu
	connect(this->helpActionAbout, SIGNAL(triggered()), this, SLOT(helpAbout()));
	connect(this->helpActionManual, SIGNAL(triggered()), this, SLOT(helpManual()));


	console->setReadOnly(true);
	current_win = this;

	PRINT(helptitle);
	PRINT(copyrighttext);
	PRINT("");

	editor->setTabStopWidth(30);

	if (filename) {
		openFile(filename);
	} else {
		setFileName("");
	}

	connect(editor->document(), SIGNAL(contentsChanged()), this, SLOT(animateUpdateDocChanged()));
	connect(editor->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
	connect(editor->document(), SIGNAL(modificationChanged(bool)), fileActionSave, SLOT(setEnabled(bool)));
	connect(screen, SIGNAL(doAnimateUpdate()), this, SLOT(animateUpdate()));

	// display this window and check for OpenGL 2.0 (OpenCSG) support
	viewModeThrownTogether();
	show();

	// make sure it looks nice..
	resize(800, 600);
	splitter1->setSizes(QList<int>() << 400 << 400);
	splitter2->setSizes(QList<int>() << 400 << 200);

#ifdef ENABLE_OPENCSG
	viewModeOpenCSG();
#else
	viewModeThrownTogether();
#endif
	viewPerspective();

	setAcceptDrops(true);
	current_win = NULL;
}

MainWindow::~MainWindow()
{
	if (root_module)
		delete root_module;
	if (root_node)
		delete root_node;
#ifdef ENABLE_CGAL
	if (root_N)
		delete root_N;
	if (cgal_ogl_p) {
		CGAL::OGL::Polyhedron *p = (CGAL::OGL::Polyhedron*)cgal_ogl_p;
		delete p;
	}
#endif
}

/*!
	Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
#ifdef ENABLE_MDI
void MainWindow::requestOpenFile(const QString &filename)
{
	new MainWindow(filename.toUtf8());
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
	if (!editor->toPlainText().isEmpty()) {
		new MainWindow(new_filename.toUtf8());
		current_win = NULL;
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
		setWindowTitle("OpenSCAD - New Document[*]");
	}
	else {
		QFileInfo fileinfo(filename);
		QString infoFileName = fileinfo.canonicalFilePath();
		setWindowTitle("OpenSCAD - " + fileinfo.fileName() + "[*]");

		// Check that the canonical file path exists - only update recent files
		// if it does. Should prevent empty list items on initial open etc.
		if (!infoFileName.isEmpty()) {
			QSettings settings; // already set up properly via main.cpp
			QStringList files = settings.value("recentFileList").toStringList();
			files.removeAll(this->fileName);
			files.prepend(this->fileName);
			while (files.size() > maxRecentFiles)
				files.removeLast();
			settings.setValue("recentFileList", files);
			this->fileName = infoFileName;
		} else {
			this->fileName = fileinfo.fileName();
		}
		
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
	current_win = this;
	if (!this->fileName.isEmpty()) {
		QString text;
		FILE *fp = fopen(this->fileName.toUtf8(), "rt");
		if (!fp) {
			PRINTA("Failed to open file: %1 (%2)", this->fileName, QString(strerror(errno)));
		} else {
			char buffer[513];
			int rc;
			while ((rc = fread(buffer, 1, 512, fp)) > 0) {
				buffer[rc] = 0;
				text += buffer;
			}
			fclose(fp);
			PRINTA("Loaded design `%1'.", this->fileName);
		}
		editor->setPlainText(text);
	}
	current_win = this;
}

void MainWindow::find_root_tag(AbstractNode *n)
{
	foreach(AbstractNode *v, n->children) {
		if (v->modinst->tag_root)
			root_node = v;
		if (root_node)
			return;
		find_root_tag(v);
	}
}

void MainWindow::compile(bool procevents)
{
	PRINT("Parsing design (AST generation)...");
	if (procevents)
		QApplication::processEvents();

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
	enableOpenCSG = false;

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

	last_compiled_doc = editor->toPlainText();
	root_module = parse((last_compiled_doc + "\n" + commandline_commands).toAscii().data(), false);

	if (highlighter) {
		delete highlighter;
		highlighter = NULL;
	}
	if (parser_error_pos >= 0) {
		highlighter = new Highlighter(editor->document());
	}

	if (!root_module) {
		if (!animate_panel->isVisible()) {
			QTextCursor cursor = editor->textCursor();
			cursor.setPosition(parser_error_pos);
			editor->setTextCursor(cursor);
		}
		goto fail;
	}

	PRINT("Compiling design (CSG Tree generation)...");
	if (procevents)
		QApplication::processEvents();

	AbstractNode::idx_counter = 1;
	{
		ModuleInstanciation root_inst;
		absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);
	}

	if (!absolute_root_node)
		goto fail;

	find_root_tag(absolute_root_node);
	if (!root_node)
		root_node = absolute_root_node;
	root_node->dump("");

	PRINT("Compiling design (CSG Products generation)...");
	if (procevents)
		QApplication::processEvents();

	double m[16];

	for (int i = 0; i < 16; i++)
		m[i] = i % 5 == 0 ? 1.0 : 0.0;

	root_raw_term = root_node->render_csg_term(m, &highlight_terms, &background_terms);

	if (!root_raw_term)
		goto fail;

	PRINT("Compiling design (CSG Products normalization)...");
	if (procevents)
		QApplication::processEvents();

	root_norm_term = root_raw_term->link();

	while (1) {
		CSGTerm *n = root_norm_term->normalize();
		root_norm_term->unlink();
		if (root_norm_term == n)
			break;
		root_norm_term = n;
	}

	if (!root_norm_term)
		goto fail;

	root_chain = new CSGChain();
	root_chain->import(root_norm_term);

	if (root_chain->polysets.size() > 1000) {
		PRINTF("WARNING: Normalized tree has %d elements!", root_chain->polysets.size());
		PRINTF("WARNING: OpenCSG rendering has been disabled.");
	} else {
		enableOpenCSG = true;
	}

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

	if (1) {
		PRINT("Compilation finished.");
		if (procevents)
			QApplication::processEvents();
	} else {
fail:
		PRINT("ERROR: Compilation failed!");
		if (procevents)
			QApplication::processEvents();
	}
}

void MainWindow::actionNew()
{
#ifdef ENABLE_MDI
	new MainWindow;
#else
	setFileName("");
	editor->setPlainText("");
#endif
}

void MainWindow::actionOpen()
{
	current_win = this;
	QString new_filename = QFileDialog::getOpenFileName(this, "Open File", "", 
																											"OpenSCAD Designs (*.scad)");
	if (!new_filename.isEmpty()) openFile(new_filename);
	current_win = NULL;
}

void MainWindow::actionOpenRecent()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		openFile(action->data().toString());
	}
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

void MainWindow::actionSave()
{
	if (this->fileName.isEmpty()) {
		actionSaveAs();
	}
	else {
		current_win = this;
		FILE *fp = fopen(this->fileName.toUtf8(), "wt");
		if (!fp) {
			PRINTA("Failed to open file for writing: %1 (%2)", this->fileName, QString(strerror(errno)));
		} else {
			fprintf(fp, "%s", editor->toPlainText().toAscii().data());
			fclose(fp);
			PRINTA("Saved design `%1'.", this->fileName);
			this->editor->document()->setModified(false);
		}
		current_win = NULL;
	}
}

void MainWindow::actionSaveAs()
{
	QString new_filename = QFileDialog::getSaveFileName(this, "Save File", this->fileName, "OpenSCAD Designs (*.scad)");
	if (!new_filename.isEmpty()) {
		setFileName(new_filename);
		actionSave();
	}
}

void MainWindow::actionReload()
{
	load();
}

void MainWindow::editIndent()
{
	QTextCursor cursor = editor->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("\t"));
	if (txt.endsWith(QString(QChar(8233)) + QString("\t")))
		txt.chop(1);
	txt = QString("\t") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	editor->setTextCursor(cursor);
}

void MainWindow::editUnindent()
{
	QTextCursor cursor = editor->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("\t"), QString(QChar(8233)));
	if (txt.startsWith(QString("\t")))
		txt.remove(0, 1);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	editor->setTextCursor(cursor);
}

void MainWindow::editComment()
{
	QTextCursor cursor = editor->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("//"));
	if (txt.endsWith(QString(QChar(8233)) + QString("//")))
		txt.chop(2);
	txt = QString("//") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	editor->setTextCursor(cursor);
}

void MainWindow::editUncomment()
{
	QTextCursor cursor = editor->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("//"), QString(QChar(8233)));
	if (txt.startsWith(QString("//")))
		txt.remove(0, 2);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	editor->setTextCursor(cursor);
}

void MainWindow::hideEditor()
{
	if (editActionHide->isChecked()) {
		editor->hide();
	} else {
		editor->show();
	}
}

void MainWindow::pasteViewportTranslation()
{
	QTextCursor cursor = editor->textCursor();
	QString txt;
	txt.sprintf("[ %.2f, %.2f, %.2f ]", -screen->object_trans_x, -screen->object_trans_y, -screen->object_trans_z);
	cursor.insertText(txt);
}

void MainWindow::pasteViewportRotation()
{
	QTextCursor cursor = editor->textCursor();
	QString txt;
	txt.sprintf("[ %.2f, %.2f, %.2f ]",
		fmodf(360 - screen->object_rot_x + 90, 360), fmodf(360 - screen->object_rot_y, 360), fmodf(360 - screen->object_rot_z, 360));
	cursor.insertText(txt);
}

void MainWindow::actionReloadCompile()
{
	console->clear();

	load();

	current_win = this;
	compile(true);

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
	current_win = NULL;
}

void MainWindow::actionCompile()
{
	current_win = this;
	console->clear();

	compile(!viewActionAnimate->isChecked());

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
	current_win = NULL;
}

#ifdef ENABLE_CGAL

static void report_func(const class AbstractNode*, void *vp, int mark)
{
	QProgressDialog *pd = (QProgressDialog*)vp;
	int v = (int)((mark*100.0) / progress_report_count);
	pd->setValue(v < 100 ? v : 99);
	QString label;
	label.sprintf("Rendering Polygon Mesh using CGAL (%d/%d)", mark, progress_report_count);
	pd->setLabelText(label);
	QApplication::processEvents();
}

void MainWindow::actionRenderCGAL()
{
	current_win = this;
	console->clear();

	compile(true);

	if (!root_module || !root_node)
		return;

	if (root_N) {
		delete root_N;
		root_N = NULL;
		recreate_cgal_ogl_p = true;
	}

	PRINT("Rendering Polygon Mesh using CGAL...");
	QApplication::processEvents();

	QTime t;
	t.start();

	QProgressDialog *pd = new QProgressDialog("Rendering Polygon Mesh using CGAL...", QString(), 0, 100);
	pd->setValue(0);
	pd->setAutoClose(false);
	pd->show();
	QApplication::processEvents();

	progress_report_prep(root_node, report_func, pd);
	root_N = new CGAL_Nef_polyhedron(root_node->render_cgal_nef_polyhedron());
	progress_report_fin();

	PRINTF("Number of vertices currently in CGAL cache: %d", AbstractNode::cgal_nef_cache.totalCost());
	PRINTF("Number of objects currently in CGAL cache: %d", AbstractNode::cgal_nef_cache.size());
	QApplication::processEvents();

	if (root_N->dim == 2) {
		PRINTF("   Top level object is a 2D object:");
		QApplication::processEvents();
		PRINTF("   Empty:      %6s", root_N->p2.is_empty() ? "yes" : "no");
		QApplication::processEvents();
		PRINTF("   Plane:      %6s", root_N->p2.is_plane() ? "yes" : "no");
		QApplication::processEvents();
		PRINTF("   Vertices:   %6d", (int)root_N->p2.explorer().number_of_vertices());
		QApplication::processEvents();
		PRINTF("   Halfedges:  %6d", (int)root_N->p2.explorer().number_of_halfedges());
		QApplication::processEvents();
		PRINTF("   Edges:      %6d", (int)root_N->p2.explorer().number_of_edges());
		QApplication::processEvents();
		PRINTF("   Faces:      %6d", (int)root_N->p2.explorer().number_of_faces());
		QApplication::processEvents();
		PRINTF("   FaceCycles: %6d", (int)root_N->p2.explorer().number_of_face_cycles());
		QApplication::processEvents();
		PRINTF("   ConnComp:   %6d", (int)root_N->p2.explorer().number_of_connected_components());
		QApplication::processEvents();
	}

	if (root_N->dim == 3) {
		PRINTF("   Top level object is a 3D object:");
		PRINTF("   Simple:     %6s", root_N->p3.is_simple() ? "yes" : "no");
		QApplication::processEvents();
		PRINTF("   Valid:      %6s", root_N->p3.is_valid() ? "yes" : "no");
		QApplication::processEvents();
		PRINTF("   Vertices:   %6d", (int)root_N->p3.number_of_vertices());
		QApplication::processEvents();
		PRINTF("   Halfedges:  %6d", (int)root_N->p3.number_of_halfedges());
		QApplication::processEvents();
		PRINTF("   Edges:      %6d", (int)root_N->p3.number_of_edges());
		QApplication::processEvents();
		PRINTF("   Halffacets: %6d", (int)root_N->p3.number_of_halffacets());
		QApplication::processEvents();
		PRINTF("   Facets:     %6d", (int)root_N->p3.number_of_facets());
		QApplication::processEvents();
		PRINTF("   Volumes:    %6d", (int)root_N->p3.number_of_volumes());
		QApplication::processEvents();
	}

	int s = t.elapsed() / 1000;
	PRINTF("Total rendering time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);

	if (!viewActionCGALSurfaces->isChecked() && !viewActionCGALGrid->isChecked()) {
		viewModeCGALSurface();
	} else {
		screen->updateGL();
	}

	PRINT("Rendering finished.");

	delete pd;
	current_win = NULL;

}

#endif /* ENABLE_CGAL */

void MainWindow::actionDisplayAST()
{
	current_win = this;
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("AST Dump");
	if (root_module) {
		e->setPlainText(root_module->dump("", ""));
	} else {
		e->setPlainText("No AST to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
	current_win = NULL;
}

void MainWindow::actionDisplayCSGTree()
{
	current_win = this;
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Tree Dump");
	if (root_node) {
		e->setPlainText(root_node->dump(""));
	} else {
		e->setPlainText("No CSG to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
	current_win = NULL;
}

void MainWindow::actionDisplayCSGProducts()
{
	current_win = this;
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Products Dump");
	e->setPlainText(QString("\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering chain:\n%3\n\n\nHighlights CSG rendering chain:\n%4\n\n\nBackground CSG rendering chain:\n%5\n").arg(root_raw_term ? root_raw_term->dump() : "N/A", root_norm_term ? root_norm_term->dump() : "N/A", root_chain ? root_chain->dump() : "N/A", highlights_chain ? highlights_chain->dump() : "N/A", background_chain ? background_chain->dump() : "N/A"));
	e->show();
	e->resize(600, 400);
	current_win = NULL;
}

#ifdef ENABLE_CGAL
void MainWindow::actionExportSTLorOFF(bool stl_mode)
#else
void MainWindow::actionExportSTLorOFF(bool)
#endif
{
	current_win = this;

#ifdef ENABLE_CGAL
	if (!root_N) {
		PRINT("Nothing to export! Try building first (press F6).");
		current_win = NULL;
		return;
	}

	if (root_N->dim != 3) {
		PRINT("Current top level object is not a 3D object.");
		current_win = NULL;
		return;
	}

	if (!root_N->p3.is_simple()) {
		PRINT("Object isn't a valid 2-manifold! Modify your design..");
		current_win = NULL;
		return;
	}

	QString stl_filename = QFileDialog::getSaveFileName(this,
			stl_mode ? "Export STL File" : "Export OFF File", "",
			stl_mode ? "STL Files (*.stl)" : "OFF Files (*.off)");
	if (stl_filename.isEmpty()) {
		PRINTF("No filename specified. %s export aborted.", stl_mode ? "STL" : "OFF");
		current_win = NULL;
		return;
	}

	QProgressDialog *pd = new QProgressDialog(
			stl_mode ? "Exporting object to STL file..." : "Exporting object to OFF file...",
			QString(), 0, root_N->p3.number_of_facets() + 1);
	pd->setValue(0);
	pd->setAutoClose(false);
	pd->show();
	QApplication::processEvents();

	if (stl_mode)
		export_stl(root_N, stl_filename, pd);
	else
		export_off(root_N, stl_filename, pd);

	PRINTF("%s export finished.", stl_mode ? "STL" : "OFF");

	delete pd;
#endif /* ENABLE_CGAL */
	current_win = NULL;
}

void MainWindow::actionExportSTL()
{
	actionExportSTLorOFF(true);
}

void MainWindow::actionExportOFF()
{
	actionExportSTLorOFF(false);
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

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	PolySet *p;
	double *m;
	int csgmode;
	virtual void render() {
		glPushMatrix();
		glMultMatrixd(m);
		p->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode));
		glPopMatrix();
	}
};

static void renderCSGChainviaOpenCSG(CSGChain *chain, GLint *shaderinfo, bool highlight, bool background)
{
	std::vector<OpenCSG::Primitive*> primitives;
	int j = 0;
	for (int i = 0;; i++)
	{
		bool last = i == chain->polysets.size();

		if (last || chain->types[i] == CSGTerm::TYPE_UNION)
		{
			OpenCSG::render(primitives);
			glDepthFunc(GL_EQUAL);
			if (shaderinfo)
				glUseProgram(shaderinfo[0]);
			for (; j < i; j++) {
				glPushMatrix();
				glMultMatrixd(chain->matrices[j]);
				int csgmode = chain->types[j] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
				if (highlight) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20), shaderinfo);
				} else if (background) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10), shaderinfo);
				} else if (chain->types[j] == CSGTerm::TYPE_DIFFERENCE) {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode), shaderinfo);
				} else {
					chain->polysets[j]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode), shaderinfo);
				}
				glPopMatrix();
			}
			if (shaderinfo)
				glUseProgram(0);
			for (unsigned int k = 0; k < primitives.size(); k++) {
				delete primitives[k];
			}
			glDepthFunc(GL_LEQUAL);
			primitives.clear();
		}

		if (last)
			break;

		OpenCSGPrim *prim = new OpenCSGPrim(chain->types[i] == CSGTerm::TYPE_DIFFERENCE ?
				OpenCSG::Subtraction : OpenCSG::Intersection, chain->polysets[i]->convexity);
		prim->p = chain->polysets[i];
		prim->m = chain->matrices[i];
		prim->csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight)
			prim->csgmode += 20;
		else if (background)
			prim->csgmode += 10;
		primitives.push_back(prim);
	}
}

static void renderGLThrownTogether(void *vp);

static void renderGLviaOpenCSG(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	if (!m->enableOpenCSG) {
		renderGLThrownTogether(vp);
		return;
	}
	static int glew_initialized = 0;
	if (!glew_initialized) {
		glew_initialized = 1;
		glewInit();
	}
#ifdef ENABLE_MDI
	OpenCSG::reset();
#endif
	if (m->root_chain) {
		GLint *shaderinfo = m->screen->shaderinfo;
		if (!shaderinfo[0])
			shaderinfo = NULL;
		renderCSGChainviaOpenCSG(m->root_chain, m->viewActionShowEdges->isChecked() ? shaderinfo : NULL, false, false);
		if (m->background_chain) {
			renderCSGChainviaOpenCSG(m->background_chain, m->viewActionShowEdges->isChecked() ? shaderinfo : NULL, false, true);
		}
		if (m->highlights_chain) {
			renderCSGChainviaOpenCSG(m->highlights_chain, m->viewActionShowEdges->isChecked() ? shaderinfo : NULL, true, false);
		}
	}
}

void MainWindow::viewModeOpenCSG()
{
	if (screen->opencsg_support) {
		viewModeActionsUncheck();
		viewActionOpenCSG->setChecked(true);
		screen->renderfunc = renderGLviaOpenCSG;
		screen->renderfunc_vp = this;
		screen->updateGL();
	} else {
		viewModeThrownTogether();
	}
}

#endif /* ENABLE_OPENCSG */

#ifdef ENABLE_CGAL

static void renderGLviaCGAL(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	if (m->recreate_cgal_ogl_p) {
		m->recreate_cgal_ogl_p = false;
		CGAL::OGL::Polyhedron *p = (CGAL::OGL::Polyhedron*)m->cgal_ogl_p;
		delete p;
		m->cgal_ogl_p = NULL;
	}
	if (m->root_N && m->root_N->dim == 2)
	{
		typedef CGAL_Nef_polyhedron2::Explorer Explorer;
		typedef Explorer::Face_const_iterator fci_t;
		typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
		typedef Explorer::Point Point;
		Explorer E = m->root_N->p2.explorer();

		for (fci_t fit = E.faces_begin(), fend = E.faces_end(); fit != fend; ++fit)
		{
			bool fset = false;
			double fx = 0.0, fy = 0.0;
			heafcc_t fcirc(E.halfedge(fit)), fend(fcirc);
			CGAL_For_all(fcirc, fend) {
				if(E.is_standard(E.target(fcirc))) {
					Point p = E.point(E.target(fcirc));
					double x = to_double(p.x()), y = to_double(p.y());
					if (!fset) {
						glBegin(GL_LINE_STRIP);
						fx = x, fy = y;
						fset = true;
					}
					glVertex3d(x, y, 0.0);
				}
			}
			if (fset) {
				glVertex3d(fx, fy, 0.0);
				glEnd();
			}
		}
	}
	if (m->root_N && m->root_N->dim == 3)
	{
		CGAL::OGL::Polyhedron *p = (CGAL::OGL::Polyhedron*)m->cgal_ogl_p;
		if (!p) {
			m->cgal_ogl_p = p = new CGAL::OGL::Polyhedron();
			CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(m->root_N->p3, p);
			p->init();
		}
		if (m->viewActionCGALSurfaces->isChecked())
			p->set_style(CGAL::OGL::SNC_BOUNDARY);
		if (m->viewActionCGALGrid->isChecked())
			p->set_style(CGAL::OGL::SNC_SKELETON);
#if 0
		p->draw();
#else
		if (p->style == CGAL::OGL::SNC_BOUNDARY) {
			glCallList(p->object_list_+2);
			if (m->viewActionShowEdges->isChecked()) {
				glDisable(GL_LIGHTING);
				glCallList(p->object_list_+1);
				glCallList(p->object_list_);
			}
		} else {
			glDisable(GL_LIGHTING);
			glCallList(p->object_list_+1);
			glCallList(p->object_list_);
		}
#endif
	}
}

void MainWindow::viewModeCGALSurface()
{
	viewModeActionsUncheck();
	viewActionCGALSurfaces->setChecked(true);
	screen->renderfunc = renderGLviaCGAL;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

void MainWindow::viewModeCGALGrid()
{
	viewModeActionsUncheck();
	viewActionCGALGrid->setChecked(true);
	screen->renderfunc = renderGLviaCGAL;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

#endif /* ENABLE_CGAL */

static void renderGLThrownTogetherChain(MainWindow *m, CSGChain *chain, bool highlight, bool background, bool fberror)
{
	glDepthFunc(GL_LEQUAL);
	QHash<QPair<PolySet*,double*>,int> polySetVisitMark;
	bool showEdges = m->viewActionShowEdges->isChecked();
	for (int i = 0; i < chain->polysets.size(); i++) {
		if (polySetVisitMark[QPair<PolySet*,double*>(chain->polysets[i], chain->matrices[i])]++ > 0)
			continue;
		glPushMatrix();
		glMultMatrixd(chain->matrices[i]);
		int csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20));
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20));
				glEnable(GL_LIGHTING);
			}
		} else if (background) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10));
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10));
				glEnable(GL_LIGHTING);
			}
		} else if (fberror) {
			if (highlight) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 20));
			} else if (background) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 10));
			} else {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode));
			}
		} else if (chain->types[i] == CSGTerm::TYPE_DIFFERENCE) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode));
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		} else {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode));
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		}
		glPopMatrix();
	}
}

static void renderGLThrownTogether(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	if (m->root_chain) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		renderGLThrownTogetherChain(m, m->root_chain, false, false, false);
		glCullFace(GL_FRONT);
		glColor3ub(255, 0, 255);
		renderGLThrownTogetherChain(m, m->root_chain, false, false, true);
		glDisable(GL_CULL_FACE);
	}
	if (m->background_chain)
		renderGLThrownTogetherChain(m, m->background_chain, false, true, false);
	if (m->highlights_chain)
		renderGLThrownTogetherChain(m, m->highlights_chain, true, false, false);
}

void MainWindow::viewModeThrownTogether()
{
	viewModeActionsUncheck();
	viewActionThrownTogether->setChecked(true);
	screen->renderfunc = renderGLThrownTogether;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

void MainWindow::viewModeShowEdges()
{
	screen->updateGL();
}

void MainWindow::viewModeShowAxes()
{
	screen->showaxes = viewActionShowAxes->isChecked();
	screen->updateGL();
}

void MainWindow::viewModeShowCrosshairs()
{
	screen->showcrosshairs = viewActionShowCrosshairs->isChecked();
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
	viewActionPerspective->setChecked(true);
	viewActionOrthogonal->setChecked(false);
	screen->orthomode = false;
	screen->updateGL();
}

void MainWindow::viewOrthogonal()
{
	viewActionPerspective->setChecked(false);
	viewActionOrthogonal->setChecked(true);
	screen->orthomode = true;
	screen->updateGL();
}

void MainWindow::hideConsole()
{
	if (viewActionHide->isChecked()) {
		console->hide();
	} else {
		console->show();
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	current_win = this;
	const QList<QUrl> urls = event->mimeData()->urls();
	for (int i = 0; i < urls.size(); i++) {
		if (urls[i].scheme() != "file")
			continue;
		openFile(urls[i].path());
	}
	current_win = NULL;
}

void
MainWindow::helpAbout()
{
	qApp->setWindowIcon(QApplication::windowIcon());
  QMessageBox::information(this, "About OpenSCAD", 
													 QString(helptitle) + QString(copyrighttext));
}

void
MainWindow::helpManual()
{
	QDesktopServices::openUrl(QUrl("http://en.wikibooks.org/wiki/OpenSCAD_User_Manual"));
}

/*!
	FIXME: In SDI mode, this should be called also on New and Open
	In MDI mode; also call on both reload functions?
 */
bool
MainWindow::maybeSave()
{
	if (editor->document()->isModified()) {
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
		event->accept();
	} else {
		event->ignore();
	}
}
