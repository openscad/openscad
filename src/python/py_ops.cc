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

#include "genlang/genlang.h"
#include <Python.h>
#include "pyopenscad.h"
#include <io/fileutils.h>
#include <handle_dep.h>
#include <PullNode.h>
#include <WrapNode.h>
#include <ColorNode.h>
#include <RoofNode.h>
#include <OversampleNode.h>
#include <DebugNode.h>
#include <FilletNode.h>
#include <RepairNode.h>
#include <SurfaceNode.h>
#include <RenderNode.h>
#include <ProjectionNode.h>
#include <OffsetNode.h>
#include <TransformNode.h>
#include <ColorUtil.h>
#include "pyfunctions.h"

// std::string lookup_file(const std::string& filename, const std::string& path,
//                         const std::string& fallbackpath);

PyObject *python_offset_core(PyObject *obj, double r, double delta, PyObject *chamfer,
                             CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<OffsetNode>(instance, discretizer);

  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in offset");
    return NULL;
  }

  node->delta = 1;
  node->chamfer = false;
  node->join_type = Clipper2Lib::JoinType::Round;
  if (!isnan(r)) {
    node->delta = r;
  } else if (!isnan(delta)) {
    node->delta = delta;
    node->join_type = Clipper2Lib::JoinType::Miter;
    if (chamfer == Py_True) {
      node->chamfer = true;
      node->join_type = Clipper2Lib::JoinType::Square;
    } else if (chamfer == Py_False || chamfer == NULL) node->chamfer = 0;
    else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for chamfer parameter");
      return NULL;
    }
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_offset(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "r", "delta", "chamfer", NULL};
  PyObject *obj = NULL;
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ddO", kwlist, &obj, &r, &delta, &chamfer)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r,delta)");
    return NULL;
  }
  return python_offset_core(obj, r, delta, chamfer, std::move(discretizer));
}

PyObject *python_oo_offset(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"r", "delta", "chamfer", NULL};
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddO", kwlist, &r, &delta, &chamfer)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r,delta)");
    return NULL;
  }
  return python_offset_core(obj, r, delta, chamfer, std::move(discretizer));
}

PyObject *python_pull_core(PyObject *obj, PyObject *anchor, PyObject *dir)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<PullNode>(instance);
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in translate\n");
    return NULL;
  }

  double x = 0, y = 0, z = 0;
  if (python_vectorval(anchor, 3, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in anchor\n");
    return NULL;
  }
  node->anchor = Vector3d(x, y, z);

  if (python_vectorval(dir, 3, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in dir\n");
    return NULL;
  }
  node->dir = Vector3d(x, y, z);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_pull(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "src", "dst", NULL};
  PyObject *obj = NULL;
  PyObject *anchor = NULL;
  PyObject *dir = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|", kwlist, &obj, &anchor, &dir)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_pull_core(obj, anchor, dir);
}

PyObject *python_oo_pull(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"src", "dst", NULL};
  PyObject *anchor = NULL;
  PyObject *dir = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|", kwlist, &anchor, &dir)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_pull_core(obj, anchor, dir);
}

PyObject *python_wrap_core(PyObject *obj, PyObject *target, double r, double d, double fn, double fa,
                           double fs)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<WrapNode>(instance);

  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in Wrap\n");
    return NULL;
  }

  if (!python_numberval(target, &node->r, nullptr, 0)) {
    node->shape = nullptr;
  } else if (target != nullptr &&
             PyObject_IsInstance(target, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    std::shared_ptr<AbstractNode> abstr = ((PyOpenSCADObject *)target)->node;
    node->shape = abstr;
  } else if (!isnan(r)) {
    node->r = r;
    node->shape = nullptr;
  } else if (!isnan(d)) {
    node->r = d / 2.0;
    node->shape = nullptr;
  } else {
    PyErr_SetString(PyExc_TypeError, "wrapping object must bei either Polygon or cylinder radius\n");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_wrap(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "target", "r", "d", "fn", "fa", "fs", NULL};
  PyObject *obj = NULL, *target = NULL;
  double fn, fa, fs;
  double r = NAN, d = NAN;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oddddd", kwlist, &obj, &target, &r, &d, &fn, &fa,
                                   &fs)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing wrap\n");
    return NULL;
  }
  return python_wrap_core(obj, target, r, d, fn, fa, fs);
}

PyObject *python_oo_wrap(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"target", "r", "d", "fn", "fa", "fs", NULL};
  double fn = NAN, fa = NAN, fs = NAN;
  PyObject *target = NULL;
  double r = NAN, d = NAN;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oddddd", kwlist, &target, &r, &d, &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing wrap\n");
    return NULL;
  }
  std::vector<double> xsteps;
  xsteps.push_back(4);
  xsteps.push_back(6);

  return python_wrap_core(obj, target, r, d, fn, fa, fs);
}

PyObject *python_color_core(PyObject *obj, PyObject *color, double alpha)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in color");
    return NULL;
  }
  DECLARE_INSTANCE();
  auto node = std::make_shared<ColorNode>(instance);

  Vector4d col(0, 0, 0, alpha);
  if (!python_vectorval(color, 3, 4, &col[0], &col[1], &col[2], &col[3])) {
    node->color.setRgba(float(col[0]), float(col[1]), float(col[2]), float(col[3]));
  } else if (PyUnicode_Check(color)) {
    PyObject *value = PyUnicode_AsEncodedString(color, "utf-8", "~");
    char *colorname = PyBytes_AS_STRING(value);
    const auto parsed_color = OpenSCAD::parse_color(colorname);
    if (parsed_color) {
      node->color = *parsed_color;
      if (1.0 != alpha) node->color.setAlpha(alpha);
    } else {
      PyErr_SetString(PyExc_TypeError, "Cannot parse color");
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown color representation");
    return nullptr;
  }

  node->children.push_back(child);

  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_color(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "c", "alpha", NULL};
  PyObject *obj = NULL;
  PyObject *color = NULL;
  double alpha = 1.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Odi", kwlist, &obj, &color, &alpha)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha);
}

PyObject *python_oo_color(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"c", "alpha", NULL};
  PyObject *color = NULL;
  double alpha = 1.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Od", kwlist, &color, &alpha)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha);
}

PyObject *python_oversample_core(PyObject *obj, double size, const char *texture, const char *projection,
                                 double texturewidth, double textureheight, double texturedepth)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in oversample \n");
    return NULL;
  }

  DECLARE_INSTANCE();
  auto node = std::make_shared<OversampleNode>(instance);
  node->children.push_back(child);

  node->size = size;

  auto filename = lookup_file(texture == NULL ? "" : texture, python_scriptpath.parent_path().u8string(),
                              instance->location().filePath().parent_path().string());

  node->texturefilename = filename;
  node->textureprojection = PROJECTION_NONE;
  if (projection != nullptr) {
    node->textureprojection = -1;
    for (int i = 0; i < TEXTUREPROJECTION_NUM; i++) {
      if (strcmp(projection, projectionNames[i]) == 0) node->textureprojection = i;
    }
    if (node->textureprojection == -1) {
      node->textureprojection = PROJECTION_NONE;
      PyErr_SetString(PyExc_TypeError, "Unsupported texture projection\n");
      return NULL;
    }
  }
  node->texturewidth = texturewidth;
  node->textureheight = textureheight;
  node->texturedepth = texturedepth;

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_oversample(PyObject *self, PyObject *args, PyObject *kwargs)
{
  double size = 2;
  char *kwlist[] = {"obj",          "size",          "texture",      "projection",
                    "texturewidth", "textureheight", "texturedepth", NULL};
  PyObject *obj = NULL;
  const char *texture = nullptr;
  const char *projection = nullptr;
  double texture_width = 1;
  double texture_height = 1;
  double texture_depth = 1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od|ssddd", kwlist, &obj, &size, &texture, &projection,
                                   &texture_width, &texture_height, &texture_depth)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_oversample_core(obj, size, texture, projection, texture_width, texture_height,
                                texture_depth);
}

PyObject *python_oo_oversample(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double size = 2;
  char *kwlist[] = {"size",          "texture",      "projection", "texturewidth",
                    "textureheight", "texturedepth", NULL};
  const char *texture = nullptr;
  const char *projection = nullptr;
  double texture_width = 1;
  double texture_height = 1;
  double texture_depth = 1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|ssddd", kwlist, &size, &texture, &projection,
                                   &texture_width, &texture_height, &texture_depth)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_oversample_core(obj, size, texture, projection, texture_width, texture_height,
                                texture_depth);
}

PyObject *python_debug_core(PyObject *obj, PyObject *faces)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in debug \n");
    return NULL;
  }

  DECLARE_INSTANCE();
  auto node = std::make_shared<DebugNode>(instance);
  node->children.push_back(child);
  if (faces != nullptr) {
    std::vector<int> intfaces = python_intlistval(faces);
    node->faces = intfaces;
  }
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_debug(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "faces", NULL};
  PyObject *obj = NULL;
  PyObject *faces = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &faces)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_debug_core(obj, faces);
}

PyObject *python_oo_debug(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"faces", NULL};
  PyObject *faces = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &faces)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_debug_core(self, faces);
}

PyObject *python_repair_core(PyObject *obj, PyObject *color)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in repair \n");
    return NULL;
  }

  DECLARE_INSTANCE();
  auto node = std::make_shared<RepairNode>(instance);
  node->children.push_back(child);
  if (color != nullptr) {
    Vector4d col(0, 0, 0, 1.0);
    if (!python_vectorval(color, 3, 4, &col[0], &col[1], &col[2], &col[3])) {
      node->color.setRgba(float(col[0]), float(col[1]), float(col[2]), float(col[3]));
    } else if (PyUnicode_Check(color)) {
      PyObject *value = PyUnicode_AsEncodedString(color, "utf-8", "~");
      char *colorname = PyBytes_AS_STRING(value);
      const auto parsed_color = OpenSCAD::parse_color(colorname);
      if (parsed_color) {
        node->color = *parsed_color;
        node->color.setAlpha(1.0);
      } else {
        PyErr_SetString(PyExc_TypeError, "Cannot parse color");
        return NULL;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Unknown color representation");
      return nullptr;
    }
  }
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_repair(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "color", NULL};
  PyObject *obj = NULL;
  PyObject *color = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &color)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_repair_core(obj, color);
}

PyObject *python_oo_repair(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"color", NULL};
  PyObject *color = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &color)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_repair_core(self, color);
}

PyObject *python_fillet_core(PyObject *obj, double r, int fn, PyObject *sel, double minang)
{
  PyObject *dummydict;
  DECLARE_INSTANCE();
  auto node = std::make_shared<FilletNode>(instance);
  PyTypeObject *type = &PyOpenSCADType;
  node->r = r;
  node->fn = fn;
  node->minang = minang;
  if (obj != nullptr) {
    type = PyOpenSCADObjectType(obj);
    node->children.push_back(PyOpenSCADObjectToNodeMulti(obj, &dummydict));
  } else {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in fillet \n");
    return NULL;
  }

  if (sel != nullptr) {
    type = PyOpenSCADObjectType(sel);
    auto child = PyOpenSCADObjectToNodeMulti(sel, &dummydict);
    if (child != nullptr) node->children.push_back(child);
  }

  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_fillet(PyObject *self, PyObject *args, PyObject *kwargs)
{
  double r = 1.0;
  double fn = 3;
  double minang = 30;
  char *kwlist[] = {"obj", "r", "sel", "fn", "minang", NULL};
  PyObject *obj = NULL;
  PyObject *sel = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od|Odd", kwlist, &obj, &r, &sel, &fn, &minang)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_fillet_core(obj, r, fn, sel, minang);
}

PyObject *python_oo_fillet(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double r = 1.0;
  double fn = NAN;
  double minang = 30;
  PyObject *sel = nullptr;
  char *kwlist[] = {"r", "sel", "fn", "minang", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|Odd", kwlist, &r, &sel, &fn, &minang)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_fillet_core(obj, r, fn, sel, minang);
}

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
PyObject *python_roof_core(PyObject *obj, const char *method, int convexity,
                           CurveDiscretizer&& discretizer)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RoofNode>(instance, discretizer);
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in roof");
    return NULL;
  }

  if (method == NULL) {
    node->method = "voronoi";
  } else {
    node->method = method;
    if (!RoofNode::knownMethods.count(node->method)) {
      //      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
      //          "Unknown roof method '" + node->method + "'. Using 'voronoi'.");
      node->method = "voronoi";
    }
  }

  double tmp_convexity = convexity;
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_roof(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "method", "convexity", NULL};
  PyObject *obj = NULL;
  const char *method = NULL;
  int convexity = 2;
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sd", kwlist, &obj, &method, convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, std::move(discretizer));
}

PyObject *python_oo_roof(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"method", "convexity", NULL};
  const char *method = NULL;
  int convexity = 2;
  auto discretizer = CreateCurveDiscretizer(kwargs);
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sd", kwlist, &method, convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, std::move(discretizer));
}
#endif

PyObject *python_render_core(PyObject *obj, int convexity)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<RenderNode>(instance);

  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &dummydict);
  node->convexity = convexity;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_render(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "convexity", NULL};
  PyObject *obj = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist, &PyOpenSCADType, &obj, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_oo_render(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"convexity", NULL};
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_surface_core(const char *file, PyObject *center, PyObject *invert, PyObject *color,
                              int convexity)
{
  DECLARE_INSTANCE();

  auto node = std::make_shared<SurfaceNode>(instance);

  std::string fileval = file == NULL ? "" : file;

  std::string filename = lookup_file(fileval, python_scriptpath.parent_path().u8string(),
                                     instance->location().filePath().parent_path().string());
  node->filename = filename;
  handle_dep(fs::path(filename).generic_string());

  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
    return NULL;
  }

  if (color == Py_True) node->color = 1;
  else if (color == Py_False || color == NULL) node->color = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for color parameter");
    return NULL;
  }

  node->convexity = 2;
  if (invert == Py_True) node->invert = 1;
  else if (center == Py_False || center == NULL) node->center = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "Unknown Value for invert parameter");
    return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_surface(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"file", "center", "convexity", "invert", "color", NULL};
  const char *file = NULL;
  PyObject *center = NULL;
  PyObject *invert = NULL;
  PyObject *color = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OlOO", kwlist, &file, &center, &convexity, &invert,
                                   &color)) {
    PyErr_SetString(PyExc_TypeError,
                    "Error during parsing surface(file, center, convexity, invert, color)");
    return NULL;
  }

  return python_surface_core(file, center, invert, color, convexity);
}

PyObject *python_projection_core(PyObject *obj, PyObject *cut, int convexity)
{
  DECLARE_INSTANCE();
  auto node = std::make_shared<ProjectionNode>(instance);
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in projection");
    return NULL;
  }
  node->convexity = convexity;
  node->cut_mode = 0;
  if (cut == Py_True) node->cut_mode = 1;
  else if (cut == Py_False) node->cut_mode = 0;
  else {
    PyErr_SetString(PyExc_TypeError, "cut can be either True or false");
    return NULL;
  }

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_projection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "cut", "convexity", NULL};
  PyObject *obj = NULL;
  PyObject *cutmode = Py_False;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Ol", kwlist, &obj, &cutmode, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_oo_projection(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"cut", "convexity", NULL};
  PyObject *cutmode = Py_False;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Ol", kwlist, &cutmode, &convexity)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_group(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<GroupNode>(instance);

  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  PyObject *dummydict;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyOpenSCADType, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNode(obj, &dummydict);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);
}

PyObject *python_align_core(PyObject *obj, PyObject *pyrefmat, PyObject *pydstmat, PyObject *flip)
{
  if (!PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(&PyOpenSCADType))) {
    PyErr_SetString(PyExc_TypeError, "Must specify Object as 1st parameter");
    return nullptr;
  }
  PyObject *child_dict = nullptr;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> dstnode = PyOpenSCADObjectToNode(obj, &child_dict);
  if (dstnode == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Invalid align object");
    Py_RETURN_NONE;
  }
  DECLARE_INSTANCE();
  auto multmatnode = std::make_shared<TransformNode>(instance, "align");
  multmatnode->children.push_back(dstnode);
  Matrix4d mat;
  Matrix4d MT = Matrix4d::Identity();

  if (!python_tomatrix(pyrefmat, mat)) MT = MT * mat;
  if (!python_tomatrix(pydstmat, mat)) {
    if (flip == Py_True) {
      for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) mat(j, i) = -mat(j, i);
    }
    MT = MT * mat.inverse();
  }

  multmatnode->matrix = MT;
  multmatnode->setPyName(dstnode->getPyName());

  PyObject *pyresult = PyOpenSCADObjectFromNode(type, multmatnode);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      //       PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      //       const char *value_str =  PyBytes_AS_STRING(value1);
      if (!python_tomatrix(value, mat)) {
        mat = MT * mat;
        PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, python_frommatrix(mat));
      } else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_align(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "refmat", "objmat", "flip", NULL};
  PyObject *obj = NULL;
  PyObject *pyrefmat = NULL;
  PyObject *pyobjmat = NULL;
  PyObject *flip = Py_False;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|OO", kwlist, &obj, &pyrefmat, &pyobjmat, &flip)) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat, flip);
}

PyObject *python_oo_align(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"refmat", "objmat", "flip", NULL};
  PyObject *pyrefmat = NULL;
  PyObject *pyobjmat = NULL;
  PyObject *flip = Py_False;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO", kwlist, &pyrefmat, &pyobjmat, &flip)) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat, flip);
}

PyObject *python_oo_clone(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *dict;
  PyObject *obj = NULL;
  char *kwlist[] = {"obj", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during clone");
    return NULL;
  }
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNodeMulti(obj, &dict);
  if (node.use_count() > 1) ((PyOpenSCADObject *)self)->node = node->clone();
  else ((PyOpenSCADObject *)self)->node = node;

  if (dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
      PyDict_SetItem(((PyOpenSCADObject *)self)->dict, key, value);
    }
  }
  Py_RETURN_NONE;
}

PyObject *python_debug_modifier(PyObject *arg, int mode)
{
  DECLARE_INSTANCE();
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(arg);
  auto child = PyOpenSCADObjectToNode(arg, &dummydict);
  switch (mode) {
  case 0: instance->tag_highlight = true; break;   // #
  case 1: instance->tag_background = true; break;  // %
  case 2: instance->tag_root = true; break;        // !
  }
  auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(type, node);  // TODO 1st loswerden
}

PyObject *python_debug_modifier_func(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyOpenSCADType, &obj)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  return python_debug_modifier(obj, mode);
}

PyObject *python_debug_modifier_func_oo(PyObject *obj, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  return python_debug_modifier(obj, mode);
}

PyObject *python_highlight(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func(self, args, kwargs, 0);
}

PyObject *python_oo_highlight(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func_oo(self, args, kwargs, 0);
}

PyObject *python_background(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func(self, args, kwargs, 1);
}

PyObject *python_oo_background(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func_oo(self, args, kwargs, 1);
}

PyObject *python_only(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func(self, args, kwargs, 2);
}

PyObject *python_oo_only(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_debug_modifier_func_oo(self, args, kwargs, 2);
}

PyObject *python_nb_invert(PyObject *arg)
{
  return python_debug_modifier(arg, 2);
}

PyObject *python_nb_neg(PyObject *arg)
{
  return python_debug_modifier(arg, 1);
}

PyObject *python_nb_pos(PyObject *arg)
{
  return python_debug_modifier(arg, 0);
}
