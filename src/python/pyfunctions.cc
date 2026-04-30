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

#include <Python.h>
#include "python/pyopenscad.h"
#include "pyfunctions.h"
#include "core/TransformNode.h"
#include "core/primitives.h"
#include "python/pyconversion.h"
PyObject *python__getsetitem_hier(std::shared_ptr<AbstractNode> node, const std::string& keystr,
                                  PyObject *v, int hier)
{
  PyObject *result = nullptr;

  if (keystr == "matrix") {
    std::shared_ptr<const TransformNode> trans = std::dynamic_pointer_cast<const TransformNode>(node);
    if (trans != nullptr) {
      Matrix4d matrix = Matrix4d::Identity();
      matrix = trans->matrix.matrix();
      result = python_frommatrix(matrix);
    }
  }

  if (keystr == "points") {
    std::shared_ptr<PolygonNode> polygon = std::dynamic_pointer_cast<PolygonNode>(node);
    if (polygon != nullptr) {
      if (v != nullptr) {
        polygon->points = python_to2dvarpointlist(v);
        Py_RETURN_NONE;
      }

      result = python_from2dvarpointlist(polygon->points);
    }
    std::shared_ptr<const PolyhedronNode> polyhedron =
      std::dynamic_pointer_cast<const PolyhedronNode>(node);
    if (polyhedron != nullptr) {
      result = python_from3dpointlist(polyhedron->points);
    }
  }

  if (keystr == "paths") {
    std::shared_ptr<PolygonNode> polygon = std::dynamic_pointer_cast<PolygonNode>(node);
    if (polygon != nullptr) {
      if (v != nullptr) {
        polygon->paths = python_to2dintlist(v);
        Py_RETURN_NONE;
      }

      result = python_from2dint(polygon->paths);
    }
  }

  if (keystr == "faces") {
    std::shared_ptr<const PolyhedronNode> polyhedron =
      std::dynamic_pointer_cast<const PolyhedronNode>(node);
    if (polyhedron != nullptr) {
      result = python_from2dlong(polyhedron->faces);
    }
  }
  if (result != nullptr) return result;

  if (hier > 0) {
    for (auto& child : node->children) {
      result = python__getsetitem_hier(child, keystr, v, hier - 1);
      if (result != nullptr) return result;
    }
  }
  return result;
}

PyObject *python__getitem__(PyObject *obj, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *)obj;
  PyObject *result;
  if (self->dict != nullptr) {
    // object dict
    result = PyDict_GetItem(self->dict, key);
    if (result != NULL) {
      Py_INCREF(result);
      return result;
    }
  }
  PyObject *keyname = PyUnicode_AsEncodedString(key, "utf-8", "~");
  if (keyname == nullptr) return nullptr;
  std::string keystr = PyBytes_AS_STRING(keyname);

  // member function lookup

  auto it = std::find(python_member_names.begin(), python_member_names.end(), keystr.c_str());
  if (it != python_member_names.end()) {
    int idx = (int)(it - python_member_names.begin());
    PyOpenSCADBoundMemberObject *bm =
      PyObject_New(PyOpenSCADBoundMemberObject, &PyOpenSCADBoundMemberType);
    if (!bm) return NULL;
    Py_INCREF(self);
    bm->scad_self = obj;
    bm->index = idx;
    return (PyObject *)bm;
  }

  PyObject *dummy_dict;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(obj, &dummy_dict);
  if (node != nullptr) {
    result = python__getsetitem_hier(node, keystr, nullptr, 0);
    if (result != nullptr) return result;
  }

  result = Py_None;
  if (keystr == "size") {
    return python_size_core(obj);
  } else if (keystr == "position") {
    return python_position_core(obj);
  } else if (keystr == "bbox") {
    return python_bbox_core(obj);
  }
  return result;
}

int python__setitem__(PyObject *obj, PyObject *key, PyObject *v)
{
  PyObject *keyname = PyUnicode_AsEncodedString(key, "utf-8", "~");
  if (keyname == nullptr) return 0;
  std::string keystr = PyBytes_AS_STRING(keyname);

  PyOpenSCADObject *self = (PyOpenSCADObject *)obj;

  PyObject *dummy_dict;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(obj, &dummy_dict);
  if (node != nullptr) {
    python__getsetitem_hier(node, keystr, v, 2);
  }

  if (self->dict == NULL) {
    return 0;
  }
  Py_INCREF(v);
  PyDict_SetItem(self->dict, key, v);
  return 0;
}

PyObject *python_osversion(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing version()");
    return NULL;
  }

  PyObject *version = PyList_New(3);
  PyList_SetItem(version, 0, PyFloat_FromDouble(OPENSCAD_YEAR));
  PyList_SetItem(version, 1, PyFloat_FromDouble(OPENSCAD_MONTH));
#ifdef OPENSCAD_DAY
  PyList_SetItem(version, 2, PyFloat_FromDouble(OPENSCAD_DAY));
#else
  PyList_SetItem(version, 2, PyFloat_FromDouble(0));
#endif

  return version;
}

PyObject *python_osversion_num(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing version_num()");
    return NULL;
  }

  double version = OPENSCAD_YEAR * 10000 + OPENSCAD_MONTH * 100;
#ifdef OPENSCAD_DAY
  version += OPENSCAD_DAY;
#endif
  return PyFloat_FromDouble(version);
}

PyObject *python_oo_hasattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *dict;
  char *keyword = NULL;
  char *kwlist[] = {"keyword", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &keyword)) {
    PyErr_SetString(PyExc_TypeError, "Error during hasattr");
    return NULL;
  }
  PyObject *pykeyword = PyUnicode_FromString(keyword);
  PyOpenSCADObjectToNodeMulti(self, &dict);
  if (PyDict_Contains(dict, pykeyword)) Py_RETURN_TRUE;
  else Py_RETURN_FALSE;
}

PyObject *python_oo_getattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *dict;
  char *keyword = NULL;
  char *kwlist[] = {"keyword", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &keyword)) {
    PyErr_SetString(PyExc_TypeError, "Error during getattr");
    return NULL;
  }
  PyObject *pykeyword = PyUnicode_FromString(keyword);
  PyOpenSCADObjectToNodeMulti(self, &dict);
  PyObject *prop = PyDict_GetItem(dict, pykeyword);
  Py_INCREF(prop);
  return prop;
}

PyObject *python_oo_setattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *dict;
  char *keyword = NULL;
  PyObject *setvalue;
  char *kwlist[] = {"keyword", "setvalue", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO", kwlist, &keyword, &setvalue)) {
    PyErr_SetString(PyExc_TypeError, "Error during setattr");
    return NULL;
  }
  PyObject *pykeyword = PyUnicode_FromString(keyword);
  PyOpenSCADObjectToNodeMulti(self, &dict);
  PyDict_SetItem(dict, pykeyword, setvalue);
  Py_RETURN_NONE;
}

PyObject *python_modelpath(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing model");
    return NULL;
  }
  return PyUnicode_FromString(python_scriptpath.u8string().c_str());
}

PyObject *python_oo_dict(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *dict = ((PyOpenSCADObject *)self)->dict;
  Py_INCREF(dict);
  return dict;
}

PyObject *python_oo__repr_mimebundle_(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *include = NULL;
  PyObject *exclude = NULL;

  char *kwlist[] = {"include", "exclude", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &include, &exclude)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing _repr_mimebundle_");
    return nullptr;
  }

  // jetzt dein Python viewer aufrufen
  PyObject *viewer_module = PyImport_ImportModule("libraries.python.jupyterdisplay");
  if (!viewer_module) {
    PyErr_SetString(PyExc_TypeError, "jupyterdisplay module not found");
    return nullptr;
  }

  PyObject *func = PyObject_GetAttrString(viewer_module, "build_widget");
  if (!func) {
    Py_DECREF(viewer_module);
    PyErr_SetString(PyExc_TypeError, "build_widget method not found");
    return nullptr;
  }

  PyObject *widget = PyObject_CallFunctionObjArgs(func, self, NULL);
  Py_DECREF(func);
  Py_DECREF(viewer_module);

  if (!widget) {
    PyErr_SetString(PyExc_TypeError, "error during execution of build_widget");
    return nullptr;
  }

  // jetzt den Formatter des Widgets verwenden
  PyObject *method = PyObject_GetAttrString(widget, "_repr_mimebundle_");
  if (!method) {
    Py_DECREF(widget);
    PyErr_SetString(PyExc_TypeError, "error during execution of repr");
    return nullptr;
  }

  PyObject *bundle = PyObject_Call(method, args, kwargs);

  Py_DECREF(method);
  Py_DECREF(widget);

  return bundle;
}

int PyDict_SetDefaultRef(PyObject *d, PyObject *key, PyObject *default_value, PyObject **result)
{
  PyDict_SetDefault(d, key, default_value);
  return 0;
}

int type_add_method(PyTypeObject *type, PyMethodDef *meth)  // from typeobject.c
{
  PyObject *descr;
  int isdescr = 1;
  if (meth->ml_flags & METH_CLASS) {
    if (meth->ml_flags & METH_STATIC) {
      PyErr_SetString(PyExc_ValueError, "method cannot be both class and static");
      return -1;
    }
    descr = PyDescr_NewClassMethod(type, meth);
  } else if (meth->ml_flags & METH_STATIC) {
    PyObject *cfunc = PyCFunction_NewEx(meth, (PyObject *)type, NULL);
    if (cfunc == NULL) {
      return -1;
    }
    descr = PyStaticMethod_New(cfunc);
    isdescr = 0;  // PyStaticMethod is not PyDescrObject
    Py_DECREF(cfunc);
  } else {
    descr = PyDescr_NewMethod(type, meth);
  }
  if (descr == NULL) {
    return -1;
  }

  PyObject *name;
  if (isdescr) {
    name = PyDescr_NAME(descr);
  } else {
    name = PyUnicode_FromString(meth->ml_name);
    if (name == NULL) {
      Py_DECREF(descr);
      return -1;
    }
  }

  int err;
  PyObject *dict = type->tp_dict;
  if (!(meth->ml_flags & METH_COEXIST)) {
    err = PyDict_SetDefaultRef(dict, name, descr, NULL) < 0;
  } else {
    err = PyDict_SetItem(dict, name, descr) < 0;
  }
  if (!isdescr) {
    Py_DECREF(name);
  }
  Py_DECREF(descr);
  if (err) {
    return -1;  // return here
  }
  return 0;
}

std::vector<PyObject *> python_member_callables;
std::vector<std::string> python_member_names;

PyObject *python_member_trampoline(PyObject *self, PyObject *args, PyObject *kwargs, int callind)
{
  int n = PyTuple_Size(args);
  PyObject *newargs = PyTuple_New(n + 1);
  PyTuple_SetItem(newargs, 0, self);
  for (int i = 0; i < n; i++) PyTuple_SetItem(newargs, i + 1, PyTuple_GetItem(args, i));
  return PyObject_Call(python_member_callables[callind], newargs, kwargs);
}

// --- PyOpenSCADBoundMember: een callable object dat self + index inbakt ---

static PyObject *PyOpenSCADBoundMemberCall(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyOpenSCADBoundMemberObject *bm = (PyOpenSCADBoundMemberObject *)self;
  return python_member_trampoline(bm->scad_self, args, kwargs, bm->index);
}

static void PyOpenSCADBoundMemberDealloc(PyObject *self)
{
  PyOpenSCADBoundMemberObject *bm = (PyOpenSCADBoundMemberObject *)self;
  Py_XDECREF(bm->scad_self);
  Py_TYPE(self)->tp_free(self);
}

PyTypeObject PyOpenSCADBoundMemberType = {
  PyVarObject_HEAD_INIT(NULL, 0) "PyOpenSADBoundMember",  // tp_name
  sizeof(PyOpenSCADBoundMemberObject),                    // tp_basicsize
  0,                                                      // tp_itemsize
  PyOpenSCADBoundMemberDealloc,                           // tp_dealloc
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  PyOpenSCADBoundMemberCall,  // tp_call
  0,
  0,
  0,
  0,
  Py_TPFLAGS_DEFAULT,  // tp_flags
};

PyObject *python_memberfunction(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"membername", "memberfunc", "docstring", NULL};
  char *membername = nullptr;
  PyObject *memberfunc = nullptr;
  char *memberdoc = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|s", kwlist, &membername, &memberfunc, &memberdoc)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing member");
    return NULL;
  }
  std::string member_name = membername;
  auto it = std::find(python_member_names.begin(), python_member_names.end(), member_name);

  Py_INCREF(memberfunc);  // needed because pythons garbage collector eats it when not used.
  if (it != python_member_names.end()) {
    int idx = (int)(it - python_member_names.begin());
    python_member_callables[idx] = memberfunc;
  } else {
    python_member_names.push_back(member_name);
    python_member_callables.push_back(memberfunc);
  }

  Py_RETURN_NONE;
}

std::shared_ptr<RenderVariables> renderVarsSet = nullptr;

PyMethodDef PyOpenSCADFunctions[] = {
  {"edge", (PyCFunction)python_edge, METH_VARARGS | METH_KEYWORDS, "Create Edge."},
  {"square", (PyCFunction)python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction)python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction)python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"polyline", (PyCFunction)python_polyline, METH_VARARGS | METH_KEYWORDS, "Create a Polyline."},
  {"spline", (PyCFunction)python_spline, METH_VARARGS | METH_KEYWORDS, "Create Spline."},
  {"text", (PyCFunction)python_text, METH_VARARGS | METH_KEYWORDS, "Create Text."},
  {"textmetrics", (PyCFunction)python_textmetrics, METH_VARARGS | METH_KEYWORDS, "Get textmetrics."},

  {"cube", (PyCFunction)python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction)python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction)python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction)python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},
#ifdef ENABLE_LIBFIVE
  {"frep", (PyCFunction)python_frep, METH_VARARGS | METH_KEYWORDS, "Create F-Rep."},
  {"ifrep", (PyCFunction)python_ifrep, METH_VARARGS | METH_KEYWORDS, "Create Inverse F-Rep."},
#endif

  {"translate", (PyCFunction)python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"right", (PyCFunction)python_right, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"left", (PyCFunction)python_left, METH_VARARGS | METH_KEYWORDS, "Move Left Object."},
  {"back", (PyCFunction)python_back, METH_VARARGS | METH_KEYWORDS, "Move Back Object."},
  {"front", (PyCFunction)python_front, METH_VARARGS | METH_KEYWORDS, "Move Front Object."},
  {"up", (PyCFunction)python_up, METH_VARARGS | METH_KEYWORDS, "Move Up Object."},
  {"down", (PyCFunction)python_down, METH_VARARGS | METH_KEYWORDS, "Move Down Object."},
  {"rotx", (PyCFunction)python_rotx, METH_VARARGS | METH_KEYWORDS, "Rotate X Object."},
  {"roty", (PyCFunction)python_roty, METH_VARARGS | METH_KEYWORDS, "Rotate Y Object."},
  {"rotz", (PyCFunction)python_rotz, METH_VARARGS | METH_KEYWORDS, "Rotate Z Object."},
  {"rotate", (PyCFunction)python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction)python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"mirror", (PyCFunction)python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction)python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"divmatrix", (PyCFunction)python_divmatrix, METH_VARARGS | METH_KEYWORDS, "Divmatrix Object."},
  {"offset", (PyCFunction)python_offset, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  {"roof", (PyCFunction)python_roof, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
#endif
  {"pull", (PyCFunction)python_pull, METH_VARARGS | METH_KEYWORDS, "Pull apart Object."},
  {"wrap", (PyCFunction)python_wrap, METH_VARARGS | METH_KEYWORDS, "Wrap Object around cylinder."},
  {"color", (PyCFunction)python_color, METH_VARARGS | METH_KEYWORDS, "Color Object."},
  {"output", (PyCFunction)python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"show", (PyCFunction)python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {"separate", (PyCFunction)python_separate, METH_VARARGS | METH_KEYWORDS, "Split into separate parts."},
  {"export", (PyCFunction)python_export, METH_VARARGS | METH_KEYWORDS, "Export the result."},

  {"linear_extrude", (PyCFunction)python_linear_extrude, METH_VARARGS | METH_KEYWORDS,
   "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction)python_rotate_extrude, METH_VARARGS | METH_KEYWORDS,
   "Rotate_extrude Object."},
  {"path_extrude", (PyCFunction)python_path_extrude, METH_VARARGS | METH_KEYWORDS,
   "Path_extrude Object."},
  {"skin", (PyCFunction)python_skin, METH_VARARGS | METH_KEYWORDS, "Path_extrude Object."},

  {"union", (PyCFunction)python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction)python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction)python_intersection, METH_VARARGS | METH_KEYWORDS,
   "Intersection Object."},
  {"hull", (PyCFunction)python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction)python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction)python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction)python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},
  {"concat", (PyCFunction)python_concat, METH_VARARGS | METH_KEYWORDS, "Concatenate Object."},

  {"highlight", (PyCFunction)python_highlight, METH_VARARGS | METH_KEYWORDS, "Highlight Object."},
  {"background", (PyCFunction)python_background, METH_VARARGS | METH_KEYWORDS, "Background Object."},
  {"only", (PyCFunction)python_only, METH_VARARGS | METH_KEYWORDS, "Only Object."},

  {"projection", (PyCFunction)python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction)python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},
  {"sheet", (PyCFunction)python_sheet, METH_VARARGS | METH_KEYWORDS, "Sheet Object."},
  {"mesh", (PyCFunction)python_mesh, METH_VARARGS | METH_KEYWORDS, "exports mesh."},
  {"inside", (PyCFunction)python_inside, METH_VARARGS | METH_KEYWORDS,
   "checks if a given point is inside"},
  {"bbox", (PyCFunction)python_bbox, METH_VARARGS | METH_KEYWORDS, "caluculate bbox of object."},
  {"size", (PyCFunction)python_size, METH_VARARGS | METH_KEYWORDS, "get size dimensions of object."},
  {"position", (PyCFunction)python_position, METH_VARARGS | METH_KEYWORDS,
   "get position (minimum coordinates) of object."},
  {"faces", (PyCFunction)python_faces, METH_VARARGS | METH_KEYWORDS, "exports a list of faces."},
  {"children", (PyCFunction)python_children, METH_VARARGS | METH_KEYWORDS, "create Tuple from children"},
  {"edges", (PyCFunction)python_edges, METH_VARARGS | METH_KEYWORDS,
   "exports a list of edges from a face."},
  {"explode", (PyCFunction)python_explode, METH_VARARGS | METH_KEYWORDS,
   "explode a solid with a vector"},
  {"oversample", (PyCFunction)python_oversample, METH_VARARGS | METH_KEYWORDS, "oversample."},
  {"debug", (PyCFunction)python_debug, METH_VARARGS | METH_KEYWORDS, "debug a face."},
  {"repair", (PyCFunction)python_repair, METH_VARARGS | METH_KEYWORDS, "Make solid watertight."},
  {"fillet", (PyCFunction)python_fillet, METH_VARARGS | METH_KEYWORDS, "fillet."},

  {"group", (PyCFunction)python_group, METH_VARARGS | METH_KEYWORDS, "Group Object."},
  {"render", (PyCFunction)python_render, METH_VARARGS | METH_KEYWORDS, "Render Object."},
  {"osimport", (PyCFunction)python_import, METH_VARARGS | METH_KEYWORDS, "Import Object."},
  {"osuse", (PyCFunction)python_osuse, METH_VARARGS | METH_KEYWORDS, "Use OpenSCAD Library."},
  {"osinclude", (PyCFunction)python_osinclude, METH_VARARGS | METH_KEYWORDS,
   "Include OpenSCAD Library."},
  {"version", (PyCFunction)python_osversion, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"version_num", (PyCFunction)python_osversion_num, METH_VARARGS | METH_KEYWORDS,
   "Output openscad Version."},
  {"add_parameter", (PyCFunction)python_add_parameter, METH_VARARGS | METH_KEYWORDS,
   "Add Parameter for Customizer."},
  {"scad", (PyCFunction)python_scad, METH_VARARGS | METH_KEYWORDS, "Source OpenSCAD code."},
  {"align", (PyCFunction)python_align, METH_VARARGS | METH_KEYWORDS, "Align Object to another."},
#ifndef OPENSCAD_NOGUI
  {"add_menuitem", (PyCFunction)python_add_menuitem, METH_VARARGS | METH_KEYWORDS,
   "Add Menuitem to the the openscad window."},
  {"nimport", (PyCFunction)python_nimport, METH_VARARGS | METH_KEYWORDS, "Import Networked Object."},
#endif
  {"model", (PyCFunction)python_model, METH_VARARGS | METH_KEYWORDS, "Yield Model"},
  {"modelpath", (PyCFunction)python_modelpath, METH_VARARGS | METH_KEYWORDS,
   "Returns absolute Path to script"},
  {"memberfunction", (PyCFunction)python_memberfunction, METH_VARARGS | METH_KEYWORDS,
   "Registers additional openscad memberfunction functions"},
  {"marked", (PyCFunction)python_marked, METH_VARARGS | METH_KEYWORDS, "Create a marked value."},
  {"Sin", (PyCFunction)python_sin, METH_VARARGS | METH_KEYWORDS, "Calculate sin."},
  {"Cos", (PyCFunction)python_cos, METH_VARARGS | METH_KEYWORDS, "Calculate cos."},
  {"Tan", (PyCFunction)python_tan, METH_VARARGS | METH_KEYWORDS, "Calculate tan."},
  {"Asin", (PyCFunction)python_asin, METH_VARARGS | METH_KEYWORDS, "Calculate asin."},
  {"Acos", (PyCFunction)python_acos, METH_VARARGS | METH_KEYWORDS, "Calculate acos."},
  {"Atan", (PyCFunction)python_atan, METH_VARARGS | METH_KEYWORDS, "Calculate atan."},
  {"norm", (PyCFunction)python_norm, METH_VARARGS | METH_KEYWORDS, "Calculate vector size."},
  {"dot", (PyCFunction)python_dot, METH_VARARGS | METH_KEYWORDS, "Calculate dot product."},
  {"cross", (PyCFunction)python_cross, METH_VARARGS | METH_KEYWORDS, "Calculate cross product."},
  {"machineconfig", (PyCFunction)python_machineconfig, METH_VARARGS | METH_KEYWORDS,
   "set Machineconfig"},
  {"rendervars", (PyCFunction)python_rendervars, METH_VARARGS | METH_KEYWORDS, "Set Rendervars"},
  {"vector", (PyCFunction)python_vector, METH_VARARGS | METH_KEYWORDS, "Create PythonSCAD Vector"},
  {NULL, NULL, 0, NULL}};

#define OO_METHOD_ENTRY(name, desc) \
  {#name, (PyCFunction)python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {
  OO_METHOD_ENTRY(translate, "Move Object") OO_METHOD_ENTRY(rotate, "Rotate Object") OO_METHOD_ENTRY(
    right, "Right Object") OO_METHOD_ENTRY(left, "Left Object") OO_METHOD_ENTRY(back, "Back Object")
    OO_METHOD_ENTRY(front, "Front Object") OO_METHOD_ENTRY(up, "Up Object") OO_METHOD_ENTRY(
      down, "Lower Object")

      OO_METHOD_ENTRY(union, "Union Object") OO_METHOD_ENTRY(difference, "Difference Object")
        OO_METHOD_ENTRY(intersection, "Intersection Object")

          OO_METHOD_ENTRY(rotx, "Rotx Object") OO_METHOD_ENTRY(roty, "Roty Object") OO_METHOD_ENTRY(
            rotz, "Rotz Object")

            OO_METHOD_ENTRY(scale, "Scale Object") OO_METHOD_ENTRY(mirror, "Mirror Object")
              OO_METHOD_ENTRY(multmatrix, "Multmatrix Object") OO_METHOD_ENTRY(
                divmatrix, "Divmatrix Object") OO_METHOD_ENTRY(offset, "Offset Object")
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
                OO_METHOD_ENTRY(roof, "Roof Object")
#endif
                  OO_METHOD_ENTRY(color, "Color Object") OO_METHOD_ENTRY(
                    separate, "Split into separate Objects") OO_METHOD_ENTRY(export, "Export Object")

                    OO_METHOD_ENTRY(linear_extrude, "Linear_extrude Object")
                      OO_METHOD_ENTRY(rotate_extrude, "Rotate_extrude Object") OO_METHOD_ENTRY(
                        path_extrude, "Path_extrude Object") OO_METHOD_ENTRY(resize, "Resize Object")

                        OO_METHOD_ENTRY(explode, "Explode a solid with a vector") OO_METHOD_ENTRY(
                          mesh, "Mesh Object") OO_METHOD_ENTRY(inside, "check if given point is inside")
                          OO_METHOD_ENTRY(bbox, "Evaluate Bound Box of object")
                            OO_METHOD_ENTRY(faces, "Create Faces list")
                              OO_METHOD_ENTRY(children, "Return Tupple from solid children")
                                OO_METHOD_ENTRY(edges, "Create Edges list") OO_METHOD_ENTRY(
                                  oversample, "Oversample Object") OO_METHOD_ENTRY(debug,
                                                                                   "Debug Object Faces")
                                  OO_METHOD_ENTRY(repair, "Make solid watertight") OO_METHOD_ENTRY(
                                    fillet, "Fillet Object") OO_METHOD_ENTRY(align,
                                                                             "Align Object to another")

                                    OO_METHOD_ENTRY(highlight, "Highlight Object")
                                      OO_METHOD_ENTRY(background, "Background Object") OO_METHOD_ENTRY(
                                        only, "Only Object") OO_METHOD_ENTRY(show, "Show Object")
                                        OO_METHOD_ENTRY(projection, "Projection Object")
                                          OO_METHOD_ENTRY(pull, "Pull Obejct apart") OO_METHOD_ENTRY(
                                            wrap, "Wrap Object around Cylinder")
                                            OO_METHOD_ENTRY(render, "Render Object")
                                              OO_METHOD_ENTRY(clone, "Clone Object") OO_METHOD_ENTRY(
                                                hasattr, "Check if an attribute exists")
                                                OO_METHOD_ENTRY(setattr, "Sets an attribute on a solid")
                                                  OO_METHOD_ENTRY(getattr,
                                                                  "Gets an attribute from a solid")
                                                    OO_METHOD_ENTRY(_repr_mimebundle_,
                                                                    "Jupyter display hook")
                                                      OO_METHOD_ENTRY(dict, "return all dictionary"){
                                                        NULL, NULL, 0, NULL}};

PyNumberMethods PyOpenSCADNumbers = {
  python_nb_add,        // binaryfunc nb_add
  python_nb_subtract,   // binaryfunc nb_subtract
  python_nb_mul,        // binaryfunc nb_multiply
  python_nb_remainder,  // binaryfunc nb_remainder
  0,                    // binaryfunc nb_divmod
  0,                    // ternaryfunc nb_power
  python_nb_neg,        // unaryfunc nb_negative
  python_nb_pos,        // unaryfunc nb_positive
  0,                    // unaryfunc nb_absolute
  0,                    // inquiry nb_bool
  python_nb_invert,     // unaryfunc nb_invert
  0,                    // binaryfunc nb_lshift
  0,                    // binaryfunc nb_rshift
  python_nb_and,        // binaryfunc nb_and
  python_nb_xor,        // binaryfunc nb_xor
  python_nb_or,         // binaryfunc nb_or
  0,                    // unaryfunc nb_int
  0,                    // void *nb_reserved
  0,                    // unaryfunc nb_float

  0,  // binaryfunc nb_inplace_add
  0,  // binaryfunc nb_inplace_subtract
  0,  // binaryfunc nb_inplace_multiply
  0,  // binaryfunc nb_inplace_remainder
  0,  // ternaryfunc nb_inplace_power
  0,  // binaryfunc nb_inplace_lshift
  0,  // binaryfunc nb_inplace_rshift
  0,  // binaryfunc nb_inplace_and
  0,  // binaryfunc nb_inplace_xor
  0,  // binaryfunc nb_inplace_or

  0,  // binaryfunc nb_floor_divide
  0,  // binaryfunc nb_true_divide
  0,  // binaryfunc nb_inplace_floor_divide
  0,  // binaryfunc nb_inplace_true_divide

  0,  // unaryfunc nb_index

  python_nb_matmult,  // binaryfunc nb_matrix_multiply
  0                   // binaryfunc nb_inplace_matrix_multiply
};

PyMappingMethods PyOpenSCADMapping = {0, python__getitem__, python__setitem__};
