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

#include "core/UserModule.h"

#include <ostream>
#include <memory>
#include <vector>

#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "utils/exceptions.h"
#include "utils/StackCheck.h"
#include "core/ScopeContext.h"
#include "core/Expression.h"
#include "utils/printutils.h"
#include "utils/compiler_specific.h"
#include <cstddef>
#include <sstream>
#include <string>

std::vector<std::string> StaticModuleNameStack::stack;

static void NOINLINE print_err(std::string name, const Location& loc, const std::shared_ptr<const Context>& context){
  LOG(message_group::Error, loc, context->documentRoot(), "Recursion detected calling module '%1$s'", name);
}

static void NOINLINE print_trace(const UserModule *mod, const std::shared_ptr<const UserModuleContext>& context, const AssignmentList& parameters){
  std::stringstream stream;
  if (parameters.size() == 0) {
    //nothing to do
  } else if (StackCheck::inst().check()) {
    stream << "...";
  } else {
    bool first = true;
    for (const auto& assignment : parameters) {
      if (first) {
        first = false;
      } else {
        stream << ", ";
      }
      if (!assignment->getName().empty()) {
        stream << assignment->getName();
        stream << " = ";
      }
      try {
        stream << context->lookup_variable(assignment->getName(), Location::NONE);
      } catch (EvaluationException& e) {
        stream << "...";
      }
    }
  }
  LOG(message_group::Trace, mod->location(), context->documentRoot(), "call of '%1$s(%2$s)'",
      mod->name, stream.str()
      );
}

std::shared_ptr<AbstractNode> UserModule::instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const
{
  if (StackCheck::inst().check()) {
    print_err(inst->name(), loc, context);
    throw RecursionException::create("module", inst->name(), loc);
    return nullptr;
  }

  StaticModuleNameStack name{inst->name()}; // push on static stack, pop at end of method!
  ContextHandle<UserModuleContext> module_context{Context::create<UserModuleContext>(
                                                    defining_context,
                                                    this,
                                                    inst->location(),
                                                    Arguments(inst->arguments, context),
                                                    Children(&inst->scope, context)
                                                    )};
#if 0 && DEBUG
  PRINTDB("UserModuleContext for module %s(%s):\n", this->name % STR(this->parameters));
  PRINTDB("%s", module_context->dump());
#endif

  std::shared_ptr<AbstractNode> ret;
  try{
    ret = this->body.instantiateModules(*module_context, std::make_shared<GroupNode>(inst, std::string("module ") + this->name));
  } catch (EvaluationException& e) {
    if (OpenSCAD::traceUsermoduleParameters && e.traceDepth > 0) {
      print_trace(this, *module_context, this->parameters);
      e.traceDepth--;
    }
    throw;
  }
  return ret;
}

void UserModule::print(std::ostream& stream, const std::string& indent) const
{
  std::string tab;
  if (!this->name.empty()) {
    stream << indent << "module " << this->name << "(";
    for (size_t i = 0; i < this->parameters.size(); ++i) {
      const auto& parameter = this->parameters[i];
      if (i > 0) stream << ", ";
      stream << parameter->getName();
      if (parameter->getExpr()) stream << " = " << *parameter->getExpr();
    }
    stream << ") {\n";
    tab = "\t";
  }
  body.print(stream, indent + tab);
  if (!this->name.empty()) {
    stream << indent << "}\n";
  }
}
