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

#include "core/CsgOpNode.h"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "core/Builtins.h"
#include "core/Children.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "geometry/PolySet.h"
#include "core/enums.h"
#include "core/module.h"

static std::shared_ptr<AbstractNode> builtin_union(const ModuleInstantiation *inst, Arguments arguments,
                                                   const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::UNION));
}

static std::shared_ptr<AbstractNode> builtin_difference(const ModuleInstantiation *inst,
                                                        Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::DIFFERENCE));
}

static std::shared_ptr<AbstractNode> builtin_intersection(const ModuleInstantiation *inst,
                                                          Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::INTERSECTION));
}

std::string CsgOpNode::toString() const
{
  std::ostringstream stream;
  stream << this->name() << "(";
  if (r != 0 || fn != 2) stream << " r = " << this->r << " , fn = " << this->fn << " ";
  stream << ")";
  return stream.str();
}

std::shared_ptr<const Geometry> CsgOpNode::dragPoint(const Vector3d& pt, const Vector3d& newpt,
                                                     DragResult& result)
{
  std::shared_ptr<PolySet> result_geom = std::make_shared<PolySet>(3);
  for (auto& child : children) {
    std::shared_ptr<const Geometry> child_geom = child->dragPoint(pt, newpt, result);
    if (child_geom == nullptr) continue;
    std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(child_geom);
    if (ps == nullptr) continue;
    for (const auto& v : ps->vertices)  // just put all vertices alltogether
      result_geom->vertices.push_back(v);
  }
  return result_geom;
}
std::string CsgOpNode::name() const
{
  switch (this->type) {
  case OpenSCADOperator::UNION:        return "union"; break;
  case OpenSCADOperator::DIFFERENCE:   return "difference"; break;
  case OpenSCADOperator::INTERSECTION: return "intersection"; break;
  default:                             assert(false);
  }
  return "internal_error";
}

void register_builtin_csgops()
{
  Builtins::init("union", new BuiltinModule(builtin_union),
                 {
                   "union()",
                 });

  Builtins::init("difference", new BuiltinModule(builtin_difference),
                 {
                   "difference()",
                 });

  Builtins::init("intersection", new BuiltinModule(builtin_intersection),
                 {
                   "intersection()",
                 });
}
