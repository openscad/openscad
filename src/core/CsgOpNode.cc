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

#include "CsgOpNode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "Builtins.h"
#include "Children.h"
#include "Parameters.h"
#include <sstream>
#include <cassert>
#include <Python.h>
#include <pyopenscad.h>

static std::shared_ptr<AbstractNode> builtin_union(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::UNION));
}

static std::shared_ptr<AbstractNode> builtin_difference(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::DIFFERENCE));
}

static std::shared_ptr<AbstractNode> builtin_intersection(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  return children.instantiate(std::make_shared<CsgOpNode>(inst, OpenSCADOperator::INTERSECTION));
}

PyObject* python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs,OpenSCADOperator mode)
{
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CsgOpNode>(&todo_fix_inst, mode);
  char *kwlist[]= { "obj",NULL };
  PyObject *objs = NULL;
  PyObject *obj;

  if (! PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, 
			  &PyList_Type,&objs
			  )) { 
    PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
    return NULL; 
  }
  n = PyList_Size(objs);
  for(i=0;i<n;i++) {
	obj = PyList_GetItem(objs,i);
	child = PyOpenSCADObjectToNode(obj);
        node->children.push_back(child);
  }
  
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::UNION);
}

PyObject* python_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject* python_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::INTERSECTION);
}

std::string CsgOpNode::toString() const
{
  return this->name() + "()";
}

std::string CsgOpNode::name() const
{
  switch (this->type) {
  case OpenSCADOperator::UNION:
    return "union";
    break;
  case OpenSCADOperator::DIFFERENCE:
    return "difference";
    break;
  case OpenSCADOperator::INTERSECTION:
    return "intersection";
    break;
  default:
    assert(false);
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

