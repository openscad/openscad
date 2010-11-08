#include "openscad.h"
#include "builtin.h"
#include "context.h"
#include "node.h"
#include "module.h"
#include "polyset.h"
#include "Tree.h"
#include "CSGTermRenderer.h"
#include "CGALRenderer.h"
#include "PolySetCGALRenderer.h"

#include "csgterm.h"
#include "render-opencsg.h"
#include <GL/glew.h>
#include "GLView.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>

using std::cerr;

QString commandline_commands;
QString librarydir;
QSet<QString> dependencies;
const char *make_command = NULL;

void handle_dep(QString filename)
{
	if (filename.startsWith("/"))
		dependencies.insert(filename);
	else
		dependencies.insert(QDir::currentPath() + QString("/") + filename);
	if (!QFile(filename).exists() && make_command) {
		char buffer[4096];
		snprintf(buffer, 4096, "%s '%s'", make_command, filename.replace("'", "'\\''").toUtf8().data());
		system(buffer); // FIXME: Handle error
	}
}

// static void renderfunc(void *vp)
// {
// 	glClearColor(1.0, 0.0, 0.0, 0.0);
// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
// }

struct CsgInfo
{
	CSGTerm *root_norm_term;          // Normalized CSG products
	class CSGChain *root_chain;
	std::vector<CSGTerm*> highlight_terms;
	CSGChain *highlights_chain;
	std::vector<CSGTerm*> background_terms;
	CSGChain *background_chain;
	GLView *glview;
};

static void renderGLThrownTogetherChain(CSGChain *chain, bool highlight, bool background, bool fberror)
{
	glDepthFunc(GL_LEQUAL);
	QHash<QPair<PolySet*,double*>,int> polySetVisitMark;
	bool showEdges = false;
	for (int i = 0; i < chain->polysets.size(); i++) {
		if (polySetVisitMark[QPair<PolySet*,double*>(chain->polysets[i], chain->matrices[i])]++ > 0)
			continue;
		double *m = chain->matrices[i];
		glPushMatrix();
		glMultMatrixd(m);
		int csgmode = chain->types[i] == CSGTerm::TYPE_DIFFERENCE ? PolySet::CSGMODE_DIFFERENCE : PolySet::CSGMODE_NORMAL;
		if (highlight) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20), m);
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_HIGHLIGHT, PolySet::csgmode_e(csgmode + 20));
				glEnable(GL_LIGHTING);
			}
		} else if (background) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10), m);
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_BACKGROUND, PolySet::csgmode_e(csgmode + 10));
				glEnable(GL_LIGHTING);
			}
		} else if (fberror) {
			if (highlight) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 20), m);
			} else if (background) {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode + 10), m);
			} else {
				chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
			}
		} else if (m[16] >= 0 || m[17] >= 0 || m[18] >= 0 || m[19] >= 0) {
			glColor4d(m[16], m[17], m[18], m[19]);
			chain->polysets[i]->render_surface(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode), m);
			if (showEdges) {
				glDisable(GL_LIGHTING);
				glColor4d((m[16]+1)/2, (m[17]+1)/2, (m[18]+1)/2, 1.0);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_NONE, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		} else if (chain->types[i] == CSGTerm::TYPE_DIFFERENCE) {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode), m);
			if (showEdges) {
				glDisable(GL_LIGHTING);
				chain->polysets[i]->render_edges(PolySet::COLORMODE_CUTOUT, PolySet::csgmode_e(csgmode));
				glEnable(GL_LIGHTING);
			}
		} else {
			chain->polysets[i]->render_surface(PolySet::COLORMODE_MATERIAL, PolySet::csgmode_e(csgmode), m);
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
	CsgInfo *csgInfo = (CsgInfo *)vp;
	if (csgInfo->root_chain) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		renderGLThrownTogetherChain(csgInfo->root_chain, false, false, false);
		glCullFace(GL_FRONT);
		glColor3ub(255, 0, 255);
		renderGLThrownTogetherChain(csgInfo->root_chain, false, false, true);
		glDisable(GL_CULL_FACE);
	}
	if (csgInfo->background_chain)
		renderGLThrownTogetherChain(csgInfo->background_chain, false, true, false);
	if (csgInfo->highlights_chain)
		renderGLThrownTogetherChain(csgInfo->highlights_chain, true, false, false);
}

static void renderGLviaOpenCSG(void *vp)
{
	CsgInfo *csgInfo = (CsgInfo *)vp;
	static bool glew_initialized = false;
	if (!glew_initialized) {
		glew_initialized = true;
		glewInit();
	}
#ifdef ENABLE_MDI
	OpenCSG::setContext(csgInfo->glview->opencsg_id);
#endif
	if (csgInfo->root_chain) {
		GLint *shaderinfo = csgInfo->glview->shaderinfo;
		if (!shaderinfo[0]) shaderinfo = NULL;
		renderCSGChainviaOpenCSG(csgInfo->root_chain, NULL, false, false);
		if (csgInfo->background_chain) {
			renderCSGChainviaOpenCSG(csgInfo->background_chain, NULL, false, true);
		}
		if (csgInfo->highlights_chain) {
			renderCSGChainviaOpenCSG(csgInfo->highlights_chain, NULL, true, false);
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.scad>\n", argv[0]);
		exit(1);
	}

	const char *filename = argv[1];

	initialize_builtin_functions();
	initialize_builtin_modules();

	QApplication app(argc, argv);

	QDir original_path = QDir::current();

	QString currentdir = QDir::currentPath();

	QDir libdir(QApplication::instance()->applicationDirPath());
#ifdef Q_WS_MAC
	libdir.cd("../Resources"); // Libraries can be bundled
	if (!libdir.exists("libraries")) libdir.cd("../../..");
#elif defined(Q_OS_UNIX)
	if (libdir.cd("../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else
	if (libdir.cd("../../share/openscad/libraries")) {
		librarydir = libdir.path();
	} else
	if (libdir.cd("../../libraries")) {
		librarydir = libdir.path();
	} else
#endif
	if (libdir.cd("libraries")) {
		librarydir = libdir.path();
	}

	Context root_ctx;
	root_ctx.functions_p = &builtin_functions;
	root_ctx.modules_p = &builtin_modules;
	root_ctx.set_variable("$fn", Value(0.0));
	root_ctx.set_variable("$fs", Value(1.0));
	root_ctx.set_variable("$fa", Value(12.0));
	root_ctx.set_variable("$t", Value(0.0));

	Value zero3;
	zero3.type = Value::VECTOR;
	zero3.append(new Value(0.0));
	zero3.append(new Value(0.0));
	zero3.append(new Value(0.0));
	root_ctx.set_variable("$vpt", zero3);
	root_ctx.set_variable("$vpr", zero3);


	AbstractModule *root_module;
	ModuleInstantiation root_inst;
	AbstractNode *root_node;

	QFileInfo fileInfo(filename);
	handle_dep(filename);
	FILE *fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "Can't open input file `%s'!\n", filename);
		exit(1);
	} else {
		QString text;
		char buffer[513];
		int ret;
		while ((ret = fread(buffer, 1, 512, fp)) > 0) {
			buffer[ret] = 0;
			text += buffer;
		}
		fclose(fp);
		root_module = parse((text+commandline_commands).toAscii().data(), fileInfo.absolutePath().toLocal8Bit(), false);
		if (!root_module) {
			exit(1);
		}
	}

	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	root_node = root_module->evaluate(&root_ctx, &root_inst);

	Tree tree(root_node);

	QHash<std::string, CGAL_Nef_polyhedron> cache;
	CGALRenderer cgalrenderer(cache, tree);
	PolySetCGALRenderer psrenderer(cgalrenderer);
	CSGTermRenderer renderer(tree);
	CSGTerm *root_raw_term = renderer.renderCSGTerm(*root_node, NULL, NULL);

	if (!root_raw_term) {
		cerr << "Error: CSG generation failed! (no top level object found)\n";
		return 1;
	}

	CsgInfo csgInfo;
	csgInfo.root_norm_term = root_raw_term->link();
		
	// CSG normalization
	while (1) {
		CSGTerm *n = csgInfo.root_norm_term->normalize();
		csgInfo.root_norm_term->unlink();
		if (csgInfo.root_norm_term == n)
			break;
		csgInfo.root_norm_term = n;
	}
		
	assert(csgInfo.root_norm_term);
	
	csgInfo.root_chain = new CSGChain();
	csgInfo.root_chain->import(csgInfo.root_norm_term);
	
	if (csgInfo.highlight_terms.size() > 0) {
		cerr << "Compiling highlights (" << "  CSG Trees)...\n";
		
		csgInfo.highlights_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.highlight_terms.size(); i++) {
			while (1) {
				CSGTerm *n = csgInfo.highlight_terms[i]->normalize();
				csgInfo.highlight_terms[i]->unlink();
				if (csgInfo.highlight_terms[i] == n)
					break;
				csgInfo.highlight_terms[i] = n;
			}
			csgInfo.highlights_chain->import(csgInfo.highlight_terms[i]);
		}
	}
	
	if (csgInfo.background_terms.size() > 0) {
		cerr << "Compiling background (" << csgInfo.background_terms.size() << " CSG Trees)...\n";
		
		csgInfo.background_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.background_terms.size(); i++) {
			while (1) {
				CSGTerm *n = csgInfo.background_terms[i]->normalize();
				csgInfo.background_terms[i]->unlink();
				if (csgInfo.background_terms[i] == n)
					break;
				csgInfo.background_terms[i] = n;
			}
			csgInfo.background_chain->import(csgInfo.background_terms[i]);
		}
	}
	
  // QGLFormat fmt;
	// fmt.setDirectRendering(false);
	csgInfo.glview = new GLView(NULL);	
// FIXME: Thrown together works, OpenCSG not..
//	csgInfo.glview->setRenderFunc(renderGLviaOpenCSG, &csgInfo);
	csgInfo.glview->setRenderFunc(renderGLThrownTogether, &csgInfo);

	QDir::setCurrent(original_path.absolutePath());
	csgInfo.glview->renderPixmap(512, 512).save("out.png");


	destroy_builtin_functions();
	destroy_builtin_modules();

	return 0;
}
