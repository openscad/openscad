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

#include "Arguments.h"
#include "Children.h"
#include "Context.h"
#include "module.h"
#include "ModuleInstantiation.h"

BuiltinModule::BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const ModuleInstantiation *, const std::shared_ptr<const Context>&), const Feature *feature) :
  AbstractModule(feature),
  do_instantiate(instantiate)
{}

BuiltinModule::BuiltinModule(std::shared_ptr<AbstractNode>(*instantiate)(const ModuleInstantiation *, Arguments, const Children&), const Feature *feature) :
  AbstractModule(feature)
{
  do_instantiate = [instantiate](const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) {
      return instantiate(inst, Arguments(inst->arguments, context), Children(&inst->scope, context));
    };
}

// In theory this would be used to print a module reference that pointed
// at a builtin module.  However, there's currently no syntactic way to
// create such a thing.
void BuiltinModule::print(std::ostream& stream, const std::string& indent) const
{
    stream << "(builtin)";
}

std::shared_ptr<AbstractNode> BuiltinModule::instantiate(const std::shared_ptr<const Context>& /*defining_context*/, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const
{
  return do_instantiate(inst, context);
}
