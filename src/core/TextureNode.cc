/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
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

#include "function.h"
#include "Arguments.h"
#include "Expression.h"
#include "Builtins.h"
#include "printutils.h"
#include "memory.h"
#include "UserModule.h"
#include "degree_trig.h"
#include "FreetypeRenderer.h"
#include "Parameters.h"
#include "import.h"
#include "fileutils.h"

Value builtin_texture(Arguments arguments, const Location& loc)
{
	printf("b\n");
  auto session = arguments.session();
  const Parameters parameters = Parameters::parse(std::move(arguments), loc, {}, {"file"});
  std::string raw_filename = parameters.get("file", "");
  std::string file = lookup_file(raw_filename, loc.filePath().parent_path().string(), parameters.documentRoot());
  printf("texture is %s\n",file.c_str());
  return {std::fabs(1.0)};
}

void register_builtin_texture()
{
	printf("a\n");
  Builtins::init("texture", new BuiltinFunction(&builtin_texture),
  {
    "is_object(arg) -> boolean",
  });
}
