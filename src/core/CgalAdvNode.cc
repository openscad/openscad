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

#include "CgalAdvNode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Builtins.h"
#include "Children.h"
#include "Parameters.h"
#include <sstream>
#include <cassert>
#include <boost/assign/std/vector.hpp>
#include <Python.h>
#include "pyopenscad.h"

using namespace boost::assign; // bring 'operator+=()' into scope

PyObject* python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs,CgalAdvType mode)
{
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CgalAdvNode>(&todo_fix_inst, mode);
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

static std::shared_ptr<AbstractNode> builtin_minkowski(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CgalAdvNode>(inst, CgalAdvType::MINKOWSKI);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"convexity"});
  node->convexity = static_cast<int>(parameters["convexity"].toDouble());

  return children.instantiate(node);
}

PyObject* python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  int convexity=2;

  auto node = std::make_shared<CgalAdvNode>(&todo_fix_inst, CgalAdvType::MINKOWSKI);
  char *kwlist[]= { "obj","convexity",NULL };
  PyObject *objs = NULL;
  PyObject *obj;

  if (! PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist, 
			  &PyList_Type,&objs,
			  &convexity
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
  node->convexity = convexity;
  
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

static std::shared_ptr<AbstractNode> builtin_hull(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CgalAdvNode>(inst, CgalAdvType::HULL);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});
  node->convexity = 0;

  return children.instantiate(node);
}

PyObject* python_hull(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_adv_sub(self,args,kwargs,CgalAdvType::HULL);
}

static std::shared_ptr<AbstractNode> builtin_fill(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CgalAdvNode>(inst, CgalAdvType::FILL);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {});

  return children.instantiate(node);
}

PyObject* python_fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_adv_sub(self,args,kwargs,CgalAdvType::FILL);
}

static std::shared_ptr<AbstractNode> builtin_resize(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CgalAdvNode>(inst, CgalAdvType::RESIZE);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"newsize", "auto", "convexity"});
  node->convexity = static_cast<int>(parameters["convexity"].toDouble());
  node->newsize << 0, 0, 0;
  if (parameters["newsize"].type() == Value::Type::VECTOR) {
    const auto& vs = parameters["newsize"].toVector();
    if (vs.size() >= 1) node->newsize[0] = vs[0].toDouble();
    if (vs.size() >= 2) node->newsize[1] = vs[1].toDouble();
    if (vs.size() >= 3) node->newsize[2] = vs[2].toDouble();
  }
  const auto& autosize = parameters["auto"];
  node->autosize << false, false, false;
  if (autosize.type() == Value::Type::VECTOR) {
    const auto& va = autosize.toVector();
    if (va.size() >= 1) node->autosize[0] = va[0].toBool();
    if (va.size() >= 2) node->autosize[1] = va[1].toBool();
    if (va.size() >= 3) node->autosize[2] = va[2].toBool();
  } else if (autosize.type() == Value::Type::BOOL) {
    node->autosize << autosize.toBool(), autosize.toBool(), autosize.toBool();
  }

  return children.instantiate(node);
}

PyObject* python_resize(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  int convexity=2;

  auto node = std::make_shared<CgalAdvNode>(&todo_fix_inst, CgalAdvType::RESIZE);
  char *kwlist[]= { "obj","newsize","auto","convexity",NULL };
  PyObject *obj;
  PyObject *newsize=NULL;
  PyObject *autosize=NULL;

  if (! PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!O!i", kwlist, 
			  &PyOpenSCADType,&obj,
			  &PyList_Type,&newsize,
			  &PyList_Type,&autosize,
			  &convexity
			  )) {
    PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
    return NULL; 
  }
  child = PyOpenSCADObjectToNode(obj);

  if(newsize != NULL && PyList_Check(newsize)) {
	if(PyList_Size(newsize) >= 1) node->newsize[0] = PyFloat_AsDouble(PyList_GetItem(newsize, 0));
	if(PyList_Size(newsize) >= 2) node->newsize[1] = PyFloat_AsDouble(PyList_GetItem(newsize, 1));
	if(PyList_Size(newsize) >= 3) node->newsize[2] = PyFloat_AsDouble(PyList_GetItem(newsize, 2));
  }

  /* TODO what is that ?
  const auto& autosize = parameters["auto"];
  node->autosize << false, false, false;
  if (autosize.type() == Value::Type::VECTOR) {
    const auto& va = autosize.toVector();
    if (va.size() >= 1) node->autosize[0] = va[0].toBool();
    if (va.size() >= 2) node->autosize[1] = va[1].toBool();
    if (va.size() >= 3) node->autosize[2] = va[2].toBool();
  } else if (autosize.type() == Value::Type::BOOL) {
    node->autosize << autosize.toBool(), autosize.toBool(), autosize.toBool();
  }
  */

  node->children.push_back(child);
  node->convexity = convexity;
  
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

std::string CgalAdvNode::name() const
{
  switch (this->type) {
  case CgalAdvType::MINKOWSKI:
    return "minkowski";
    break;
  case CgalAdvType::HULL:
    return "hull";
    break;
  case CgalAdvType::FILL:
    return "fill";
    break;
  case CgalAdvType::RESIZE:
    return "resize";
    break;
  default:
    assert(false);
  }
  return "internal_error";
}

std::string CgalAdvNode::toString() const
{
  std::ostringstream stream;

  stream << this->name();
  switch (type) {
  case CgalAdvType::MINKOWSKI:
    stream << "(convexity = " << this->convexity << ")";
    break;
  case CgalAdvType::HULL:
  case CgalAdvType::FILL:
    stream << "()";
    break;
  case CgalAdvType::RESIZE:
    stream << "(newsize = ["
           << this->newsize[0] << "," << this->newsize[1] << "," << this->newsize[2] << "]"
           << ", auto = ["
           << this->autosize[0] << "," << this->autosize[1] << "," << this->autosize[2] << "]"
           << ", convexity = " << this->convexity
           << ")";
    break;
  default:
    assert(false);
  }

  return stream.str();
}

void register_builtin_cgaladv()
{
  Builtins::init("minkowski", new BuiltinModule(builtin_minkowski),
  {
    "minkowski(convexity = number)",
  });

  Builtins::init("hull", new BuiltinModule(builtin_hull),
  {
    "hull()",
  });

  Builtins::init("fill", new BuiltinModule(builtin_fill),
  {
    "fill()",
  });

  Builtins::init("resize", new BuiltinModule(builtin_resize),
  {
    "resize([x, y, z])",
    "resize([x, y, z], boolean)",
    "resize([x, y, z], [boolean, boolean, boolean])",
    "resize([x, y, z], [boolean, boolean, boolean], convexity = number)",
  });
}
