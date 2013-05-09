// csg test core, used by throwntegether test and opencsg test
#include "csgtestcore.h"

#include "tests-common.h"
#include "system-gl.h"
#include "openscad.h"
#include "parsersettings.h"
#include "builtin.h"
#include "modcontext.h"
#include "node.h"
#include "module.h"
#include "polyset.h"
#include "Tree.h"
#include "CSGTermEvaluator.h"
#include "CGALEvaluator.h"
#include "PolySetCGALEvaluator.h"

#include <opencsg.h>
#include "OpenCSGRenderer.h"
#include "ThrownTogetherRenderer.h"

#include "csgterm.h"
#include "csgtermnormalizer.h"
#include "OffscreenView.h"

#include <sstream>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "CsgInfo.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
#include "boosty.h"

using std::string;
using std::vector;
using std::cerr;
using std::cout;

std::string commandline_commands;

//#define DEBUG

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

#ifndef OPENCSG_VERSION_STRING
#define OPENCSG_VERSION_STRING "unknown, <1.3.2"
#endif

	std::stringstream out;
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
	out << "\nOpenSCAD Version: " << TOSTRING(OPENSCAD_VERSION)
            << "\nCompiled by: " << compiler_info
	    << "\nCompile date: " << __DATE__
	    << "\nBoost version: " << BOOST_LIB_VERSION
	    << "\nEigen version: " << EIGEN_WORLD_VERSION << "."
	    << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION
	    << "\nCGAL version: " << TOSTRING(CGAL_VERSION)
	    << "\nOpenCSG version: " << OPENCSG_VERSION_STRING
	    << "\n" << glview->getRendererInfo()
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
                ("output-file", po::value< vector<string> >(), "output file");

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

	fs::path original_path = fs::current_path();

	std::string currentdir = boosty::stringy( fs::current_path() );

	parser_init(boosty::stringy(fs::path(argv[0]).branch_path()));
	add_librarydir(boosty::stringy(fs::path(argv[0]).branch_path() / "../libraries"));

	ModuleContext top_ctx;
	top_ctx.registerBuiltin();

	FileModule *root_module;
	ModuleInstantiation root_inst("group");

	if (sysinfo_dump)
		root_module = parse("sphere();","",false);
	else
		root_module = parsefile(filename);

	if (!root_module) {
		exit(1);
	}

	if (!sysinfo_dump) {
		fs::path fpath = boosty::absolute(fs::path(filename));
		fs::path fparent = fpath.parent_path();
		fs::current_path(fparent);
		top_ctx.setDocumentPath(fparent.string());
	}

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->instantiate(&top_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CsgInfo csgInfo = CsgInfo();
	if ( !csgInfo.compile_chains( tree ) ) return 1;

	fs::current_path(original_path);

	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		exit(1);
	}

	if (sysinfo_dump) cout << info_dump(csgInfo.glview);
	Camera camera(Camera::VECTOR);
	camera.center << 0,0,0;
	double radius = 1.0;

	if (csgInfo.root_chain) {
		BoundingBox bbox = csgInfo.root_chain->getBoundingBox();
		camera.center = (bbox.min() + bbox.max()) / 2;
		radius = (bbox.max() - bbox.min()).norm() / 2;
	}
	Vector3d cameradir(1, 1, -0.5);
	camera.eye = camera.center - radius*1.8*cameradir;
	csgInfo.glview->setCamera(camera);

	OpenCSGRenderer opencsgRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain);

	if (test_type == TEST_THROWNTOGETHER)
		csgInfo.glview->setRenderer(&thrownTogetherRenderer);
	else
		csgInfo.glview->setRenderer(&opencsgRenderer);

	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);

	csgInfo.glview->paintGL();
	
	if (outfilename) csgInfo.glview->save(outfilename);
	
	delete root_node;
	delete root_module;

	Builtins::instance(true);

	return 0;
}
