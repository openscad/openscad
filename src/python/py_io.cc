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

#include <iostream>
#include <fstream>
#include "genlang/genlang.h"
#include <Python.h>
#include "pyopenscad.h"
#include "pyfunctions.h"
#include "export.h"
#include "ImportNode.h"
#include <io/fileutils.h>
#include <GeometryEvaluator.h>
#include <platform/PlatformUtils.h>
#include <handle_dep.h>
#include <Expression.h>
#include "BuiltinContext.h"
#include <primitives.h>
#include "pydata.h"

extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename,
                  const std::string& mainFile, int debug);

PyObject *python_show_core(PyObject *obj)
{
  if (pythonDryRun) {
    Py_INCREF(obj);
    return obj;
  }
  python_result_obj = obj;
  PyObject *child_dict = nullptr;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in show");
    return NULL;
  }
  if (child == void_node) {
    Py_RETURN_NONE;
  }

  if (child == full_node) {
    PyErr_SetString(PyExc_TypeError, "Cannot display infinite space");
    Py_RETURN_TRUE;
  }

  PyObject *key, *value;
  Py_ssize_t pos = 0;
  python_build_hashmap(child, 0);
  std::string varname = child->getPyName();
  if (child_dict != nullptr) {
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      Matrix4d raw;
      if (python_tomatrix(value, raw)) continue;
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str = PyBytes_AS_STRING(value1);
      SelectedObject sel;
      sel.pt.clear();
      sel.pt.push_back(Vector3d(raw(0, 3), raw(1, 3), raw(2, 3)));
      sel.pt.push_back(Vector3d(raw(0, 0), raw(1, 0), raw(2, 0)));
      sel.pt.push_back(Vector3d(raw(0, 1), raw(1, 1), raw(2, 1)));
      sel.pt.push_back(Vector3d(raw(0, 2), raw(1, 2), raw(2, 2)));
      sel.type = SelectionType::SELECTION_HANDLE;
      sel.name = varname + "." + value_str;
      python_result_handle.push_back(sel);
    }
  }
  shows.push_back(child);
  return obj;
}

PyObject *python_show(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *result = Py_None;
  if (args == nullptr) return result;
  for (int i = 0; i < PyTuple_Size(args); i++) {
    result = python_show_core(PyTuple_GetItem(args, i));
    if (result == nullptr) return result;
  }
  if (result != Py_None) Py_INCREF(result);
  return result;
}

PyObject *python_oo_show(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  auto res = python_show_core(obj);
  Py_INCREF(res);
  return res;
}

PyObject *python_output(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  LOG(message_group::Deprecated, "output is deprecated, please use show() instead");
  return python_show(obj, args, kwargs);
}

void Export3mfPartInfo::writeProps(void *obj) const
{
  if (this->props == nullptr) return;
  PyObject *prop = (PyObject *)this->props;
  if (!PyDict_Check(prop)) return;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(prop, &pos, &key, &value)) {
    PyObject *key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
    const char *key_str = PyBytes_AS_STRING(key1);
    if (key_str == nullptr) continue;
    if (PyFloat_Check(value)) {
      writePropsFloat(obj, key_str, PyFloat_AsDouble(value));
    }
    if (PyLong_Check(value)) {
      writePropsLong(obj, key_str, PyLong_AsLong(value));
    }
    if (PyUnicode_Check(value)) {
      PyObject *val1 = PyUnicode_AsEncodedString(value, "utf-8", "~");
      const char *val_str = PyBytes_AS_STRING(val1);
      writePropsString(obj, key_str, val_str);
    }
  }
}

void python_export_obj_att(std::ostream& output)
{
  PyObject *child_dict = nullptr;
  if (python_result_obj == nullptr) return;
  PyOpenSCADObjectToNodeMulti(python_result_obj, &child_dict);
  if (child_dict == nullptr) return;
  if (!PyDict_Check(child_dict)) return;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(child_dict, &pos, &key, &value)) {
    PyObject *key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
    const char *key_str = PyBytes_AS_STRING(key1);
    if (key_str == nullptr) continue;

    if (PyLong_Check(value)) output << "# " << key_str << " = " << PyLong_AsLong(value) << "\n";

    if (PyFloat_Check(value)) output << "# " << key_str << " = " << PyFloat_AsDouble(value) << "\n";

    if (PyUnicode_Check(value)) {
      auto valuestr = std::string(PyUnicode_AsUTF8(value));
      output << "# " << key_str << " = \"" << valuestr << "\"\n";
    }
  }
}

PyObject *python_export_core(PyObject *obj, char *file)
{
  if (pythonDryRun) {
    Py_RETURN_NONE;
  }
  std::string filename;
  if (python_scriptpath.string().size() > 0)
    filename = lookup_file(file, python_scriptpath.parent_path().u8string(), ".");  // TODO problem hbier
  else filename = file;
  const auto path = fs::path(filename);
  std::string suffix = path.has_extension() ? path.extension().generic_string().substr(1) : "";
  boost::algorithm::to_lower(suffix);
  python_result_obj = obj;

  FileFormat exportFileFormat = FileFormat::BINARY_STL;
  if (!fileformat::fromIdentifier(suffix, exportFileFormat)) {
    LOG("Invalid suffix %1$s. Defaulting to binary STL.", suffix);
  }

  std::vector<Export3mfPartInfo> export3mfPartInfos;

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child != nullptr) {
    Tree tree(child, "parent");
    GeometryEvaluator geomevaluator(tree);
    Export3mfPartInfo info(geomevaluator.evaluateGeometry(*tree.root(), false), "OpenSCAD Model",
                           nullptr);
    export3mfPartInfos.push_back(info);
  } else if (PyDict_Check(obj)) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(obj, &pos, &key, &value)) {
      PyObject *value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str = PyBytes_AS_STRING(value1);
      if (value_str == nullptr) continue;
      std::shared_ptr<AbstractNode> dict_child = PyOpenSCADObjectToNodeMulti(value, &child_dict);
      if (dict_child == nullptr) continue;

      void *prop = nullptr;
      if (child_dict != nullptr && PyDict_Check(child_dict)) {
        PyObject *props_key = PyUnicode_FromStringAndSize("props_3mf", 9);
        prop = PyDict_GetItem(child_dict, props_key);
      }
      Tree tree(dict_child, "parent");
      GeometryEvaluator geomevaluator(tree);
      Export3mfPartInfo info(geomevaluator.evaluateGeometry(*tree.root(), false), value_str, prop);
      export3mfPartInfos.push_back(info);
    }
  }
  if (export3mfPartInfos.size() == 0) {
    PyErr_SetString(PyExc_TypeError, "Object not recognized");
    return NULL;
  }

  Export3mfOptions options3mf;
  options3mf.decimalPrecision = 6;
  options3mf.color = "#f9d72c";
  Camera camera;
  camera.viewall = true;
  camera.autocenter = true;
  ExportInfo exportInfo =
    createExportInfo(exportFileFormat, fileformat::info(exportFileFormat), file, &camera, {});

  if (exportFileFormat == FileFormat::_3MF) {
    std::ofstream fstream(file, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fstream.is_open()) {
      PyErr_SetString(PyExc_TypeError, "Can't write export file");
      return nullptr;
    }
    export_3mf(export3mfPartInfos, fstream, exportInfo);
  } else {
    if (export3mfPartInfos.size() > 1) {
      PyErr_SetString(PyExc_TypeError, "This Format can at most export one object");
      return nullptr;
    }
    exportFileByName(export3mfPartInfos[0].geom, file, exportInfo);
  }
  Py_RETURN_NONE;
}

PyObject *python_export(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *file = nullptr;
  char *kwlist[] = {"obj", "file", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Os|O", kwlist, &obj, &file)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_export_core(obj, file);
}

PyObject *python_oo_export(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"file", NULL};
  char *file = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_export_core(obj, file);
}

PyObject *do_import_python(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type)
{
  DECLARE_INSTANCE();
  char *kwlist[] = {"file", "layer", "convexity", "origin", "scale", "width", "height", "center",
                    "dpi",  "id",    "stroke",    "fn",     "fa",    "fs",    NULL};
  double fn = NAN, fa = NAN, fs = NAN;
  PyObject *stroke = nullptr;

  std::string filename;
  const char *v = NULL, *layer = NULL, *id = NULL;
  PyObject *center = NULL;
  int convexity = 2;
  double scale = 1.0, width = 1, height = 1, dpi = 1.0;
  PyObject *origin = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|siO!dddOdsOddd", kwlist, &v, &layer, &convexity,
                                   &PyList_Type, &origin, &scale, &width, &height, &center, &dpi, &id,
                                   &stroke, &fn, &fa, &fs

                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing osimport(filename)");
    return NULL;
  }
  filename = lookup_file(v == NULL ? "" : v, python_scriptpath.parent_path().u8string(),
                         instance->location().filePath().parent_path().string());
  if (!filename.empty()) handle_dep(filename);
  ImportType actualtype = type;
  if (actualtype == ImportType::UNKNOWN) {
    std::string extraw = fs::path(filename).extension().generic_string();
    std::string ext = boost::algorithm::to_lower_copy(extraw);
    if (ext == ".stl") actualtype = ImportType::STL;
    else if (ext == ".off") actualtype = ImportType::OFF;
    else if (ext == ".obj") actualtype = ImportType::OBJ;
    else if (ext == ".dxf") actualtype = ImportType::DXF;
    else if (ext == ".nef3") actualtype = ImportType::NEF3;
    else if (ext == ".3mf") actualtype = ImportType::_3MF;
    else if (ext == ".amf") actualtype = ImportType::AMF;
    else if (ext == ".svg") actualtype = ImportType::SVG;
    else if (ext == ".cdr") actualtype = ImportType::CDR;
    else if (ext == ".stp") actualtype = ImportType::STEP;
    else if (ext == ".step") actualtype = ImportType::STEP;
  }

  auto node = std::make_shared<ImportNode>(instance, actualtype, CreateCurveDiscretizer(kwargs));

  node->filename = filename;

  if (layer != NULL) node->layer = layer;
  if (id != NULL) node->id = id;
  node->convexity = convexity;
  if (node->convexity <= 0) node->convexity = 1;

  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }

  node->center = 0;
  if (center == Py_True) node->center = 1;

  node->stroke = true;
  if (stroke == Py_False) node->stroke = false;

  node->scale = scale;
  if (node->scale <= 0) node->scale = 1;

  node->dpi = ImportNode::SVG_DEFAULT_DPI;
  double val = dpi;
  if (val < 0.001) {
    PyErr_SetString(PyExc_TypeError, "Invalid dpi value giving");
    return NULL;
  } else {
    node->dpi = val;
  }

  node->width = width;
  node->height = height;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_import(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return do_import_python(self, args, kwargs, ImportType::UNKNOWN);
}

#ifndef OPENSCAD_NOGUI
std::vector<std::string> nimport_downloaded;

extern int curl_download(const std::string& url, const std::string& path);
PyObject *python_nimport(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"url", NULL};
  const char *c_url = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &c_url)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing nimport(filename)");
    return NULL;
  }
  if (c_url == nullptr) Py_RETURN_NONE;

  std::string url = c_url;
  std::string filename, path, importcode;
  filename = url.substr(url.find_last_of("/") + 1);
  importcode = "from " + filename.substr(0, filename.find_last_of(".")) + " import *";

  path = PlatformUtils::userLibraryPath() + "/" + filename;
  bool do_download = false;
  if (std::find(nimport_downloaded.begin(), nimport_downloaded.end(), url) == nimport_downloaded.end()) {
    do_download = true;
    nimport_downloaded.push_back(url);
  }

  std::ifstream f(path.c_str());
  if (!f.good()) {
    do_download = true;
  }

  if (do_download) {
    curl_download(url, path);
  }

  PyRun_SimpleString(importcode.c_str());
  Py_RETURN_NONE;
}
#endif

void python_str_sub(std::ostringstream& stream, const std::shared_ptr<AbstractNode>& node, int ident)
{
  for (int i = 0; i < ident; i++) stream << "  ";
  stream << node->toString();
  switch (node->children.size()) {
  case 0: stream << ";\n"; break;
  case 1:
    stream << "\n";
    python_str_sub(stream, node->children[0], ident + 1);
    break;
  default:
    stream << "{\n";
    for (const auto& child : node->children) {
      python_str_sub(stream, child, ident + 1);
    }
    for (int i = 0; i < ident; i++) stream << "  ";
    stream << "}\n";
  }
}

PyObject *python_str(PyObject *self)
{
  std::ostringstream stream;
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(self, &dummydict);
  if (node != nullptr) python_str_sub(stream, node, 0);
  else stream << "Invalid OpenSCAD Object";

  return PyUnicode_FromStringAndSize(stream.str().c_str(), stream.str().size());
}

PyObject *python_add_parameter(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type)
{
  char *kwlist[] = {"name", "default",    "description", "group", "range",
                    "step", "max_length", "options",     NULL};
  char *name = NULL;
  PyObject *value = NULL;
  const char *description = NULL;
  const char *group = NULL;
  PyObject *range_obj = NULL;
  double step_val = -1.0;
  int max_length = -1;
  PyObject *options = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|zzOdiO", kwlist, &name, &value, &description,
                                   &group, &range_obj, &step_val, &max_length, &options)) {
    PyErr_SetString(PyExc_TypeError,
                    "Error during parsing add_parameter(name, default, [description], [group], "
                    "[range], [step], [max_length], [options])");
    return NULL;
  }

  // Type-specific parameter validation
  bool is_string = PyUnicode_Check(value);
  bool is_number = PyFloat_Check(value) || PyLong_Check(value);
  bool is_bool = (value == Py_True || value == Py_False);
  bool is_list = PyList_Check(value);

  if (max_length > 0 && !is_string) {
    PyErr_SetString(PyExc_TypeError, "add_parameter(): 'max_length' only applies to string parameters");
    return NULL;
  }

  if ((range_obj != NULL || step_val > 0) && !is_number && !is_list) {
    PyErr_SetString(PyExc_TypeError,
                    "add_parameter(): 'range' and 'step' only apply to numeric or vector parameters");
    return NULL;
  }

  if (is_list) {
    Py_ssize_t size = PyList_Size(value);
    if (size < 1 || size > 4) {
      PyErr_SetString(PyExc_ValueError, "add_parameter(): vector parameters must have 1-4 elements");
      return NULL;
    }
  }

  // Extract range constraints from range() object or tuple
  double min_val = NAN, max_val = NAN, range_step = NAN;
  if (range_obj != NULL) {
    if (PyObject_HasAttrString(range_obj, "start") && PyObject_HasAttrString(range_obj, "stop") &&
        PyObject_HasAttrString(range_obj, "step")) {
      // Python range() object - extract start, stop, step
      PyObject *start = PyObject_GetAttrString(range_obj, "start");
      PyObject *stop = PyObject_GetAttrString(range_obj, "stop");
      PyObject *step = PyObject_GetAttrString(range_obj, "step");
      if (start && stop && step) {
        min_val = PyLong_AsDouble(start);
        max_val = PyLong_AsDouble(stop) - 1;  // Convert exclusive to inclusive
        range_step = PyLong_AsDouble(step);
      }
      Py_XDECREF(start);
      Py_XDECREF(stop);
      Py_XDECREF(step);
    } else if (PyTuple_Check(range_obj)) {
      // Tuple: (min, max) or (min, max, step)
      Py_ssize_t size = PyTuple_Size(range_obj);
      if (size >= 2) {
        PyObject *item0 = PyTuple_GetItem(range_obj, 0);
        PyObject *item1 = PyTuple_GetItem(range_obj, 1);
        if (item0 != Py_None) {
          if (PyFloat_Check(item0)) min_val = PyFloat_AsDouble(item0);
          else if (PyLong_Check(item0)) min_val = PyLong_AsDouble(item0);
        }
        if (item1 != Py_None) {
          if (PyFloat_Check(item1)) max_val = PyFloat_AsDouble(item1);
          else if (PyLong_Check(item1)) max_val = PyLong_AsDouble(item1);
        }
      }
      if (size >= 3) {
        PyObject *item2 = PyTuple_GetItem(range_obj, 2);
        if (item2 != Py_None) {
          if (PyFloat_Check(item2)) range_step = PyFloat_AsDouble(item2);
          else if (PyLong_Check(item2)) range_step = PyLong_AsDouble(item2);
        }
      }
    }
  }
  // Use 'step' kwarg if range didn't provide one
  if (std::isnan(range_step) && step_val > 0) {
    range_step = step_val;
  }

  // Create default value expression
  std::shared_ptr<Expression> default_expr;
  bool found = false;

  if (is_bool) {
    default_expr = std::make_shared<Literal>(value == Py_True, Location::NONE);
    found = true;
  } else if (PyFloat_Check(value)) {
    default_expr = std::make_shared<Literal>(PyFloat_AsDouble(value), Location::NONE);
    found = true;
  } else if (PyLong_Check(value)) {
    default_expr = std::make_shared<Literal>(PyLong_AsLong(value) * 1.0, Location::NONE);
    found = true;
  } else if (is_string) {
    PyObject *value1 = PyUnicode_AsEncodedString(value, "utf-8", "~");
    const char *value_str = PyBytes_AS_STRING(value1);
    std::string value_string(value_str);
    Py_DECREF(value1);
    default_expr = std::make_shared<Literal>(value_string, Location::NONE);
    found = true;
  } else if (is_list) {
    // Vector parameter
    auto vec = std::make_shared<Vector>(Location::NONE);
    Py_ssize_t size = PyList_Size(value);
    for (Py_ssize_t i = 0; i < size; i++) {
      PyObject *item = PyList_GetItem(value, i);
      double item_val = 0.0;
      if (PyFloat_Check(item)) {
        item_val = PyFloat_AsDouble(item);
      } else if (PyLong_Check(item)) {
        item_val = PyLong_AsDouble(item);
      } else {
        PyErr_SetString(PyExc_TypeError, "add_parameter(): vector elements must be numeric");
        return NULL;
      }
      vec->emplace_back(new Literal(item_val, Location::NONE));
    }
    default_expr = vec;
    found = true;
  }

  if (found) {
    auto *annotationList = new AnnotationList();

    // Create Parameter annotation with constraints
    std::shared_ptr<Expression> param_expr;

    if (options != NULL) {
      // Enum/dropdown parameter
      auto vec = std::make_shared<Vector>(Location::NONE);
      if (PyList_Check(options)) {
        // Simple list: ["a", "b", "c"]
        Py_ssize_t size = PyList_Size(options);
        for (Py_ssize_t i = 0; i < size; i++) {
          PyObject *item = PyList_GetItem(options, i);
          if (PyUnicode_Check(item)) {
            PyObject *encoded = PyUnicode_AsEncodedString(item, "utf-8", "~");
            std::string item_string(PyBytes_AS_STRING(encoded));
            Py_DECREF(encoded);
            vec->emplace_back(new Literal(item_string, Location::NONE));
          } else if (PyFloat_Check(item)) {
            vec->emplace_back(new Literal(PyFloat_AsDouble(item), Location::NONE));
          } else if (PyLong_Check(item)) {
            vec->emplace_back(new Literal(PyLong_AsDouble(item), Location::NONE));
          }
        }
      } else if (PyDict_Check(options)) {
        // Dict with labels: {10: "Low", 20: "High"}
        PyObject *key, *label;
        Py_ssize_t pos = 0;
        while (PyDict_Next(options, &pos, &key, &label)) {
          auto item_vec = new Vector(Location::NONE);
          // Value
          if (PyFloat_Check(key)) {
            item_vec->emplace_back(new Literal(PyFloat_AsDouble(key), Location::NONE));
          } else if (PyLong_Check(key)) {
            item_vec->emplace_back(new Literal(PyLong_AsDouble(key), Location::NONE));
          } else if (PyUnicode_Check(key)) {
            PyObject *encoded = PyUnicode_AsEncodedString(key, "utf-8", "~");
            std::string key_string(PyBytes_AS_STRING(encoded));
            Py_DECREF(encoded);
            item_vec->emplace_back(new Literal(key_string, Location::NONE));
          }
          // Label
          if (PyUnicode_Check(label)) {
            PyObject *encoded = PyUnicode_AsEncodedString(label, "utf-8", "~");
            std::string label_string(PyBytes_AS_STRING(encoded));
            Py_DECREF(encoded);
            item_vec->emplace_back(new Literal(label_string, Location::NONE));
          }
          vec->emplace_back(item_vec);
        }
      }
      param_expr = vec;
    } else if (!std::isnan(min_val) && !std::isnan(max_val)) {
      // Range with min and max -> slider
      if (!std::isnan(range_step)) {
        param_expr = std::make_shared<Range>(new Literal(min_val, Location::NONE),
                                             new Literal(range_step, Location::NONE),
                                             new Literal(max_val, Location::NONE), Location::NONE);
      } else {
        param_expr = std::make_shared<Range>(new Literal(min_val, Location::NONE),
                                             new Literal(max_val, Location::NONE), Location::NONE);
      }
    } else if (!std::isnan(range_step)) {
      // Step only -> spinbox
      param_expr = std::make_shared<Literal>(range_step, Location::NONE);
    } else if (max_length > 0) {
      // String max length
      param_expr = std::make_shared<Literal>((double)max_length, Location::NONE);
    } else {
      // No constraints - create empty marker for Parameter annotation
      param_expr = std::make_shared<Literal>("", Location::NONE);
    }

    annotationList->push_back(Annotation("Parameter", param_expr));

    if (description != NULL) {
      annotationList->push_back(
        Annotation("Description", std::make_shared<Literal>(description, Location::NONE)));
    }

    if (group != NULL) {
      annotationList->push_back(Annotation("Group", std::make_shared<Literal>(group, Location::NONE)));
    }

    auto assignment = std::make_shared<Assignment>(name, default_expr);
    assignment->addAnnotations(annotationList);
    customizer_parameters.push_back(assignment);

    PyObject *value_effective = value;
    for (unsigned int i = 0; i < customizer_parameters_finished.size(); i++) {
      if (customizer_parameters_finished[i]->getName() == name) {
        auto expr = customizer_parameters_finished[i]->getExpr();
        const auto& lit = std::dynamic_pointer_cast<Literal>(expr);
        if (lit != nullptr) {
          if (lit->isDouble()) value_effective = PyFloat_FromDouble(lit->toDouble());
          if (lit->isString()) value_effective = PyUnicode_FromString(lit->toString().c_str());
          if (lit->isBool()) value_effective = lit->toBool() ? Py_True : Py_False;
        }
        // Handle vector values
        const auto& vec = std::dynamic_pointer_cast<Vector>(expr);
        if (vec != nullptr) {
          PyObject *result_list = PyList_New(vec->getChildren().size());
          for (size_t j = 0; j < vec->getChildren().size(); j++) {
            const auto& child_lit = std::dynamic_pointer_cast<Literal>(vec->getChildren()[j]);
            if (child_lit && child_lit->isDouble()) {
              PyList_SetItem(result_list, j, PyFloat_FromDouble(child_lit->toDouble()));
            }
          }
          value_effective = result_list;
        }
      }
    }
    // Only set global variable if the pure function feature is not enabled
    // (default: creates variable for backward compatibility)
    if (!Feature::ExperimentalAddParameterPureFunction.is_enabled()) {
      PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
      PyDict_SetItemString(maindict, name, value_effective);
    }
    Py_INCREF(value_effective);
    return value_effective;
  }
  Py_RETURN_NONE;
}

PyObject *python_scad(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  char *kwlist[] = {"code", NULL};
  const char *code = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &code)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scad(code)");
    return NULL;
  }

  SourceFile *parsed_file = NULL;
  if (!parse(parsed_file, code, "python", "python", false)) {
    PyErr_SetString(PyExc_TypeError, "Error in SCAD code");
    Py_RETURN_NONE;
  }
  parsed_file->handleDependencies(true);

  EvaluationSession session{"python"};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> resultnode = parsed_file->instantiate(*builtin_context, &file_context);
  resultnode = resultnode->clone();  // instmod will go out of scope
  delete parsed_file;
  parsed_file = nullptr;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, resultnode);
}

PyObject *python_osuse_include(int mode, PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE();
  auto empty = std::make_shared<CubeNode>(instance);
  char *kwlist[] = {"file", NULL};
  const char *file = NULL;
  std::ostringstream stream;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file)) {
    if (mode) PyErr_SetString(PyExc_TypeError, "Error during parsing osinclude(path)");
    else PyErr_SetString(PyExc_TypeError, "Error during parsing osuse(path)");
    return NULL;
  }
  const std::string includedfile = lookup_file(file, python_scriptpath.parent_path().u8string(), ".");
  stream << "include <" << includedfile << ">\n";

  // Pass the Python script path as the "source" file doing the including
  std::string scriptpath = python_scriptpath.u8string();

  SourceFile *source;
  if (!parse(source, stream.str(), scriptpath, scriptpath, false)) {
    PyErr_SetString(PyExc_TypeError, "Error in SCAD code");
    Py_RETURN_NONE;
  }
  if (mode == 0) source->scope->moduleInstantiations.clear();
  source->handleDependencies(true);

  EvaluationSession *session = new EvaluationSession(python_scriptpath.parent_path().u8string());
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(session)};

  std::shared_ptr<const FileContext> osinclude_context;
  source->instantiate(*builtin_context, &osinclude_context);  // TODO keine globakle var, kollision!

  auto scope = source->scope;
  PyOpenSCADObject *result = (PyOpenSCADObject *)PyOpenSCADObjectFromNode(&PyOpenSCADType, empty);

  for (const auto& mod : source->scope->modules) {  // copy modules
    PyDict_SetItemString(result->dict, mod.first.c_str(),
                         PyDataObjectFromModule(&PyDataType, includedfile, mod.first));
  }

  for (const auto& fun : source->scope->functions) {  // copy functions
    PyDict_SetItemString(result->dict, fun.first.c_str(),
                         PyDataObjectFromFunction(&PyDataType, includedfile, fun.first));
  }

  for (auto ass : source->scope->assignments) {  // copy assignments
                                                 //    printf("Var %s\n",ass->getName().c_str());
    const std::shared_ptr<Expression> expr = ass->getExpr();
    Value val = expr->evaluate(osinclude_context);
    if (val.isDefined()) {
      PyObject *res = python_fromopenscad(std::move(val));
      PyDict_SetItemString(result->dict, ass->getName().c_str(), res);
    }
  }
  // SourceFileCache::instance()->clear();
  return (PyObject *)result;
}

PyObject *python_osuse(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_osuse_include(0, self, args, kwargs);
}

PyObject *python_osinclude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  LOG(message_group::Deprecated, "osinclude  is deprecated, please use osuse() instead");
  return python_osuse_include(1, self, args, kwargs);
}

#ifndef OPENSCAD_NOGUI
extern void add_menuitem_trampoline(const char *menuname, const char *itemname, const char *callback);
PyObject *python_add_menuitem(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"menuname", "itemname", "callback", NULL};
  const char *menuname = nullptr, *itemname = nullptr, *callback = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss", kwlist, &menuname, &itemname, &callback)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing add_menuitem");
    return NULL;
  }
  add_menuitem_trampoline(menuname, itemname, callback);
  Py_RETURN_NONE;
}
#endif

PyObject *python_model(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing model");
    return NULL;
  }
  if (genlang_result_node == nullptr) Py_RETURN_NONE;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, genlang_result_node);
}

// global accessible version of the machine config settings that are
// used by export_gcode for colormapping power and feed.
extern boost::property_tree::ptree _machineconfig_settings_;

// convert a python dictionary directly to a property tree
boost::property_tree::ptree pyToPtree(PyObject *obj)
{
  boost::property_tree::ptree pt;

  if (PyDict_Check(obj)) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(obj, &pos, &key, &value)) {
      std::string keyStr = PyUnicode_AsUTF8(key);
      pt.add_child(keyStr, pyToPtree(value));
    }
  } else if (PyList_Check(obj)) {
    Py_ssize_t size = PyList_Size(obj);
    for (Py_ssize_t i = 0; i < size; ++i) {
      PyObject *item = PyList_GetItem(obj, i);  // borrowed reference
      pt.push_back(std::make_pair("", pyToPtree(item)));
    }
  } else if (PyBool_Check(obj)) {
    // important: Check Bool BEFORE Long.
    bool value = (obj == Py_True);
    pt.put("", value);
  } else if (PyLong_Check(obj)) {
    pt.put("", PyLong_AsLongLong(obj));
  } else if (PyFloat_Check(obj)) {
    pt.put("", PyFloat_AsDouble(obj));
  } else if (PyUnicode_Check(obj)) {
    pt.put("", PyUnicode_AsUTF8(obj));
  } else if (obj == Py_None) {
    // property_tree does not support a true 'null', so set it to an
    // empty node.
    pt.put("", "");
  }

  return pt;
}

PyObject *python_machineconfig(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"config", NULL};
  PyObject *config;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &config)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing machineconfig");
    return NULL;
  }

  if (!PyDict_Check(config)) {
    PyErr_SetString(PyExc_TypeError, "Config must be a dictionary");
    return NULL;
  }

  // parse the python dictionary directly into a property tree
  _machineconfig_settings_ = pyToPtree(config);

  Py_RETURN_NONE;
}

PyObject *python_rendervars(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"vpd", "vpf", "vpr", "vpt", NULL};
  double x, y, z;
  double vpd = NAN;
  double vpf = NAN;
  PyObject *vpr = nullptr;
  PyObject *vpt = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddOO", kwlist, &vpd, &vpf, &vpr, &vpt)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rendervars");
    return NULL;
  }
  renderVarsSet = std::make_shared<RenderVariables>();

  if (!isnan(vpd)) renderVarsSet->camera.viewer_distance = vpd;
  if (!isnan(vpf)) renderVarsSet->camera.fov = vpf;
  if (vpt != nullptr && !python_vectorval(vpt, 3, 3, &x, &y, &z))
    renderVarsSet->camera.object_trans = Vector3d(x, y, z);
  if (vpr != nullptr && !python_vectorval(vpr, 3, 3, &x, &y, &z))
    renderVarsSet->camera.object_rot = Vector3d(x, y, z);

  Py_RETURN_NONE;
}
