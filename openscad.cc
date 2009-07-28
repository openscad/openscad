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

	destroy_builtin_functions();
	destroy_builtin_modules();

	return rc;
}

