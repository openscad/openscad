// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include <sstream>

#include "module.h"
#include "ModuleInstantiation.h"
#include "Builtins.h"
#include "Parameters.h"
#include "Children.h"
#include "RoofNode.h"

#include <Python.h>
#include "pyopenscad.h"

static std::shared_ptr<AbstractNode> builtin_roof(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<RoofNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"method"},
                                            {"convexity"}
                                            );

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  node->fa = std::max(node->fa, 0.01);
  node->fs = std::max(node->fs, 0.01);
  if (node->fn > 0) {
    node->fa = 360.0 / node->fn;
    node->fs = 0.0;
  }

  if (parameters["method"].isUndefined()) {
    node->method = "voronoi";
  } else {
    node->method = parameters["method"].toString();
    // method can only be one of...
    if (node->method != "voronoi" && node->method != "straight") {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unknown roof method '" + node->method + "'. Using 'voronoi'.");
      node->method = "voronoi";
    }
  }

  double tmp_convexity = 0.0;
  parameters["convexity"].getFiniteDouble(tmp_convexity);
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  children.instantiate(node);

  return node;
}


PyObject* python_roof(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<RoofNode>(&todo_fix_inst);
  double fn=-1,fa=-1,fs=-1;

  char * kwlist[] ={"obj","method","convexity","fn","fa","fs",NULL};
  PyObject *obj = NULL;
  const char *method = NULL;
  int convexity=2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|sdddd", kwlist, 
                          &PyOpenSCADType, &obj,
			  &method, convexity,
			  &fn,&fa,&fs
                          )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
        return NULL;
  }
  child = PyOpenSCADObjectToNode(obj);

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

  node->fa = std::max(node->fa, 0.01);
  node->fs = std::max(node->fs, 0.01);
  if (node->fn > 0) {
    node->fa = 360.0 / node->fn;
    node->fs = 0.0;
  }

  if (method == NULL) {
    node->method = "voronoi";
  } else {
    node->method = method;
    // method can only be one of...
    if (node->method != "voronoi" && node->method != "straight") {
//      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
//          "Unknown roof method '" + node->method + "'. Using 'voronoi'.");
      node->method = "voronoi";
    }
  }

  double tmp_convexity = convexity;
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_roof_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
        PyObject *new_args=python_oo_args(self,args);
        PyObject *result = python_roof(self,new_args,kwargs);
//      Py_DECREF(&new_args);
        return result;
}

std::string RoofNode::toString() const
{
  std::stringstream stream;

  stream << "roof(method = \"" << this->method << "\""
         << ", $fa = " << this->fa
         << ", $fs = " << this->fs
         << ", $fn = " << this->fn
         << ", convexity = " << this->convexity
         << ")";

  return stream.str();
}

void register_builtin_roof()
{
  Builtins::init("roof", new BuiltinModule(builtin_roof, &Feature::ExperimentalRoof), {
    "roof(method = \"voronoi\")"
  });
}
