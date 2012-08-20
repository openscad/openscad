// csg test core, used by throwntegether test and opencsg test
#include "csgtestcore.h"

#include "tests-common.h"
#include "system-gl.h"
#include "openscad.h"
#include "parsersettings.h"
#include "builtin.h"
#include "context.h"
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

#include <QCoreApplication>
#include <QTimer>

#include <sstream>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
#include "boosty.h"

using std::string;
using std::vector;
using std::cerr;
using std::cout;

std::string commandline_commands;

//#define DEBUG

class CsgInfo
{
public:
	CsgInfo();
	shared_ptr<CSGTerm> root_norm_term;          // Normalized CSG products
	class CSGChain *root_chain;
	std::vector<shared_ptr<CSGTerm> > highlight_terms;
	CSGChain *highlights_chain;
	std::vector<shared_ptr<CSGTerm> > background_terms;
	CSGChain *background_chain;
	OffscreenView *glview;
};

CsgInfo::CsgInfo() {
        root_chain = NULL;
        highlights_chain = NULL;
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

void enable_opencsg_shaders( OffscreenView *glview )
{
	bool ignore_gl_version = true;
	const char *openscad_disable_gl20_env = getenv("OPENSCAD_DISABLE_GL20");
	if (openscad_disable_gl20_env && !strcmp(openscad_disable_gl20_env, "0"))
		openscad_disable_gl20_env = NULL;
	if (glewIsSupported("GL_VERSION_2_0") && openscad_disable_gl20_env == NULL )
	{
		const char *vs_source =
			"uniform float xscale, yscale;\n"
			"attribute vec3 pos_b, pos_c;\n"
			"attribute vec3 trig, mask;\n"
			"varying vec3 tp, tr;\n"
			"varying float shading;\n"
			"void main() {\n"
			"  vec4 p0 = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
			"  vec4 p1 = gl_ModelViewProjectionMatrix * vec4(pos_b, 1.0);\n"
			"  vec4 p2 = gl_ModelViewProjectionMatrix * vec4(pos_c, 1.0);\n"
			"  float a = distance(vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
			"  float b = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w));\n"
			"  float c = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
			"  float s = (a + b + c) / 2.0;\n"
			"  float A = sqrt(s*(s-a)*(s-b)*(s-c));\n"
			"  float ha = 2.0*A/a;\n"
			"  gl_Position = p0;\n"
			"  tp = mask * ha;\n"
			"  tr = trig;\n"
			"  vec3 normal, lightDir;\n"
			"  normal = normalize(gl_NormalMatrix * gl_Normal);\n"
			"  lightDir = normalize(vec3(gl_LightSource[0].position));\n"
			"  shading = abs(dot(normal, lightDir));\n"
			"}\n";

		const char *fs_source =
			"uniform vec4 color1, color2;\n"
			"varying vec3 tp, tr, tmp;\n"
			"varying float shading;\n"
			"void main() {\n"
			"  gl_FragColor = vec4(color1.r * shading, color1.g * shading, color1.b * shading, color1.a);\n"
			"  if (tp.x < tr.x || tp.y < tr.y || tp.z < tr.z)\n"
			"    gl_FragColor = color2;\n"
			"}\n";

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
		glCompileShader(vs);

		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
		glCompileShader(fs);

		GLuint edgeshader_prog = glCreateProgram();
		glAttachShader(edgeshader_prog, vs);
		glAttachShader(edgeshader_prog, fs);
		glLinkProgram(edgeshader_prog);

		glview->shaderinfo[0] = edgeshader_prog;
		glview->shaderinfo[1] = glGetUniformLocation(edgeshader_prog, "color1");
		glview->shaderinfo[2] = glGetUniformLocation(edgeshader_prog, "color2");
		glview->shaderinfo[3] = glGetAttribLocation(edgeshader_prog, "trig");
		glview->shaderinfo[4] = glGetAttribLocation(edgeshader_prog, "pos_b");
		glview->shaderinfo[5] = glGetAttribLocation(edgeshader_prog, "pos_c");
		glview->shaderinfo[6] = glGetAttribLocation(edgeshader_prog, "mask");
		glview->shaderinfo[7] = glGetUniformLocation(edgeshader_prog, "xscale");
		glview->shaderinfo[8] = glGetUniformLocation(edgeshader_prog, "yscale");

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr, "OpenGL Error: %s\n", gluErrorString(err));
		}

		GLint status;
		glGetProgramiv(edgeshader_prog, GL_LINK_STATUS, &status);
		if (status == GL_FALSE) {
			int loglen;
			char logbuffer[1000];
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			fprintf(stderr, "OpenGL Program Linker Error:\n%.*s", loglen, logbuffer);
		} else {
			int loglen;
			char logbuffer[1000];
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			if (loglen > 0) {
				fprintf(stderr, "OpenGL Program Link OK:\n%.*s", loglen, logbuffer);
			}
			glValidateProgram(edgeshader_prog);
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			if (loglen > 0) {
				fprintf(stderr, "OpenGL Program Validation results:\n%.*s", loglen, logbuffer);
			}
		}
	}
	glview->shaderinfo[9] = glview->width;
	glview->shaderinfo[10] = glview->height;
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

	QCoreApplication app(argc, argv);

	fs::path original_path = fs::current_path();

	std::string currentdir = boosty::stringy( fs::current_path() );

	parser_init(QCoreApplication::instance()->applicationDirPath().toStdString());
	add_librarydir(boosty::stringy(fs::path(QCoreApplication::instance()->applicationDirPath().toStdString()) / "../libraries"));

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

	if (!sysinfo_dump) {
		if (fs::path(filename).has_parent_path()) {
			fs::current_path(fs::path(filename).parent_path());
		}
	}

	AbstractNode::resetIndexCounter();
	AbstractNode *absolute_root_node = root_module->evaluate(&root_ctx, &root_inst);
	AbstractNode *root_node;
	// Do we have an explicit root node (! modifier)?
	if (!(root_node = find_root_tag(absolute_root_node))) root_node = absolute_root_node;

	Tree tree(root_node);

	CsgInfo csgInfo = CsgInfo();
	CGALEvaluator cgalevaluator(tree);
	CSGTermEvaluator evaluator(tree, &cgalevaluator.psevaluator);
	shared_ptr<CSGTerm> root_raw_term = evaluator.evaluateCSGTerm(*root_node, 
																																csgInfo.highlight_terms, 
																																csgInfo.background_terms);

	if (!root_raw_term) {
		cerr << "Error: CSG generation failed! (no top level object found)\n";
		return 1;
	}

	// CSG normalization
	CSGTermNormalizer normalizer(5000);
	csgInfo.root_norm_term = normalizer.normalize(root_raw_term);
	if (csgInfo.root_norm_term) {
		csgInfo.root_chain = new CSGChain();
		csgInfo.root_chain->import(csgInfo.root_norm_term);
		fprintf(stderr, "Normalized CSG tree has %d elements\n", int(csgInfo.root_chain->polysets.size()));
	}
	else {
		csgInfo.root_chain = NULL;
		fprintf(stderr, "WARNING: CSG normalization resulted in an empty tree\n");
	}

	if (csgInfo.highlight_terms.size() > 0) {
		cerr << "Compiling highlights (" << csgInfo.highlight_terms.size() << " CSG Trees)...\n";
		
		csgInfo.highlights_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.highlight_terms.size(); i++) {
			csgInfo.highlight_terms[i] = normalizer.normalize(csgInfo.highlight_terms[i]);
			csgInfo.highlights_chain->import(csgInfo.highlight_terms[i]);
		}
	}
	
	if (csgInfo.background_terms.size() > 0) {
		cerr << "Compiling background (" << csgInfo.background_terms.size() << " CSG Trees)...\n";
		
		csgInfo.background_chain = new CSGChain();
		for (unsigned int i = 0; i < csgInfo.background_terms.size(); i++) {
			csgInfo.background_terms[i] = normalizer.normalize(csgInfo.background_terms[i]);
			csgInfo.background_chain->import(csgInfo.background_terms[i]);
		}
	}
	
	fs::current_path(original_path);

	try {
		csgInfo.glview = new OffscreenView(512,512);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i. Exiting.\n", error);
		exit(1);
	}
	enable_opencsg_shaders(csgInfo.glview);

	if (sysinfo_dump) cout << info_dump(csgInfo.glview);
	Vector3d center(0,0,0);
	double radius = 1.0;

	if (csgInfo.root_chain) {
		BoundingBox bbox = csgInfo.root_chain->getBoundingBox();
		center = (bbox.min() + bbox.max()) / 2;
		radius = (bbox.max() - bbox.min()).norm() / 2;
	}
	Vector3d cameradir(1, 1, -0.5);
	Vector3d camerapos = center - radius*1.8*cameradir;
	csgInfo.glview->setCamera(camerapos, center);

	OpenCSGRenderer opencsgRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain);

	if (test_type == TEST_THROWNTOGETHER)
		csgInfo.glview->setRenderer(&thrownTogetherRenderer);
	else
		csgInfo.glview->setRenderer(&opencsgRenderer);

	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
  // FIXME: This is necessary for Mac OS X 10.7 for now. kintel 20120527.
	OpenCSG::setOption(OpenCSG::AlgorithmSetting, OpenCSG::Goldfeather);

	csgInfo.glview->paintGL();
	
	csgInfo.glview->save(outfilename);
	
	delete root_node;
	delete root_module;

	Builtins::instance(true);

	return 0;
}
