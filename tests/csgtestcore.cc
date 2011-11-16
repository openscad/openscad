// csg test core, used by throwntegether test and opencsg test
#include "csgtestcore.h"

#include "tests-common.h"
#include "system-gl.h"
#include "openscad.h"
#include "builtin.h"
#include "context.h"
#include "node.h"
#include "module.h"
#include "polyset.h"
#include "Tree.h"
#include "CSGTermEvaluator.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"

#include "OpenCSGRenderer.h"
#include "ThrownTogetherRenderer.h"

#include "csgterm.h"
#include "OffscreenView.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTimer>

#include <sstream>
#include <vector>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

using std::string;
using std::vector;
using std::cerr;
using std::cout;

std::string commandline_commands;
QString librarydir;

//#define DEBUG

class CsgInfo
{
public:
	CsgInfo();
	CSGTerm *root_norm_term;          // Normalized CSG products
	class CSGChain *root_chain;
	vector<CSGTerm*> highlight_terms;
	CSGChain *highlights_chain;
	vector<CSGTerm*> background_terms;
	CSGChain *background_chain;
	OffscreenView *glview;
};

CsgInfo::CsgInfo() {
        root_norm_term = NULL;
        root_chain = NULL;
        highlight_terms = vector<CSGTerm*>();
        highlights_chain = NULL;
        background_terms = vector<CSGTerm*>();
        background_chain = NULL;
        glview = NULL;
}

AbstractNode *find_root_tag(AbstractNode *n)
{
	foreach(AbstractNode *v, n->children) {
		if (v->modinst->tag_root) return v;
		if (AbstractNode *vroot = find_root_tag(v)) return vroot;
	}
	return NULL;
}

string info_dump(OffscreenView *glview)
{
	assert(glview);

#ifdef __GNUG__
#define compiler_info "GCC " << __VERSION__
#elif defined(_MSC_VER)
#define compiler_info "MSVC " << _MSC_FULL_VER
#else
#define compiler_info "unknown compiler"
#endif

	std::stringstream out;
	out << "OpenSCAD info dump:"
	    << "\nOpenSCAD Year/Month/Day: " << int(OPENSCAD_YEAR) << "."
	    << int(OPENSCAD_MONTH) << "."
#ifdef OPENSCAD_DAY
	    << int(OPENSCAD_DAY);
#endif
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
	    << "\nOpenSCAD Version: " << TOSTRING(OPENSCAD_VERSION)
            << "\nCompiled by: " << compiler_info
	    << "\nBoost version: " << BOOST_LIB_VERSION
	    << "\nEigen version: " << EIGEN_WORLD_VERSION << "."
	    << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	    // << "\nCGAL version: " << CGAL_VERSION ???
	    // << "\nOpenCSG" << ???
	    << "\n" << glview->getInfo()
	    << "\n";

	return out.str();
}

po::variables_map parse_options(int argc, char *argv[])
{
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help,h", "help message")//;
                ("info,i", "information on GLEW, OpenGL, OpenSCAD, and OS")//;

//        po::options_description hidden("Hidden options");
//        hidden.add_options()
                ("input-file", po::value< vector<string> >(), "input file")
                ("output-file", po::value< vector<string> >(), "ouput file");

        po::positional_options_description p;
        p.add("input-file", 1).add("output-file", 1);

        po::options_description all_options;
        all_options.add(desc); // .add(hidden);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(all_options).positional(p).run(), vm);
	po::notify(vm);

	return vm;
}

int csgtestcore(int argc, char *argv[], test_type_e test_type)
{
	bool sysinfo_dump = false;
	const char *filename, *outfilename = NULL;
	po::variables_map vm;
	try {
		vm = parse_options(argc, argv);
	} catch ( po::error e ) {
		cerr << "error parsing options\n";
	}
	if (vm.count("info")) sysinfo_dump = true;
	if (vm.count("input-file"))
		filename = vm["input-file"].as< vector<string> >().begin()->c_str();
	if (vm.count("output-file"))
		outfilename = vm["output-file"].as< vector<string> >().begin()->c_str();

	if ((!filename || !outfilename) && !sysinfo_dump) {
		cerr << "Usage: " << argv[0] << " <file.scad> <output.png>\n";
		exit(1);
	}

	Builtins::instance()->initialize();

	QApplication app(argc, argv, false);

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
	register_builtin(root_ctx);

	AbstractModule *root_module;
	ModuleInstantiation root_inst;

	if (sysinfo_dump)
		root_module = parse("sphere();","",false);
	else
		root_module = parsefile(filename);

	if (!root_module) {
		exit(1);
	}

	QFileInfo fileInfo(filename);
	QDir::setCurrent(fileInfo.absolutePath());

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CsgInfo csgInfo = CsgInfo();
	CGALEvaluator cgalevaluator(tree);
	CSGTermEvaluator evaluator(tree, &cgalevaluator.psevaluator);
	CSGTerm *root_raw_term = evaluator.evaluateCSGTerm(*root_node, 
																										 csgInfo.highlight_terms, 
																										 csgInfo.background_terms);

	if (!root_raw_term) {
		cerr << "Error: CSG generation failed! (no top level object found)\n";
		return 1;
	}

	// CSG normalization
	csgInfo.root_norm_term = root_raw_term->link();
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
	fprintf(stderr, "Normalized CSG tree has %d elements\n", csgInfo.root_chain->polysets.size());
	
	if (csgInfo.highlight_terms.size() > 0) {
		cerr << "Compiling highlights (" << csgInfo.highlight_terms.size() << " CSG Trees)...\n";
		
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
	
	QDir::setCurrent(original_path.absolutePath());

	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		exit(1);
	}
	if (sysinfo_dump) cout << info_dump(csgInfo.glview);
	BoundingBox bbox = csgInfo.root_chain->getBoundingBox();

	Vector3d center = (bbox.min() + bbox.max()) / 2;
	double radius = (bbox.max() - bbox.min()).norm() / 2;


	Vector3d cameradir(1, 1, -0.5);
	Vector3d camerapos = center - radius*1.8*cameradir;
	csgInfo.glview->setCamera(camerapos, center);

	OpenCSGRenderer opencsgRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain);

	if (test_type == TEST_THROWNTOGETHER)
		csgInfo.glview->setRenderer(&thrownTogetherRenderer);
	else
		csgInfo.glview->setRenderer(&opencsgRenderer);

	csgInfo.glview->paintGL();

	csgInfo.glview->save(outfilename);
	
	Builtins::instance(true);

	return 0;
}
