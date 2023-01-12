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

#include "OffsetNode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "Children.h"
#include "Parameters.h"
#include "Builtins.h"
#include "Python.h"
#include "pyopenscad.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

static std::shared_ptr<AbstractNode> builtin_offset(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<OffsetNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"delta", "chamfer"});

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  // default with no argument at all is (r = 1, chamfer = false)
  // radius takes precedence if both r and delta are given.
  node->delta = 1;
  node->chamfer = false;
  node->join_type = ClipperLib::jtRound;
  if (parameters["r"].isDefinedAs(Value::Type::NUMBER)) {
    node->delta = parameters["r"].toDouble();
  } else if (parameters["delta"].isDefinedAs(Value::Type::NUMBER)) {
    node->delta = parameters["delta"].toDouble();
    node->join_type = ClipperLib::jtMiter;
    if (parameters["chamfer"].isDefinedAs(Value::Type::BOOL) && parameters["chamfer"].toBool()) {
      node->chamfer = true;
      node->join_type = ClipperLib::jtSquare;
    }
  }

  return children.instantiate(node);
}


PyObject* python_offset(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<OffsetNode>(&todo_fix_inst);

  char * kwlist[] ={"obj","r","delta","chamfer",NULL};
  PyObject *obj = NULL;
  double r=-1,delta=-1;
  const char *chamfer=NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!d|ds", kwlist, 
                          &PyOpenSCADType, &obj,
			  &r,&delta,&chamfer
                          ))
        return NULL;
  child = PyOpenSCADObjectToNode(obj);

  node->fn=10;
  node->fs=10;
  node->fa=10;

  node->delta = 1;
  node->chamfer = false;
  node->join_type = ClipperLib::jtRound;
  if (r != -1) {
    node->delta = r;
  } else if (delta != -1) {
    node->delta = delta;
    node->join_type = ClipperLib::jtMiter;
    if (chamfer != NULL && !strcasecmp(chamfer,"true"))  {
      node->chamfer = true;
      node->join_type = ClipperLib::jtSquare;
    }
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);   
}

PyObject* python_offset_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
        PyObject *new_args=python_oo_args(self,args);
        PyObject *result = python_offset(self,new_args,kwargs);
//      Py_DECREF(&new_args);
        return result;
}



std::string OffsetNode::toString() const
{
  std::ostringstream stream;

  bool isRadius = this->join_type == ClipperLib::jtRound;
  auto var = isRadius ? "(r = " : "(delta = ";

  stream << this->name() << var << std::dec << this->delta;
  if (!isRadius) {
    stream << ", chamfer = " << (this->chamfer ? "true" : "false");
  }
  stream << ", $fn = " << this->fn
         << ", $fa = " << this->fa
         << ", $fs = " << this->fs << ")";

  return stream.str();
}

void register_builtin_offset()
{
  Builtins::init("offset", new BuiltinModule(builtin_offset),
  {
    "offset(r = number)",
    "offset(delta = number)",
    "offset(r = number, chamfer = false)",
  });
}
