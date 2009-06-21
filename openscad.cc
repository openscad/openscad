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

#include "openscad.h"

int main()
{
	int rc = 0;

	initialize_builtin_functions();
	initialize_builtin_modules();

	Context ctx(NULL);
	ctx.functions_p = &builtin_functions;
	ctx.modules_p = &builtin_modules;

	AbstractModule *root_module = parse(stdin, 0);
	QString text = root_module->dump("", "*");
	printf("%s", text.toAscii().data());
	delete root_module;

	destroy_builtin_functions();
	destroy_builtin_modules();

	return rc;
}

