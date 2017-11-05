
/* OpenGL helper functions */

#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include "system-gl.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace std;
using namespace boost;

double gl_version()
{
	string tmp((const char *)glGetString(GL_VERSION));
	vector<string> strs;
	split(strs, tmp, is_any_of("."));
	stringstream out;
	if (strs.size() >= 2) out << strs[0] << "." << strs[1];
	else out << "0.0";
	double d;
	out >> d;
	return d;
}

string glew_extensions_dump()
{
	std::string tmp;
	if (gl_version() >= 3.0) {
		GLint numexts = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numexts);
		for (int i = 0; i < numexts; i++) {
			tmp += (const char *) glGetStringi(GL_EXTENSIONS, i);
			tmp += " ";
		}
	}
	else {
		tmp = (const char *) glGetString(GL_EXTENSIONS);
	}
	vector<string> extensions;
	split(extensions, tmp, is_any_of(" "));
	sort(extensions.begin(), extensions.end());
	stringstream out;
	out << "GL Extensions:";
	for (unsigned int i = 0; i < extensions.size(); i++)
		out << extensions[i] << "\n";
	return out.str();
}

string glew_dump()
{
	GLint rbits, gbits, bbits, abits, dbits, sbits;
	glGetIntegerv(GL_RED_BITS, &rbits);
	glGetIntegerv(GL_GREEN_BITS, &gbits);
	glGetIntegerv(GL_BLUE_BITS, &bbits);
	glGetIntegerv(GL_ALPHA_BITS, &abits);
	glGetIntegerv(GL_DEPTH_BITS, &dbits);
	glGetIntegerv(GL_STENCIL_BITS, &sbits);

	stringstream out;
	out << "GLEW version: " << glewGetString(GLEW_VERSION)
			<< "\nOpenGL Version: " << (const char *)glGetString(GL_VERSION)
			<< "\nGL Renderer: " << (const char *)glGetString(GL_RENDERER)
			<< "\nGL Vendor: " << (const char *)glGetString(GL_VENDOR)
			<< boost::format("\nRGBA(%d%d%d%d), depth(%d), stencil(%d)") %
		rbits % gbits % bbits % abits % dbits % sbits;
	out << "\nGL_ARB_framebuffer_object: "
			<< (glewIsSupported("GL_ARB_framebuffer_object") ? "yes" : "no")
			<< "\nGL_EXT_framebuffer_object: "
			<< (glewIsSupported("GL_EXT_framebuffer_object") ? "yes" : "no")
			<< "\nGL_EXT_packed_depth_stencil: "
			<< (glewIsSupported("GL_EXT_packed_depth_stencil") ? "yes" : "no")
			<< "\n";
	return out.str();
}

bool report_glerror(const char *function)
{
	GLenum tGLErr = glGetError();
	if (tGLErr != GL_NO_ERROR) {
		std::ostringstream hexErr;
		hexErr << hex << tGLErr;
		cerr << "OpenGL error 0x" << hexErr.str() << ": " << gluErrorString(tGLErr) << " after " << function << endl;
		return true;
	}
	return false;
}

