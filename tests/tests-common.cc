#include "tests-common.h"
#include "openscad.h"
#include "module.h"
#include "handle_dep.h"

#include <QFile>
#include <QFileInfo>
#include <sstream>

Module *parsefile(const char *filename)
{
	Module *root_module = NULL;

	QFileInfo fileInfo(filename);
	handle_dep(filename);
	FILE *fp = fopen(filename, "rt");
	if (!fp) {
		fprintf(stderr, "Can't open input file `%s'!\n", filename);
	} else {
		std::stringstream text;
		char buffer[513];
		int ret;
		while ((ret = fread(buffer, 1, 512, fp)) > 0) {
			buffer[ret] = 0;
			text << buffer;
		}
		fclose(fp);
		text << "\n" << commandline_commands;
		root_module = parse(text.str().c_str(), fileInfo.absolutePath().toLocal8Bit(), false);
		if (root_module) {
			root_module->handleDependencies();
		}
	}
	return root_module;
}
