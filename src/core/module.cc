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

#include "core/module.h"

#include <memory>
#include "core/Arguments.h"
#include "core/Children.h"
#include "core/Context.h"
#include "core/ModuleInstantiation.h"

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

std::shared_ptr<AbstractNode> BuiltinModule::instantiate(const std::shared_ptr<const Context>& /*defining_context*/, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const
{
  return do_instantiate(inst, context);
}
