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

// #define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"

#include <QApplication>

#if 0
void report_func(const class AbstractNode*, void*, int mark)
{
	printf("CSG rendering progress: %.2f%%\n", (mark*100.0) / progress_report_count);
}
#endif

int main(int argc, char **argv)
{
	int rc;

	initialize_builtin_functions();
	initialize_builtin_modules();

	QApplication a(argc, argv);
	MainWindow *m;

	if (argc > 1)
		m = new MainWindow(argv[1]);
	else
		m = new MainWindow();

	m->show();
	m->resize(800, 600);
	m->s1->setSizes(QList<int>() << 400 << 400);
	m->s2->setSizes(QList<int>() << 400 << 200);

	a.connect(m, SIGNAL(destroyed()), &a, SLOT(quit()));
	rc = a.exec();

#if 0
	Context root_ctx(NULL);
	root_ctx.functions_p = &builtin_functions;
	root_ctx.modules_p = &builtin_modules;

	AbstractModule *root_module = parse(stdin, 0);

	printf("--- Abstract Syntax Tree ---\n");
	QString ast_text = root_module->dump("", "**root**");
	printf("%s", ast_text.toAscii().data());

	AbstractNode *root_node = root_module->evaluate(&root_ctx, QVector<QString>(), QVector<Value>(), QVector<AbstractNode*>());

	printf("--- Compiled CSG Tree ---\n");
	QString csg_text = root_node->dump("");
	printf("%s", csg_text.toAscii().data());

	CGAL_Nef_polyhedron N;
	CGAL_Polyhedron P;

	progress_report_prep(root_node, report_func, NULL);
	N = root_node->render_cgal_nef_polyhedron();
	progress_report_fin();
	printf("CSG rendering finished.\n");

	N.convert_to_Polyhedron(P);

	std::ofstream outFile("output.off");
	if (outFile.fail()) {
		std::cerr << "unable to open output file merged.off for writing!" << std::endl;
		exit(1);
	}
	outFile << P;

	delete root_node;
	delete root_module;
#endif

	destroy_builtin_functions();
	destroy_builtin_modules();

	return rc;
}

