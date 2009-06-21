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

#include <CGAL/IO/Polyhedron_iostream.h>

#include <fstream>
#include <iostream>

int main()
{
	int rc = 0;

	initialize_builtin_functions();
	initialize_builtin_modules();

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
	N = root_node->render_cgal_nef_polyhedron();
	N.convert_to_Polyhedron(P);

	std::ofstream outFile("output.off");
	if (outFile.fail()) {
		std::cerr << "unable to open output file merged.off for writing!" << std::endl;
		exit(1);
	}
	outFile << P;

	delete root_module;

	destroy_builtin_functions();
	destroy_builtin_modules();

        return rc;
}

