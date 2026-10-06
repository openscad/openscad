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
#include "version.h"
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
        auto points = python_to2dvarpointlist(v);
        if (PyErr_Occurred()) return nullptr;
        polygon->points = std::move(points);
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
        auto paths = python_to2dintlist(v);
        if (PyErr_Occurred()) return nullptr;
        polygon->paths = std::move(paths);
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
  std::string keystr;
  if (!python_pyobject_to_utf8(key, keystr, "obj[key]")) {
    return nullptr;
  }

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

  PyObject *dummy_dict_raw = nullptr;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(obj, &dummy_dict_raw);
  auto dummy_dict = py_owned(dummy_dict_raw);
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
  } else if (keystr == "c") {
    return python_solid_root_color_rgba(obj);
  }
  return result;
}

int python__setitem__(PyObject *obj, PyObject *key, PyObject *v)
{
  // mp_ass_subscript convention: 0 == success, -1 == failure with
  // exception set. The helper raises a clean TypeError on non-str
  // keys (and on str keys that the strict utf-8 handler refuses to
  // encode, e.g. lone surrogates) and is leak-free on every path,
  // so we just propagate its result. Returning -1 with the helper's
  // exception set is the contract CPython expects for
  // ``c[non_str] = v`` -- without it the C-API surfaces a confusing
  // ``SystemError: <built-in function ...> returned a result with
  // an exception set`` from a wrong frame.
  std::string keystr;
  if (!python_pyobject_to_utf8(key, keystr, "obj[key]")) {
    return -1;
  }

  PyOpenSCADObject *self = (PyOpenSCADObject *)obj;

  PyObject *dummy_dict_raw = nullptr;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(obj, &dummy_dict_raw);
  auto dummy_dict = py_owned(dummy_dict_raw);
  // mp_ass_subscript reuses this slot for `del obj[key]`, in which
  // case CPython passes v == NULL. The hier setter only makes sense
  // for assignment, so skip it on deletion.
  if (node != nullptr && v != nullptr) {
    PyObject *hier_result = python__getsetitem_hier(node, keystr, v, 2);
    if (hier_result == nullptr && PyErr_Occurred()) return -1;
    if (hier_result != nullptr) {
      Py_DECREF(hier_result);
      return 0;
    }
  }

  if (self->dict == NULL) {
    return 0;
  }
  // Route the deletion case through PyDict_DelItem; without this guard
  // the Py_INCREF / PyDict_SetItem path below would crash on a NULL v.
  if (v == nullptr) {
    if (PyDict_DelItem(self->dict, key) < 0) return -1;
    return 0;
  }
  // PyDict_SetItem already increments the value's refcount on success
  // and leaves it untouched on failure, so no manual Py_INCREF is
  // required. The previous unconditional incref leaked one reference
  // per __setitem__ call (and also doubled the leak when SetItem
  // failed, since the dict never absorbed the extra ref).
  if (PyDict_SetItem(self->dict, key, v) < 0) return -1;
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
  PyList_SetItem(version, 0, PyLong_FromLong(OPENSCAD_MAJOR));
  PyList_SetItem(version, 1, PyLong_FromLong(OPENSCAD_MINOR));
  PyList_SetItem(version, 2, PyLong_FromLong(OPENSCAD_PATCH));

  return version;
}

PyObject *python_osversion_num(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing version_num()");
    return NULL;
  }

  long long version = OPENSCAD_MAJOR * 1000000LL + OPENSCAD_MINOR * 1000LL + OPENSCAD_PATCH;
  return PyLong_FromLongLong(version);
}

PyObject *python_osversion_string(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing version_string()");
    return NULL;
  }

  const auto version = std::string(openscad_versionnumber);
  return PyUnicode_FromString(version.c_str());
}

PyObject *python_oo_hasattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *keyword = NULL;
  char *kwlist[] = {"keyword", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &keyword)) {
    PyErr_SetString(PyExc_TypeError, "Error during hasattr");
    return NULL;
  }
  auto pykeyword = py_owned(PyUnicode_FromString(keyword));
  // PyUnicode_FromString already raised the relevant exception (typically
  // MemoryError or UnicodeDecodeError) on failure; just propagate it.
  if (pykeyword.get() == nullptr) return NULL;

  PyObject *dict_raw = nullptr;
  PyOpenSCADObjectToNodeMulti(self, &dict_raw);
  auto dict = py_owned(dict_raw);
  if (dict_raw == nullptr) {
    // PyOpenSCADObjectToNodeMulti can fail with a Python exception
    // already set (e.g. MemoryError from the dict-merge path); the
    // C-API contract forbids returning a value while an exception is
    // pending. Surface the error; only treat the no-dict case as
    // "not present" when no exception is in flight.
    if (PyErr_Occurred()) return NULL;
    Py_RETURN_FALSE;
  }

  // PyDict_Contains returns -1 with an exception set on error. Treating
  // that as truthy and returning Py_True would silently swallow the
  // error AND hand the caller a return value while an exception is
  // pending, which the C API contract forbids.
  int contains = PyDict_Contains(dict_raw, pykeyword.get());
  if (contains < 0) return NULL;
  if (contains) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

PyObject *python_oo_getattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *keyword = NULL;
  char *kwlist[] = {"keyword", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &keyword)) {
    PyErr_SetString(PyExc_TypeError, "Error during getattr");
    return NULL;
  }
  auto pykeyword = py_owned(PyUnicode_FromString(keyword));
  if (pykeyword.get() == nullptr) return NULL;

  PyObject *dict_raw = nullptr;
  PyOpenSCADObjectToNodeMulti(self, &dict_raw);
  auto dict = py_owned(dict_raw);
  if (dict_raw == nullptr) {
    // Same propagation rule as in python_oo_hasattr: a pending
    // exception from PyOpenSCADObjectToNodeMulti must surface
    // through to Python; only the non-exceptional "no dict for
    // this input" case should resolve to None.
    if (PyErr_Occurred()) return NULL;
    Py_RETURN_NONE;
  }

  // PyDict_GetItem returns NULL for "missing key" WITHOUT setting an
  // exception, so we cannot distinguish that from a hard error by the
  // return value alone. Returning NULL to Python in the missing-key
  // case is incorrect (Python would surface a SystemError because no
  // exception is set). Surface the missing-key case as Py_None to
  // match the most common "attribute lookup with default" idiom.
  PyObject *prop = PyDict_GetItem(dict_raw, pykeyword.get());
  if (prop == nullptr) Py_RETURN_NONE;
  Py_INCREF(prop);
  return prop;
}

PyObject *python_oo_setattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *keyword = NULL;
  PyObject *setvalue;
  char *kwlist[] = {"keyword", "setvalue", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO", kwlist, &keyword, &setvalue)) {
    PyErr_SetString(PyExc_TypeError, "Error during setattr");
    return NULL;
  }
  auto pykeyword = py_owned(PyUnicode_FromString(keyword));
  if (pykeyword.get() == nullptr) return NULL;

  PyObject *dict_raw = nullptr;
  PyOpenSCADObjectToNodeMulti(self, &dict_raw);
  auto dict = py_owned(dict_raw);
  if (dict_raw == nullptr) {
    // Same propagation rule as in python_oo_hasattr/getattr: a
    // pending exception from PyOpenSCADObjectToNodeMulti must
    // surface through to Python; only the non-exceptional "no
    // dict for this input" case should resolve to None.
    if (PyErr_Occurred()) return NULL;
    Py_RETURN_NONE;
  }

  // PyDict_SetItem returns -1 with an exception set on failure (e.g.
  // unhashable key, MemoryError on resize). Propagate that instead of
  // silently swallowing it.
  if (PyDict_SetItem(dict_raw, pykeyword.get(), setvalue) < 0) return NULL;
  Py_RETURN_NONE;
}

PyObject *python_modelpath(PyObject *self, PyObject *args, PyObject *kwargs)
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

// IPython display hook. Tries to build a `pythreejs` widget via the
// `jupyterdisplay` helper module (which lives at
// libraries/python/jupyterdisplay.py and is on sys.path because
// initPython prepends `libraries/python` to it; the importable name is
// therefore `jupyterdisplay`, NOT `libraries.python.jupyterdisplay`).
// If anything along the way fails -- helper module not found, optional
// dependencies (`numpy`, `pythreejs`, `ipywidgets`) missing, widget
// construction raised, ... -- we return `None` rather than raising.
//
// Returning `None` is IPython's documented `_repr_mimebundle_`
// "no rich representation available" sentinel: IPython then transparently
// falls through to `__repr__`. This matches what the user wants in a
// terminal `pythonscad --ipython` session, where `pythreejs` /
// `ipywidgets` / numpy are typically absent and a `cube(10)` echo
// previously crashed with `TypeError: jupyterdisplay module not found`
// (PR #600). In a real Jupyter notebook the same code path still yields
// the rich widget when the deps ARE present, because the import succeeds
// and we fall through to the widget's own `_repr_mimebundle_`.
//
// We `PyErr_Clear()` on every error path so the caller doesn't see a
// half-set exception on top of the `None` return, which would confuse
// IPython's display dispatcher.
PyObject *python_oo__repr_mimebundle_(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *include = NULL;
  PyObject *exclude = NULL;

  char *kwlist[] = {"include", "exclude", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &include, &exclude)) {
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  PyObject *viewer_module = PyImport_ImportModule("jupyterdisplay");
  if (!viewer_module) {
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  PyObject *func = PyObject_GetAttrString(viewer_module, "build_widget");
  if (!func) {
    Py_DECREF(viewer_module);
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  PyObject *widget = PyObject_CallFunctionObjArgs(func, self, NULL);
  Py_DECREF(func);
  Py_DECREF(viewer_module);

  if (!widget) {
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  PyObject *method = PyObject_GetAttrString(widget, "_repr_mimebundle_");
  if (!method) {
    Py_DECREF(widget);
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  PyObject *bundle = PyObject_Call(method, args, kwargs);

  Py_DECREF(method);
  Py_DECREF(widget);

  if (!bundle) {
    // The widget itself raised while formatting (e.g. transitive ipywidgets
    // call failed). Same contract: swallow the exception and let IPython
    // fall through to __repr__.
    PyErr_Clear();
    Py_RETURN_NONE;
  }

  return bundle;
}

#if PY_VERSION_HEX < 0x030D0000
// Fallback for CPython < 3.13 where this API does not exist. CPython >= 3.13
// ships the real implementation in libpython, so we MUST NOT redefine it: the
// dynamic linker resolves _asyncio.so's reference to the main executable's
// copy first, and a broken polyfill silently corrupts asyncio internals
// (e.g. enter_task() does Py_DECREF(*result) on the stack slot we never
// wrote, crashing on the first await).
int PyDict_SetDefaultRef(PyObject *d, PyObject *key, PyObject *default_value, PyObject **result)
{
  PyObject *existing = PyDict_GetItemWithError(d, key);
  if (existing != NULL) {
    if (result != NULL) {
      Py_INCREF(existing);
      *result = existing;
    }
    return 1;
  }
  if (PyErr_Occurred()) {
    if (result != NULL) *result = NULL;
    return -1;
  }
  if (PyDict_SetItem(d, key, default_value) < 0) {
    if (result != NULL) *result = NULL;
    return -1;
  }
  if (result != NULL) {
    Py_INCREF(default_value);
    *result = default_value;
  }
  return 0;
}
#endif

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
  {"edge", (PyCFunction)python_edge, METH_VARARGS | METH_KEYWORDS,
   "2D edge/line primitive.\n"
   "edge()\n"
   "edge(size=1)\n"
   "edge(size=1, center=True)"},
  {"square", (PyCFunction)python_square, METH_VARARGS | METH_KEYWORDS,
   "2D square or rectangle.\n"
   "square(dim)\n"
   "square([width, height])\n"
   "square([width, height], center=True)"},
  {"circle", (PyCFunction)python_circle, METH_VARARGS | METH_KEYWORDS,
   "2D circle.\n"
   "circle(r=radius)\n"
   "circle(d=diameter)\n"
   "circle(r=radius, angle=270)"},
  {"polygon", (PyCFunction)python_polygon, METH_VARARGS | METH_KEYWORDS,
   "2D polygon from points and paths.\n"
   "polygon(points=[...])\n"
   "polygon(points=[...], paths=[...])\n"
   "polygon(points=[...], paths=[...], convexity=2)\n"
   "Each point is [x, y] or [x, y, r] where r fillets the corner.\n"
   "Use fn/fa/fs to control fillet arc subdivision."},
  {"polyline", (PyCFunction)python_polyline, METH_VARARGS | METH_KEYWORDS,
   "Open 2D or 3D polyline through points.\n"
   "polyline(points=[...])\n"
   "Points are [x, y] (2D) or [x, y, z] (3D if any z != 0)."},
  {"spline", (PyCFunction)python_spline, METH_VARARGS | METH_KEYWORDS,
   "Smooth curve through points.\n"
   "spline(points=[...])\n"
   "spline(points=[...], fn=32)"},
  {"text", (PyCFunction)python_text, METH_VARARGS | METH_KEYWORDS,
   "2D text as outlines.\n"
   "text(\"Hello\")\n"
   "text(\"Hello\", size=10, font=\"Liberation Sans\")\n"
   "text(\"Hello\", halign=\"center\", valign=\"center\")"},
  {"textmetrics", (PyCFunction)python_textmetrics, METH_VARARGS | METH_KEYWORDS,
   "Measure text dimensions, no geometry created.\n"
   "textmetrics(\"Hello\")\n"
   "textmetrics(\"Hello\", size=10, font=\"Liberation Sans\")"},

  {"cube", (PyCFunction)python_cube, METH_VARARGS | METH_KEYWORDS,
   "3D box.\n"
   "cube(size)\n"
   "cube([width, depth, height])\n"
   "cube([width, depth, height], center=True)"},
  {"cylinder", (PyCFunction)python_cylinder, METH_VARARGS | METH_KEYWORDS,
   "3D cylinder or cone.\n"
   "cylinder(h, r1, r2)\n"
   "cylinder(h=height, r=radius, center=True)\n"
   "cylinder(h=height, r1=bottom, r2=top, center=True)\n"
   "cylinder(h=height, d=diameter, center=True)\n"
   "cylinder(h=height, d1=bottom, d2=top, center=True)"},
  {"sphere", (PyCFunction)python_sphere, METH_VARARGS | METH_KEYWORDS,
   "3D sphere.\n"
   "sphere(r=radius)\n"
   "sphere(d=diameter)"},
  {"polyhedron", (PyCFunction)python_polyhedron, METH_VARARGS | METH_KEYWORDS,
   "3D solid from points and faces.\n"
   "polyhedron(points=[...], faces=[...])\n"
   "polyhedron(points=[...], faces=[...], convexity=2)\n"
   "polyhedron(points=[...], faces=[...], colors=[...])"},
#ifdef ENABLE_LIBFIVE
  {"frep", (PyCFunction)python_frep, METH_VARARGS | METH_KEYWORDS,
   "Implicit surface (F-Rep) from an expression.\n"
   "frep(exp, min=[x, y, z], max=[x, y, z])\n"
   "frep(exp, min=[x, y, z], max=[x, y, z], res=0.1)"},
  {"ifrep", (PyCFunction)python_ifrep, METH_VARARGS | METH_KEYWORDS,
   "Convert a mesh back into an implicit surface.\n"
   "ifrep(obj)"},
#endif

  {"translate", (PyCFunction)python_translate, METH_VARARGS | METH_KEYWORDS,
   "Move object by a vector.\n"
   "translate(obj, v=[x, y, z])"},
  {"right", (PyCFunction)python_right, METH_VARARGS | METH_KEYWORDS,
   "Move object along +X.\n"
   "right(obj, distance)"},
  {"left", (PyCFunction)python_left, METH_VARARGS | METH_KEYWORDS,
   "Move object along -X.\n"
   "left(obj, distance)"},
  {"back", (PyCFunction)python_back, METH_VARARGS | METH_KEYWORDS,
   "Move object along +Y.\n"
   "back(obj, distance)"},
  {"front", (PyCFunction)python_front, METH_VARARGS | METH_KEYWORDS,
   "Move object along -Y.\n"
   "front(obj, distance)"},
  {"up", (PyCFunction)python_up, METH_VARARGS | METH_KEYWORDS,
   "Move object along +Z.\n"
   "up(obj, distance)"},
  {"down", (PyCFunction)python_down, METH_VARARGS | METH_KEYWORDS,
   "Move object along -Z.\n"
   "down(obj, distance)"},
  {"rotx", (PyCFunction)python_rotx, METH_VARARGS | METH_KEYWORDS,
   "Rotate object around the X axis.\n"
   "rotx(obj, angle)"},
  {"roty", (PyCFunction)python_roty, METH_VARARGS | METH_KEYWORDS,
   "Rotate object around the Y axis.\n"
   "roty(obj, angle)"},
  {"rotz", (PyCFunction)python_rotz, METH_VARARGS | METH_KEYWORDS,
   "Rotate object around the Z axis.\n"
   "rotz(obj, angle)"},
  {"rotate", (PyCFunction)python_rotate, METH_VARARGS | METH_KEYWORDS,
   "Rotate object by angle(s) or around an axis.\n"
   "rotate(obj, a=angle)\n"
   "rotate(obj, a=[x, y, z])\n"
   "rotate(obj, a=angle, v=[x, y, z])\n"
   "rotate(obj, a=angle, ref=[x, y, z])"},
  {"scale", (PyCFunction)python_scale, METH_VARARGS | METH_KEYWORDS,
   "Scale object uniformly or per axis.\n"
   "scale(obj, v=[x, y, z])\n"
   "scale(obj, v=factor)"},
  {"mirror", (PyCFunction)python_mirror, METH_VARARGS | METH_KEYWORDS,
   "Mirror object across a plane.\n"
   "mirror(obj, v=[x, y, z])"},
  {"multmatrix", (PyCFunction)python_multmatrix, METH_VARARGS | METH_KEYWORDS,
   "Apply a 4x4 transformation matrix.\n"
   "multmatrix(obj, m=matrix)"},
  {"divmatrix", (PyCFunction)python_divmatrix, METH_VARARGS | METH_KEYWORDS,
   "Apply the inverse of a 4x4 transformation matrix.\n"
   "divmatrix(obj, m=matrix)"},
  {"offset", (PyCFunction)python_offset, METH_VARARGS | METH_KEYWORDS,
   "Grow or shrink a 2D outline.\n"
   "offset(obj, r=radius)\n"
   "offset(obj, delta=distance)\n"
   "offset(obj, delta=distance, chamfer=True)"},
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  {"roof", (PyCFunction)python_roof, METH_VARARGS | METH_KEYWORDS,
   "Build a roof/hip shape over a 2D outline.\n"
   "roof(obj)\n"
   "roof(obj, method=\"voronoi\")\n"
   "roof(obj, method=\"straight\", convexity=2)"},
#endif
  {"pull", (PyCFunction)python_pull, METH_VARARGS | METH_KEYWORDS,
   "Stretch part of an object between two points.\n"
   "pull(obj, src=[x, y, z], dst=[x, y, z])"},
  {"wrap", (PyCFunction)python_wrap, METH_VARARGS | METH_KEYWORDS,
   "Wrap a flat object around a cylinder or a 2D outline.\n"
   "wrap(obj, target)\n"
   "wrap(obj, r=radius)\n"
   "wrap(obj, d=diameter, fn=64)"},
  {"color", (PyCFunction)python_color, METH_VARARGS | METH_KEYWORDS,
   "Set object color and transparency.\n"
   "color(obj, c=\"red\")\n"
   "color(obj, c=[r, g, b])\n"
   "color(obj, c=[r, g, b, a], alpha=0.5)"},
  {"output", (PyCFunction)python_output, METH_VARARGS | METH_KEYWORDS,
   "Deprecated alias for show().\n"
   "output(obj)  # deprecated, use show(obj)"},
  {"show", (PyCFunction)python_show, METH_VARARGS | METH_KEYWORDS,
   "Mark object(s) as render output.\n"
   "show(obj)"},
  {"separate", (PyCFunction)python_separate, METH_VARARGS | METH_KEYWORDS,
   "Split a compound object into its parts.\n"
   "separate(obj)"},
  {"export", (PyCFunction)python_export, METH_VARARGS | METH_KEYWORDS,
   "Write object to a file (STL, etc.). No-op during F5 preview.\n"
   "export(obj, file=\"out.stl\")"},

  {"linear_extrude", (PyCFunction)python_linear_extrude, METH_VARARGS | METH_KEYWORDS,
   "Extrude a 2D shape straight up.\n"
   "linear_extrude(obj, height=10)\n"
   "linear_extrude(obj, height=10, twist=90, slices=20)\n"
   "linear_extrude(obj, height=10, scale=0.5, center=True)"},
  {"rotate_extrude", (PyCFunction)python_rotate_extrude, METH_VARARGS | METH_KEYWORDS,
   "Extrude a 2D shape around an axis.\n"
   "rotate_extrude(obj)\n"
   "rotate_extrude(obj, angle=180)\n"
   "rotate_extrude(obj, angle=360, v=[0, 1, 0])"},
  {"path_extrude", (PyCFunction)python_path_extrude, METH_VARARGS | METH_KEYWORDS,
   "Extrude a 2D shape along a path.\n"
   "path_extrude(obj, path=[...])\n"
   "path_extrude(obj, path=[...], xdir=[1, 0, 0])\n"
   "path_extrude(obj, path=[...], twist=360, closed=True)"},
  {"skin", (PyCFunction)python_skin, METH_VARARGS | METH_KEYWORDS,
   "Loft a solid through a sequence of cross-sections.\n"
   "skin(obj1, obj2, ...)"},

  {"union", (PyCFunction)python_union, METH_VARARGS | METH_KEYWORDS,
   "Boolean union of objects.\n"
   "union(obj1, obj2, ...)"},
  {"difference", (PyCFunction)python_difference, METH_VARARGS | METH_KEYWORDS,
   "Boolean subtraction of objects.\n"
   "difference(obj1, obj2, ...)"},
  {"intersection", (PyCFunction)python_intersection, METH_VARARGS | METH_KEYWORDS,
   "Boolean intersection of objects.\n"
   "intersection(obj1, obj2, ...)"},
  {"hull", (PyCFunction)python_hull, METH_VARARGS | METH_KEYWORDS,
   "Convex hull around objects.\n"
   "hull(obj1, obj2, ...)"},
  {"minkowski", (PyCFunction)python_minkowski, METH_VARARGS | METH_KEYWORDS,
   "Minkowski sum of two objects.\n"
   "minkowski(obj1, obj2)\n"
   "minkowski(obj1, obj2, convexity=2)"},
  {"fill", (PyCFunction)python_fill, METH_VARARGS | METH_KEYWORDS,
   "Fill holes in a 2D shape.\n"
   "fill(obj)"},
  {"resize", (PyCFunction)python_resize, METH_VARARGS | METH_KEYWORDS,
   "Change object's bounding box size.\n"
   "resize(obj, newsize=[x, y, z])\n"
   "resize(obj, newsize=[x, y, z], auto=True)\n"
   "resize(obj, newsize=[x, y, z], convexity=2)"},
  {"concat", (PyCFunction)python_concat, METH_VARARGS | METH_KEYWORDS,
   "Combine objects without a boolean operation.\n"
   "concat(obj1, obj2, ...)"},

  {"highlight", (PyCFunction)python_highlight, METH_VARARGS | METH_KEYWORDS,
   "Mark object for highlighted (#) rendering.\n"
   "highlight(obj)"},
  {"background", (PyCFunction)python_background, METH_VARARGS | METH_KEYWORDS,
   "Mark object as background (%), excluded from render.\n"
   "background(obj)"},
  {"only", (PyCFunction)python_only, METH_VARARGS | METH_KEYWORDS,
   "Show only this object (!), hide siblings.\n"
   "only(obj)"},

  {"projection", (PyCFunction)python_projection, METH_VARARGS | METH_KEYWORDS,
   "Project a 3D object down to 2D.\n"
   "projection(obj)\n"
   "projection(obj, cut=True)"},
  {"surface", (PyCFunction)python_surface, METH_VARARGS | METH_KEYWORDS,
   "3D surface generated from a heightmap file.\n"
   "surface(file=\"heightmap.png\")\n"
   "surface(file=\"heightmap.dat\", center=True, invert=True)"},
  {"sheet", (PyCFunction)python_sheet, METH_VARARGS | METH_KEYWORDS,
   "Parametric surface from a function over a 2D grid.\n"
   "sheet(func, imin=0, imax=1, jmin=0, jmax=1)\n"
   "sheet(func, imin=0, imax=1, jmin=0, jmax=1, fs=0.5)\n"
   "sheet(func, imin=0, imax=1, jmin=0, jmax=1, iclose=True, jclose=True)"},
  {"patch", (PyCFunction)python_patch, METH_VARARGS | METH_KEYWORDS,
   "patch(outer, holes=None, proj=None, grid_spacing_uv=1.0, displacement=None, use_tangents=False)"},
  {"mesh", (PyCFunction)python_mesh, METH_VARARGS | METH_KEYWORDS,
   "Get triangle mesh vertices/faces of an object.\n"
   "mesh(obj)\n"
   "mesh(obj, triangulate=True)"},
  {"inside", (PyCFunction)python_inside, METH_VARARGS | METH_KEYWORDS,
   "Test whether a point lies inside an object.\n"
   "inside(obj, point=[x, y, z])"},
  {"bbox", (PyCFunction)python_bbox, METH_VARARGS | METH_KEYWORDS,
   "Get axis-aligned bounding box of an object.\n"
   "bbox(obj)"},
  {"size", (PyCFunction)python_size, METH_VARARGS | METH_KEYWORDS,
   "Get object's dimensions (bounding box size).\n"
   "size(obj)"},
  {"position", (PyCFunction)python_position, METH_VARARGS | METH_KEYWORDS,
   "Get object's minimum corner coordinates.\n"
   "position(obj)"},
  {"faces", (PyCFunction)python_faces, METH_VARARGS | METH_KEYWORDS,
   "Get list of faces of an object.\n"
   "faces(obj)\n"
   "faces(obj, triangulate=True)"},
  {"children", (PyCFunction)python_children, METH_VARARGS | METH_KEYWORDS,
   "Get an object's children as a tuple.\n"
   "children(obj)"},
  {"edges", (PyCFunction)python_edges, METH_VARARGS | METH_KEYWORDS,
   "Get list of edges of an object.\n"
   "edges(obj)"},
  {"explode", (PyCFunction)python_explode, METH_VARARGS | METH_KEYWORDS,
   "Move object's parts apart along a vector.\n"
   "explode(obj, v=[x, y, z])"},
  {"oversample", (PyCFunction)python_oversample, METH_VARARGS | METH_KEYWORDS,
   "Subdivide/refine a mesh, optionally with a texture.\n"
   "oversample(obj, size=2)\n"
   "oversample(obj, size=2, texture=\"leather\")\n"
   "oversample(obj, size=2, texture=\"leather\", texturewidth=1, textureheight=1)"},
  {"debug", (PyCFunction)python_debug, METH_VARARGS | METH_KEYWORDS,
   "Highlight specific faces for debugging.\n"
   "debug(obj)\n"
   "debug(obj, faces=[...])"},
  {"repair", (PyCFunction)python_repair, METH_VARARGS | METH_KEYWORDS,
   "Make a mesh watertight/manifold.\n"
   "repair(obj)\n"
   "repair(obj, color=\"red\")"},
  {"fillet", (PyCFunction)python_fillet, METH_VARARGS | METH_KEYWORDS,
   "Round edges of a solid.\n"
   "fillet(obj, r=radius)\n"
   "fillet(obj, r=radius, sel=[...])\n"
   "fillet(obj, r=radius, fn=32, minang=5)"},

  {"group", (PyCFunction)python_group, METH_VARARGS | METH_KEYWORDS,
   "Group object(s) without a boolean operation.\n"
   "group(obj)"},
  {"render", (PyCFunction)python_render, METH_VARARGS | METH_KEYWORDS,
   "Force full CGAL render (like SCAD render()).\n"
   "render(obj)\n"
   "render(obj, convexity=2)"},
  {"organic", (PyCFunction)python_organic, METH_VARARGS | METH_KEYWORDS,
   "Build an organic shape from points and a radius.\n"
   "organic(pts=[...], r=radius)"},
  {"osimport", (PyCFunction)python_import, METH_VARARGS | METH_KEYWORDS,
   "Import a file (STL, DXF, ...) as geometry.\n"
   "osimport(file=\"part.stl\")\n"
   "osimport(file=\"part.dxf\", layer=\"0\", convexity=2)"},
  {"osuse", (PyCFunction)python_osuse, METH_VARARGS | METH_KEYWORDS,
   "Use another OpenSCAD library file.\n"
   "osuse(file=\"library.scad\")"},
  {"osinclude", (PyCFunction)python_osinclude, METH_VARARGS | METH_KEYWORDS,
   "Include another OpenSCAD file's code.\n"
   "osinclude(file=\"library.scad\")"},
  {"version", (PyCFunction)python_osversion, METH_VARARGS | METH_KEYWORDS,
   "PythonSCAD version as [major, minor, patch].\n"
   "version()"},
  {"version_num", (PyCFunction)python_osversion_num, METH_VARARGS | METH_KEYWORDS,
   "PythonSCAD version as a single number.\n"
   "version_num()"},
  {"version_string", (PyCFunction)python_osversion_string, METH_VARARGS | METH_KEYWORDS,
   "PythonSCAD version as a string.\n"
   "version_string()"},
  {"_register_parameter", (PyCFunction)python_register_parameter, METH_VARARGS | METH_KEYWORDS,
   "Internal pure Customizer parameter registration helper."},
  {"add_parameter", (PyCFunction)python_add_parameter, METH_VARARGS | METH_KEYWORDS,
   "Register a Customizer parameter.\n"
   "add_parameter(name=\"size\", default=10)\n"
   "add_parameter(name=\"size\", default=10, description=\"Size of the part\", group=\"Dimensions\")\n"
   "add_parameter(name=\"size\", default=10, range=[1, 100], step=1)"},
  {"scad", (PyCFunction)python_scad, METH_VARARGS | METH_KEYWORDS,
   "Run inline OpenSCAD code.\n"
   "scad(code=\"cube(10);\")"},
  {"align", (PyCFunction)python_align, METH_VARARGS | METH_KEYWORDS,
   "Align object to another using reference matrices.\n"
   "align(obj, refmat)\n"
   "align(obj, refmat, objmat)\n"
   "align(obj, refmat, objmat, flip=True)"},
#ifndef OPENSCAD_NOGUI
  {"add_menuitem", (PyCFunction)python_add_menuitem, METH_VARARGS | METH_KEYWORDS,
   "Add a custom menu item to the GUI.\n"
   "add_menuitem(menuname=\"Tools\", itemname=\"My Action\", callback=\"my_callback\")"},

  {"add_parameter_widget", (PyCFunction)python_add_parameter_widget, METH_VARARGS | METH_KEYWORDS,
   "Register custom customizer widget type"},
  {"set_modal_dialog_active", python_set_modal_dialog_active, METH_VARARGS,
   "Suspend python_lock()/python_unlock() while a modal PyQt6 dialog is open"},
  {"nimport", (PyCFunction)python_nimport, METH_VARARGS | METH_KEYWORDS,
   "Deprecated: download and star-import a single .py file from a URL.\n"
   "Prefer a published Python package; see https://www.pythonscad.org/libraries/\n"
   "nimport(url=\"https://example.com/model.py\")  # deprecated"},
  {"qapp_ptr", python_qapp_ptr, METH_NOARGS,
   "Get raw pointer to the Qt application.\n"
   "qapp_ptr()"},
  {"mainwindow_ptr", (PyCFunction)python_mainwindow_ptr, METH_NOARGS,
   "Get raw pointer to the running main window.\n"
   "mainwindow_ptr()"},
  {"editor_get_call_args", (PyCFunction)python_editor_get_call_args, METH_VARARGS,
   "Get the current editor call's argument text.\n"
   "editor_get_call_args(position)"},
  {"editor_replace_call_args", (PyCFunction)python_editor_replace_call_args, METH_VARARGS,
   "Replace the current editor call's argument text.\n"
   "editor_replace_call_args(position, new_arguments)"},
#endif
  {"model", (PyCFunction)python_model, METH_VARARGS | METH_KEYWORDS,
   "Return the current top-level model object.\n"
   "model()"},
  {"modelpath", (PyCFunction)python_modelpath, METH_VARARGS | METH_KEYWORDS,
   "Absolute path of the running script.\n"
   "modelpath()"},
  {"memberfunction", (PyCFunction)python_memberfunction, METH_VARARGS | METH_KEYWORDS,
   "Register a custom member function on objects.\n"
   "memberfunction(membername=\"myattr\", memberfunc=my_func)\n"
   "memberfunction(membername=\"myattr\", memberfunc=my_func, docstring=\"...\")"},
  {"marked", (PyCFunction)python_marked, METH_VARARGS | METH_KEYWORDS,
   "Wrap a value so it is tracked/marked.\n"
   "marked(value)"},
  {"Sin", (PyCFunction)python_sin, METH_VARARGS | METH_KEYWORDS,
   "Sine of an angle in degrees.\n"
   "Sin(angle)"},
  {"Cos", (PyCFunction)python_cos, METH_VARARGS | METH_KEYWORDS,
   "Cosine of an angle in degrees.\n"
   "Cos(angle)"},
  {"Tan", (PyCFunction)python_tan, METH_VARARGS | METH_KEYWORDS,
   "Tangent of an angle in degrees.\n"
   "Tan(angle)"},
  {"Asin", (PyCFunction)python_asin, METH_VARARGS | METH_KEYWORDS,
   "Arc sine, result in degrees.\n"
   "Asin(value)"},
  {"Acos", (PyCFunction)python_acos, METH_VARARGS | METH_KEYWORDS,
   "Arc cosine, result in degrees.\n"
   "Acos(value)"},
  {"Atan", (PyCFunction)python_atan, METH_VARARGS | METH_KEYWORDS,
   "Arc tangent, result in degrees.\n"
   "Atan(value)"},
  {"norm", (PyCFunction)python_norm, METH_VARARGS | METH_KEYWORDS,
   "Length (magnitude) of a vector.\n"
   "norm(vec=[x, y, z])"},
  {"dot", (PyCFunction)python_dot, METH_VARARGS | METH_KEYWORDS,
   "Dot product of two vectors.\n"
   "dot(vec1=[x, y, z], vec2=[x, y, z])"},
  {"cross", (PyCFunction)python_cross, METH_VARARGS | METH_KEYWORDS,
   "Cross product of two vectors.\n"
   "cross(vec1=[x, y, z], vec2=[x, y, z])"},
  {"machineconfig", (PyCFunction)python_machineconfig, METH_VARARGS | METH_KEYWORDS,
   "Set machine configuration (for G-code export).\n"
   "machineconfig(config={...})"},
  {"rendervars", (PyCFunction)python_rendervars, METH_VARARGS | METH_KEYWORDS,
   "Set camera/render variables (viewport).\n"
   "rendervars(vpd=distance, vpf=fov)\n"
   "rendervars(vpr=[x, y, z], vpt=[x, y, z])"},
  {"vector", (PyCFunction)python_vector, METH_VARARGS | METH_KEYWORDS,
   "Create a 3D vector object.\n"
   "vector(x, y, z)"},
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
                                            wrap, "Wrap object around a cylinder or 2D outline")
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
