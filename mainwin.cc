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

#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QApplication>

MainWindow::MainWindow(const char *filename)
{
        root_ctx.functions_p = &builtin_functions;
        root_ctx.modules_p = &builtin_modules;
	root_ctx.set_variable("$fs_render", Value(1.0));
	root_ctx.set_variable("$fa_render", Value(12.0));
	root_ctx.set_variable("$fs_preview", Value(1.0));
	root_ctx.set_variable("$fa_preview", Value(12.0));

	root_module = NULL;
	root_node = NULL;
	root_raw_term = NULL;
	root_norm_term = NULL;
	root_chain = NULL;
#ifdef ENABLE_CGAL
	root_N = NULL;
#endif

	if (filename) {
		this->filename = QString(filename);
		setWindowTitle(this->filename);
	} else {
		setWindowTitle("New Document");
	}

	{
		QMenu *menu = menuBar()->addMenu("&File");
		menu->addAction("&New", this, SLOT(actionNew()));
		menu->addAction("&Open...", this, SLOT(actionOpen()));
		menu->addAction("&Save", this, SLOT(actionSave()));
		menu->addAction("Save &As...", this, SLOT(actionSaveAs()));
		menu->addAction("&Quit", this, SLOT(close()));
	}

	{
		QMenu *menu = menuBar()->addMenu("&Design");
		menu->addAction("&Compile", this, SLOT(actionCompile()), QKeySequence(Qt::Key_F5));
#ifdef ENABLE_CGAL
		menu->addAction("Compile and &Render (CGAL)", this, SLOT(actionRenderCGAL()), QKeySequence(Qt::Key_F6));
#endif
		menu->addAction("Display &AST...", this, SLOT(actionDisplayAST()));
		menu->addAction("Display CSG &Tree...", this, SLOT(actionDisplayCSGTree()));
		menu->addAction("Display CSG &Products...", this, SLOT(actionDisplayCSGProducts()));
		menu->addAction("Export as &STL...", this, SLOT(actionExportSTL()));
		menu->addAction("Export as &OFF...", this, SLOT(actionExportOFF()));
	}

	{
		QMenu *menu = menuBar()->addMenu("&View");
#ifdef ENABLE_OPENCSG
		actViewModeOpenCSG = menu->addAction("OpenCSG", this, SLOT(viewModeOpenCSG()));
		actViewModeOpenCSG->setCheckable(true);
#endif
#ifdef ENABLE_CGAL
		actViewModeCGALSurface = menu->addAction("CGAL Surfaces", this, SLOT(viewModeCGALSurface()));
		actViewModeCGALGrid = menu->addAction("CGAL Grid Only", this, SLOT(viewModeCGALGrid()));
		actViewModeCGALSurface->setCheckable(true);
		actViewModeCGALGrid->setCheckable(true);
#endif
		actViewModeThrownTogether = menu->addAction("Thrown Together", this, SLOT(viewModeThrownTogether()));
		actViewModeThrownTogether->setCheckable(true);

		menu->addSeparator();
		menu->addAction("Top");
		menu->addAction("Bottom");
		menu->addAction("Left");
		menu->addAction("Right");
		menu->addAction("Front");
		menu->addAction("Back");
		menu->addAction("Diagonal");
		menu->addSeparator();
		menu->addAction("Perspective");
		menu->addAction("Orthogonal");
	}

	s1 = new QSplitter(Qt::Horizontal, this);
	editor = new QTextEdit(s1);
	s2 = new QSplitter(Qt::Vertical, s1);
	screen = new GLView(s2);
	console = new QTextEdit(s2);

	console->setReadOnly(true);
	console->append("OpenSCAD (www.openscad.at)");
	console->append("Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>");
	console->append("");
	console->append("This program is free software; you can redistribute it and/or modify");
	console->append("it under the terms of the GNU General Public License as published by");
	console->append("the Free Software Foundation; either version 2 of the License, or");
	console->append("(at your option) any later version.");
	console->append("");

	editor->setTabStopWidth(30);

	if (filename) {
		QString text;
		FILE *fp = fopen(filename, "rt");
		if (!fp) {
			console->append(QString("Failed to open file: %1 (%2)").arg(QString(filename), QString(strerror(errno))));
		} else {
			char buffer[513];
			int rc;
			while ((rc = fread(buffer, 1, 512, fp)) > 0) {
				buffer[rc] = 0;
				text += buffer;
			}
			fclose(fp);
			console->append(QString("Loaded design `%1'.").arg(QString(filename)));
		}
		editor->setPlainText(text);
	}

#ifdef ENABLE_OPENCSG
	viewModeOpenCSG();
#else
	viewModeThrownTogether();
#endif

	setCentralWidget(s1);
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
#endif
}

void MainWindow::actionNew()
{
	filename = QString();
	setWindowTitle("New Document");
	editor->setPlainText("");
}

void MainWindow::actionOpen()
{
	QString new_filename = QFileDialog::getOpenFileName(this, "Open File", "", "OpenSCAD Designs (*.scad)");
	if (!new_filename.isEmpty())
	{
		filename = new_filename;
		setWindowTitle(filename);

		QString text;
		FILE *fp = fopen(filename.toAscii().data(), "rt");
		if (!fp) {
			console->append(QString("Failed to open file: %1 (%2)").arg(QString(filename), QString(strerror(errno))));
		} else {
			char buffer[513];
			int rc;
			while ((rc = fread(buffer, 1, 512, fp)) > 0) {
				buffer[rc] = 0;
				text += buffer;
			}
			fclose(fp);
			console->append(QString("Loaded design `%1'.").arg(QString(filename)));
		}
		editor->setPlainText(text);
	}
}

void MainWindow::actionSave()
{
	FILE *fp = fopen(filename.toAscii().data(), "wt");
	if (!fp) {
		console->append(QString("Failed to open file for writing: %1 (%2)").arg(QString(filename), QString(strerror(errno))));
	} else {
		fprintf(fp, "%s", editor->toPlainText().toAscii().data());
		fclose(fp);
		console->append(QString("Saved design `%1'.").arg(QString(filename)));
	}
}

void MainWindow::actionSaveAs()
{
	QString new_filename = QFileDialog::getSaveFileName(this, "Save File", filename, "OpenSCAD Designs (*.scad)");
	if (!new_filename.isEmpty()) {
		filename = new_filename;
		setWindowTitle(filename);
		actionSave();
	}
}

void MainWindow::actionCompile()
{
	console->append("Parsing design (AST generation)...");
	QApplication::processEvents();

	if (root_module) {
		delete root_module;
		root_module = NULL;
	}

	root_module = parse(editor->toPlainText().toAscii().data(), false);

	if (!root_module) {
		console->append("Compilation failed!");
		return;
	}

	console->append("Compiling design (CSG Tree generation)...");
	QApplication::processEvents();

	if (root_node) {
		delete root_node;
		root_node = NULL;
	}

	AbstractNode::idx_counter = 1;
	root_node = root_module->evaluate(&root_ctx, QVector<QString>(), QVector<Value>(), QVector<AbstractNode*>());

	if (!root_node) {
		console->append("Compilation failed!");
		return;
	}

	console->append("Compiling design (CSG Products generation)...");
	QApplication::processEvents();

	if (root_raw_term) {
		root_raw_term->unlink();
		root_raw_term = NULL;
	}

	double m[16];
	root_raw_term = root_node->render_csg_term(m);

	if (!root_raw_term) {
		console->append("Compilation failed!");
		return;
	}

	console->append("Compiling design (CSG Products normalization)...");
	QApplication::processEvents();

	if (root_norm_term) {
		root_norm_term->unlink();
		root_norm_term = NULL;
	}

	if (root_chain) {
		delete root_chain;
		root_chain = NULL;
	}

	root_norm_term = root_raw_term->link();

	while (1) {
		CSGTerm *n = root_norm_term->normalize();
		root_norm_term->unlink();
		if (root_norm_term == n)
			break;
		root_norm_term = n;
	}

	if (!root_norm_term) {
		console->append("Compilation failed!");
		return;
	}

	root_chain = new CSGChain();
	root_chain->import(root_norm_term);

	screen->updateGL();
	console->append("Compilation finished.");
}

#ifdef ENABLE_CGAL

static void report_func(const class AbstractNode*, void *vp, int mark)
{
	MainWindow *m = (MainWindow*)vp;
	QString msg;
	msg.sprintf("CSG rendering progress: %.2f%%", (mark*100.0) / progress_report_count);
	QApplication::processEvents();
        m->console->append(msg);
}

void MainWindow::actionRenderCGAL()
{
	actionCompile();

	if (!root_module || !root_node)
		return;

	if (root_N) {
		delete root_N;
		root_N = NULL;
	}

	progress_report_prep(root_node, report_func, this);
	root_N = new CGAL_Nef_polyhedron(root_node->render_cgal_nef_polyhedron());
	progress_report_fin();

	if (!actViewModeCGALSurface->isChecked() && !actViewModeCGALGrid->isChecked()) {
		viewModeCGALSurface();
	} else {
		screen->updateGL();
	}
}

#endif /* ENABLE_CGAL */

void MainWindow::actionDisplayAST()
{
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
}

void MainWindow::actionDisplayCSGTree()
{
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
}

void MainWindow::actionDisplayCSGProducts()
{
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Products Dump");
	e->setPlainText(QString("\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering chain:\n%3\n").arg(root_raw_term ? root_raw_term->dump() : "N/A", root_norm_term ? root_norm_term->dump() : "N/A", root_chain ? root_chain->dump() : "N/A"));
	e->show();
	e->resize(600, 400);
}

void MainWindow::actionExportSTL()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionExportOFF()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::viewModeActionsUncheck()
{
#ifdef ENABLE_OPENCSG
	actViewModeOpenCSG->setChecked(false);
#endif
#ifdef ENABLE_CGAL
	actViewModeCGALSurface->setChecked(false);
	actViewModeCGALGrid->setChecked(false);
#endif
	actViewModeThrownTogether->setChecked(false);
}

#ifdef ENABLE_OPENCSG

class OpenCSGPrim : public OpenCSG::Primitive
{
public:
	OpenCSGPrim(OpenCSG::Operation operation, unsigned int convexity) :
			OpenCSG::Primitive(operation, convexity) { }
	PolySet *p;
	virtual void render() {
		p->render_surface(PolySet::COLOR_NONE);
	}
};

static void renderGLviaOpenCSG(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	static int glew_initialized = 0;
	if (!glew_initialized) {
		glew_initialized = 1;
		glewInit();
	}

	if (m->root_chain) {
		std::vector<OpenCSG::Primitive*> primitives;
		int j = 0;
		for (int i = 0;; i++)
		{
			bool last = i == m->root_chain->polysets.size();

			if (last || m->root_chain->types[i] == CSGTerm::UNION)
			{
				OpenCSG::render(primitives, OpenCSG::Goldfeather, OpenCSG::NoDepthComplexitySampling);
				glDepthFunc(GL_EQUAL);
				glUseProgram(m->screen->shaderinfo[0]);
				for (; j < i; j++) {
					if (m->root_chain->types[j] == CSGTerm::DIFFERENCE) {
						m->root_chain->polysets[j]->render_surface(PolySet::COLOR_CUTOUT, m->screen->shaderinfo);
					} else {
						m->root_chain->polysets[j]->render_surface(PolySet::COLOR_MATERIAL, m->screen->shaderinfo);
					}
				}
				glUseProgram(0);
				for (unsigned int k = 0; k < primitives.size(); k++) {
					delete primitives[k];
				}
				glDepthFunc(GL_LESS);

				primitives.clear();
			}

			if (last)
				break;

			OpenCSGPrim *prim = new OpenCSGPrim(m->root_chain->types[i] == CSGTerm::DIFFERENCE ?
					OpenCSG::Subtraction : OpenCSG::Intersection, 1);
			prim->p = m->root_chain->polysets[i];
			primitives.push_back(prim);
		}
	}
}

void MainWindow::viewModeOpenCSG()
{
	viewModeActionsUncheck();
	actViewModeOpenCSG->setChecked(true);
	screen->renderfunc = renderGLviaOpenCSG;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

#endif /* ENABLE_OPENCSG */

#ifdef ENABLE_CGAL

#include <CGAL/Nef_3/OGL_helper.h>

static void renderGLviaCGAL(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	if (m->root_N) {
		CGAL::OGL::Polyhedron P;
		CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron>::convert_to_OGLPolyhedron(*m->root_N, &P);
		P.init();
		if (m->actViewModeCGALSurface->isChecked())
			P.set_style(CGAL::OGL::SNC_BOUNDARY);
		if (m->actViewModeCGALGrid->isChecked())
			P.set_style(CGAL::OGL::SNC_SKELETON);
		P.draw();
	}
}

void MainWindow::viewModeCGALSurface()
{
	viewModeActionsUncheck();
	actViewModeCGALSurface->setChecked(true);
	screen->renderfunc = renderGLviaCGAL;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

void MainWindow::viewModeCGALGrid()
{
	viewModeActionsUncheck();
	actViewModeCGALGrid->setChecked(true);
	screen->renderfunc = renderGLviaCGAL;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

#endif /* ENABLE_CGAL */

static void renderGLThrownTogether(void *vp)
{
	MainWindow *m = (MainWindow*)vp;
	if (m->root_chain) {
		glDepthFunc(GL_LEQUAL);
#if 0
		glUseProgram(m->screen->shaderinfo[0]);
		for (int i = 0; i < m->root_chain->polysets.size(); i++) {
			if (m->root_chain->types[i] == CSGTerm::DIFFERENCE) {
				m->root_chain->polysets[i]->render_surface(PolySet::COLOR_CUTOUT, m->screen->shaderinfo);
			} else {
				m->root_chain->polysets[i]->render_surface(PolySet::COLOR_MATERIAL, m->screen->shaderinfo);
			}
		}
		glUseProgram(0);
#else
		for (int i = 0; i < m->root_chain->polysets.size(); i++) {
			if (m->root_chain->types[i] == CSGTerm::DIFFERENCE) {
				m->root_chain->polysets[i]->render_surface(PolySet::COLOR_CUTOUT);
				m->root_chain->polysets[i]->render_edges(PolySet::COLOR_CUTOUT);
			} else {
				m->root_chain->polysets[i]->render_surface(PolySet::COLOR_MATERIAL);
				m->root_chain->polysets[i]->render_edges(PolySet::COLOR_MATERIAL);
			}
		}
#endif
	}
}

void MainWindow::viewModeThrownTogether()
{
	viewModeActionsUncheck();
	actViewModeThrownTogether->setChecked(true);
	screen->renderfunc = renderGLThrownTogether;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

