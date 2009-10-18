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

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSet>
#include <getopt.h>

static void help(const char *progname)
{
	fprintf(stderr, "Usage: %s [ -m make_command ] [ filename ]\n", progname);
	fprintf(stderr, "       %s { -s stl_file | -o off_file } [ -d deps_file ] [ -m make_command ] filename\n", progname);
	exit(1);
}

const char *make_command = NULL;
QSet<QString> dependencies;

void handle_dep(QString filename)
{
	if (filename.startsWith("/"))
		dependencies.insert(filename);
	else
		dependencies.insert(QDir::currentPath() + QString("/") + filename);
	if (!QFile(filename).exists() && make_command) {
		char buffer[4096];
		snprintf(buffer, 4096, "%s '%s", make_command, filename.replace("'", "'\\''").toUtf8().data());
		system(buffer);
	}
}

int main(int argc, char **argv)
{
	int rc;

	initialize_builtin_functions();
	initialize_builtin_modules();

#ifdef Q_WS_X11
	// see <http://qt.nokia.com/doc/4.5/qapplication.html#QApplication-2>:
	// On X11, the window system is initialized if GUIenabled is true. If GUIenabled
	// is false, the application does not connect to the X server. On Windows and
	// Macintosh, currently the window system is always initialized, regardless of the
	// value of GUIenabled. This may change in future versions of Qt.
	bool useGUI = getenv("DISPLAY") != 0;
#else
	bool useGUI = true;
#endif
	QApplication app(argc, argv, useGUI);

	const char *filename = NULL;
	const char *stl_output_file = NULL;
	const char *off_output_file = NULL;
	const char *deps_output_file = NULL;

	int opt;

	while ((opt = getopt(argc, argv, "s:o:d:m:")) != -1)
	{
		switch (opt)
		{
		case 's':
			if (stl_output_file || off_output_file)
				help(argv[0]);
			stl_output_file = optarg;
			break;
		case 'o':
			if (stl_output_file || off_output_file)
				help(argv[0]);
			off_output_file = optarg;
			break;
		case 'd':
			if (deps_output_file)
				help(argv[0]);
			deps_output_file = optarg;
			break;
		case 'm':
			if (make_command)
				help(argv[0]);
			make_command = optarg;
			break;
		default:
			help(argv[0]);
		}
	}

	if (optind < argc)
		filename = argv[optind++];

	if (optind != argc)
		help(argv[0]);

	if (stl_output_file || off_output_file)
	{
		if (!filename)
			help(argv[0]);

#ifdef ENABLE_CGAL
		Context root_ctx;
		root_ctx.functions_p = &builtin_functions;
		root_ctx.modules_p = &builtin_modules;
		root_ctx.set_variable("$fn", Value(0.0));
		root_ctx.set_variable("$fs", Value(1.0));
		root_ctx.set_variable("$fa", Value(12.0));
		root_ctx.set_variable("$t", Value(0.0));

		AbstractModule *root_module;
		ModuleInstanciation root_inst;
		AbstractNode *root_node;

		handle_dep(filename);
		FILE *fp = fopen(filename, "rt");
		if (!fp) {
			fprintf(stderr, "Can't open input file `%s'!\n", filename);
			exit(1);
		} else {
			QString text;
			char buffer[513];
			int rc;
			while ((rc = fread(buffer, 1, 512, fp)) > 0) {
				buffer[rc] = 0;
				text += buffer;
			}
			fclose(fp);
			root_module = parse(text.toAscii().data(), false);
		}

		QString original_path = QDir::currentPath();
		QFileInfo fileInfo(filename);
		QDir::setCurrent(fileInfo.dir().absolutePath());

		AbstractNode::idx_counter = 1;
		root_node = root_module->evaluate(&root_ctx, &root_inst);

		CGAL_Nef_polyhedron *root_N;
		root_N = new CGAL_Nef_polyhedron(root_node->render_cgal_nef_polyhedron());

		QDir::setCurrent(original_path);

		if (deps_output_file) {
			fp = fopen(deps_output_file, "wt");
			if (!fp) {
				fprintf(stderr, "Can't open dependencies file `%s' for writing!\n", deps_output_file);
				exit(1);
			}
			fprintf(fp, "%s:", stl_output_file ? stl_output_file : off_output_file);
			QSetIterator<QString> i(dependencies);
			while (i.hasNext())
				fprintf(fp, " \\\n\t%s", i.next().toUtf8().data());
			fprintf(fp, "\n");
			fclose(fp);
		}

		if (stl_output_file)
			export_stl(root_N, stl_output_file, NULL);

		if (off_output_file)
			export_off(root_N, off_output_file, NULL);

		delete root_node;
		delete root_N;
#else
		fprintf(stderr, "OpenSCAD has been compiled without CGAL support!\n");
		exit(1);
#endif
	}
	else if (useGUI)
	{
		MainWindow *m;
		if (filename)
			m = new MainWindow(filename);
		else
			m = new MainWindow();

		m->show();
		m->resize(800, 600);
		m->s1->setSizes(QList<int>() << 400 << 400);
		m->s2->setSizes(QList<int>() << 400 << 200);

		app.connect(m, SIGNAL(destroyed()), &app, SLOT(quit()));
		rc = app.exec();
	}
	else
	{
		fprintf(stderr, "Requested GUI mode but can't open display!\n");
		exit(1);
	}

	destroy_builtin_functions();
	destroy_builtin_modules();

	return rc;
}

