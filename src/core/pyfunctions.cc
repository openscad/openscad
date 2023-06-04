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


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <Python.h>
#include <pyopenscad.h>

#include "primitives.h"
#include "TransformNode.h"
#include "RotateExtrudeNode.h"
#include "LinearExtrudeNode.h"
#include "CgalAdvNode.h"
#include "CsgOpNode.h"
#include "ColorNode.h"
#include "RoofNode.h"
#include "RenderNode.h"
#include "SurfaceNode.h"
#include "TextNode.h"
#include "OffsetNode.h"
#include "ProjectionNode.h"
#include "ImportNode.h"

#include "degree_trig.h"
#include "printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"

//using namespace boost::assign; // bring 'operator+=()' into scope


// Colors extracted from https://drafts.csswg.org/css-color/ on 2015-08-02
// CSS Color Module Level 4 - Editor’s Draft, 29 May 2015
extern std::unordered_map<std::string, Color4f> webcolors;
extern boost::optional<Color4f> parse_hex_color(const std::string& hex);


PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CubeNode>(instance);

  char *kwlist[] = {"dim", "center", NULL};
  PyObject *dim = NULL;

  double x = 1, y = 1, z = 1;
  PyObject *center = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &dim,
                                   &center)){
    PyErr_SetString(PyExc_TypeError, "error duing parsing cube");
    return NULL;
  }	  

  if (dim != NULL) {
    double x, y, z;
    if (python_vectorval(dim, &(node->x), &(node->y), &(node->z))) {
      PyErr_SetString(PyExc_TypeError, "Invalid Cube dimensions");
      return NULL;
    }
  }
  if(node->x <= 0 || node->y <= 0 || node ->z <= 0) {
      PyErr_SetString(PyExc_TypeError, "Cube Dimensions must be positive");
      return NULL;
  }
  if (center == Py_True)  node->center = 1;
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for Cube center parameter");
      return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_sphere(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<SphereNode>(instance);

  char *kwlist[] = {"r", "d", "fn", "fa", "fs", NULL};
  double r = NAN;
  double d = NAN;
  double fn = NAN, fa = NAN, fs = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddd", kwlist,
                                   &r, &d, &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing sphere");
    return NULL;
  } 
  if (!isnan(r)) {
    if(r <= 0) {
      PyErr_SetString(PyExc_TypeError, "r must be positive");
      return NULL;
    }	    
    vr = r;
    if(!isnan(d)) {
      PyErr_SetString(PyExc_TypeError, "Cant specify r and d at the same time for sphere");
      return NULL;
    }
  } 
  if (!isnan(d)) {
    if(d <= 0) {
      PyErr_SetString(PyExc_TypeError, "d must be positive");
      return NULL;
    }	    
    vr = d / 2.0;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  node->r = vr;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CylinderNode>(instance);

  char *kwlist[] = {"h", "r", "r1", "r2", "d", "d1", "d2", "center", "fn", "fa", "fs", NULL};
  double h = NAN;
  double r = NAN;
  double r1 = NAN;
  double r2 = NAN;
  double d = NAN;
  double d1 = NAN;
  double d2 = NAN;

  double fn = NAN, fa = NAN, fs = NAN;

  PyObject *center = NULL;
  double vr1 = 1, vr2 = 1, vh = 1;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dddddddOddd", kwlist, &h, &r, &r1, &r2, &d, &d1, &d2, &center, &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing cylinder");
    return NULL;
  }

  if(h <= 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder height must be positive");
    return NULL;
  }
  vh = h;

  if(!isnan(r) && r <= 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder r must be positive");
    return NULL;
  }
  if(!isnan(d) && d <= 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder r must be positive");
    return NULL;
  }
  if(!isnan(r1) && r1 < 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder r1 must not be negative");
    return NULL;
  }
  if(!isnan(r2) && r2 < 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder r2 must not be negative");
    return NULL;
  }
  if(!isnan(d1) && d1 < 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder d1 must not be negative");
    return NULL;
  }
  if(!isnan(d2) && d2 < 0) {
    PyErr_SetString(PyExc_TypeError, "cylinder d2 must not be negative");
    return NULL;
  }

  if (!isnan(r1) && !isnan(r2)) {
    vr1 = r1; vr2 = r2;
  } else if (!isnan(d1) && !isnan(d2)) {
    vr1 = d1 / 2.0; vr2 = d2 / 2.0;
  } else if (!isnan(r)) {
    vr1 = r; vr2 = r;
  } else if (!isnan(d)) {
    vr1 = d / 2.0; vr2 = d / 2.0;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  node->r1 = vr1;
  node->r2 = vr2;
  node->h = vh;

  if (center == Py_True) node->center = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  int i, j, pointIndex;
  auto node = std::make_shared<PolyhedronNode>(instance);

  char *kwlist[] = {"points", "faces", "convexity", "triangles", NULL};
  PyObject *points = NULL;
  PyObject *faces = NULL;
  int convexity = 2;
  PyObject *triangles = NULL;

  PyObject *element;
  point3d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|iO!", kwlist,
                                   &PyList_Type, &points,
                                   &PyList_Type, &faces,
                                   &convexity,
                                   &PyList_Type, &triangles
                                   )) {
	  PyErr_SetString(PyExc_TypeError, "error duing parsing polyhedron");
	  return NULL;
  } 

  if (points != NULL && PyList_Check(points)) {
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 3) {
        point.x = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point.y = PyFloat_AsDouble(PyList_GetItem(element, 1));
        point.z = PyFloat_AsDouble(PyList_GetItem(element, 2));
        node->points.push_back(point);
      }

    }
  }

  if (triangles != NULL) {
    faces = triangles;
//	LOG(message_group::Deprecated, inst->location(), parameters.documentRoot(), "polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
  }

  if (faces != NULL && PyList_Check(faces) ) {
    for (i = 0; i < PyList_Size(faces); i++) {
      element = PyList_GetItem(faces, i);
      if (PyList_Check(element)) {
        std::vector<size_t> face;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
          face.push_back(pointIndex);
        }
        if (face.size() >= 3) {
          node->faces.push_back(std::move(face));
        }
      }
    }
  }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_square(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<SquareNode>(instance);

  char *kwlist[] = {"dim", "center", NULL};
  PyObject *dim = NULL;

  double x = 1, y = 1;
  PyObject *center = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist,
                                   &dim,
                                   &center)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing square");
    return NULL;
  }
  if (dim != NULL) {
    double z;
    if (python_vectorval(dim, &(node->x), &(node->y), &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid Square dimensions");
      return NULL;
    }
  }
  if (center == Py_True) node->center = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_circle(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CircleNode>(instance);

  char *kwlist[] = {"r", "d", "fn", "fa", "fs", NULL};
  double r = NAN;
  double d = NAN;
  double fn = NAN, fa = NAN, fs = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddd", kwlist, &r, &d, &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing circle");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  if (!isnan(r)) {
    vr = r;
  } else if (!isnan(d)){
    vr = d / 2.0;
  }


  node->r = vr;


  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_polygon(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  int i, j, pointIndex;
  auto node = std::make_shared<PolygonNode>(instance);

  char *kwlist[] = {"points", "paths", "convexity", NULL};
  PyObject *points = NULL;
  PyObject *paths = NULL;
  int convexity = 2;

  PyObject *element;
  point2d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!i", kwlist,
                                   &PyList_Type, &points,
                                   &PyList_Type, &paths,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing polygon");
    return NULL;
  }

  if (points != NULL && PyList_Check(points) ) {
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 2) {
        point.x = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point.y = PyFloat_AsDouble(PyList_GetItem(element, 1));
        node->points.push_back(point);
      }

    }
  }

  if (paths != NULL && PyList_Check(paths) ) {
    for (i = 0; i < PyList_Size(paths); i++) {
      element = PyList_GetItem(paths, i);
      if (PyList_Check(element)) {
        std::vector<size_t> path;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
          path.push_back(pointIndex);
        }
        node->paths.push_back(std::move(path));
      }
    }
  }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}
PyObject *python_scale(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(instance, "scale");

  char *kwlist[] = {"obj", "v", NULL};

  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist,
                                   &obj,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing scale");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in scale");
    return NULL;
  }

  double x, y, z;
  if (python_vectorval(val_v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in scale");
    return NULL;
  }
  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0])|| !std::isfinite(scalevec[1])|| !std::isfinite(scalevec[2])) {
//      LOG(message_group::Warning, instance->location(), parameters.documentRoot(), "scale(%1$s)", parameters["v"].toEchoStringNoThrow());
    }
  }
  node->matrix.scale(scalevec);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_scale_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_scale(self, new_args, kwargs);
  return result;
}


PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(instance, "rotate");

  char *kwlist[] = {"obj", "a", "v", NULL};

  PyObject *val_a = NULL;
  PyObject *obj = NULL;
  float *val_v = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!|f", kwlist,
                                   &obj,
                                   &PyList_Type, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing rotate");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in rotate");
    return NULL;
  }

  if (PyList_Check(val_a) && PyList_Size(val_a) > 0) {
    double sx = 0, sy = 0, sz = 0;
    double cx = 1, cy = 1, cz = 1;
    double a = 0.0;
    bool ok = true;
//    const auto& vec_a = val_a.toVector();
    switch (PyList_Size(val_a)) {
    default:
      ok &= false;
    /* fallthrough */
    case 3:
      a = PyFloat_AsDouble(PyList_GetItem(val_a, 2));
      sz = sin_degrees(a);
      cz = cos_degrees(a);
    /* fallthrough */
    case 2:
      a = PyFloat_AsDouble(PyList_GetItem(val_a, 1));
      sy = sin_degrees(a);
      cy = cos_degrees(a);
    /* fallthrough */
    case 1:
      a = PyFloat_AsDouble(PyList_GetItem(val_a, 0));
      sx = sin_degrees(a);
      cx = cos_degrees(a);
    /* fallthrough */
    case 0:
      break;

    }
    Matrix3d M;
    M << cy * cz,  cz *sx *sy - cx * sz,   cx *cz *sy + sx * sz,
      cy *sz,  cx *cz + sx * sy * sz,  -cz * sx + cx * sy * sz,
      -sy,       cy *sx,                  cx *cy;
    node->matrix.rotate(M);

  } else {
#if 0
// TODO activate this option (need better par parsing)
    double a = 0.0;
    bool aConverted = val_a.getDouble(a);
    aConverted &= !std::isinf(a) && !std::isnan(a);

    Vector3d v(0, 0, 1);
    bool vConverted = val_v.getVec3(v[0], v[1], v[2], 0.0);
    node->matrix.rotate(angle_axis_degrees(aConverted ? a : 0, v));
    if (val_v.isDefined() && !vConverted) {
      if (aConverted) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Problem converting rotate(..., v=%1$s) parameter", val_v.toEchoStringNoThrow());
      } else {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Problem converting rotate(a=%1$s, v=%2$s) parameter", val_a.toEchoStringNoThrow(), val_v.toEchoStringNoThrow());
      }
    } else if (!aConverted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Problem converting rotate(a=%1$s) parameter", val_a.toEchoStringNoThrow());
    }
#endif // if 0
  }

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_rotate_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_rotate(self, new_args, kwargs);
  return result;
}


PyObject *python_mirror(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(instance, "mirror");

  char *kwlist[] = {"obj", "v", NULL};
  double x = 1.0, y = 1.0, z = 1.0;

  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist,
                                   &obj,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing mirror");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in mirror");
    return NULL;
  }

  Vector3d mirrorvec;
  if (python_vectorval(val_v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in mirror");
    return NULL;
  }
  // x /= sqrt(x*x + y*y + z*z)
  // y /= sqrt(x*x + y*y + z*z)
  // z /= sqrt(x*x + y*y + z*z)
  if (x != 0.0 || y != 0.0 || z != 0.0) {
    // skip using sqrt to normalize the vector since each element of matrix contributes it with two multiplied terms
    // instead just divide directly within each matrix element
    // simplified calculation leads to less float errors
    double a = x * x + y * y + z * z;

    Matrix4d m;
    m << 1 - 2 * x * x / a, -2 * y * x / a, -2 * z * x / a, 0,
      -2 * x * y / a, 1 - 2 * y * y / a, -2 * z * y / a, 0,
      -2 * x * z / a, -2 * y * z / a, 1 - 2 * z * z / a, 0,
      0, 0, 0, 1;
    node->matrix = m;
  }

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_mirror_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_mirror(self, new_args, kwargs);
  return result;
}


PyObject *python_translate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(instance, "translate");

  char *kwlist[] = {"obj", "v", NULL};
  PyObject *v = NULL;
  PyObject *obj = NULL;
  double x = 0, y = 0, z = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist,
                                   &obj,
                                   &v
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing translate");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in translate");
    return NULL;
  }

  if (python_vectorval(v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in trans");
    return NULL;
  }
  Vector3d translatevec(x, y, z);
  node->matrix.translate(translatevec);
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_translate_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_translate(self, new_args, kwargs);
  return result;
}


PyObject *python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i, j;

  auto node = std::make_shared<TransformNode>(instance, "multmatrix");

  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  PyObject *element = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist,
                                   &obj,
                                   &PyList_Type, &mat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing multmatrix");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in multmatrix");
    return NULL;
  }


  if (mat != NULL) {
    Matrix4d rawmatrix{Matrix4d::Identity()};
    for (i = 0; i < std::min(size_t(PyList_Size(mat)), size_t(4)); i++) {
      element = PyList_GetItem(mat, i);
      for (j = 0; j < std::min(size_t(PyList_Size(element)), size_t(4)); j++) {
        rawmatrix(i, j) = PyFloat_AsDouble(PyList_GetItem(element, j));
      }
    }
    double w = rawmatrix(3, 3);
    if (w != 1.0) node->matrix = rawmatrix / w;
    else node->matrix = rawmatrix;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);

}

PyObject *python_multmatrix_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_multmatrix(self, new_args, kwargs);
  return result;
}

PyObject *python_output(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *object = NULL;
  char *kwlist[] = {"object", NULL};
  std::shared_ptr<AbstractNode> child;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &object
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "error duing parsing output");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(object);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in output");
    return NULL;
  }
  python_result_node = child;
  return Py_None;
}

PyObject *python_output_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_output(self, new_args, kwargs);
  return result;
}


PyObject *python__getitem__(PyObject *dict, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *) dict;
  if (self->dict == NULL) {
    printf("Dict not initialized!\n");
    return 0;
  }
  PyObject *result = PyDict_GetItem(self->dict, key);
  if (result == NULL) result = Py_None;
  else Py_INCREF(result);
  return result;
}

int python__setitem__(PyObject *dict, PyObject *key, PyObject *v)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *) dict;
  if (self->dict == NULL) {
    printf("Dict not initialized!\n");
    return 0;
  }
  Py_INCREF(v);
  PyDict_SetItem(self->dict, key, v);
  return 0;
}




PyObject *python_color(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<ColorNode>(instance);

  char *kwlist[] = {"obj", "c", "alpha", NULL};
  PyObject *obj = NULL;
  char *colorname = NULL;
  double alpha = 1.0;
  double x = 0, y = 0, z = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sd", kwlist,
                                   &obj,
                                   &colorname, &alpha
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing color");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in color");
    return NULL;
  }

  /*
     if (parameters["c"].type() == Value::Type::VECTOR) {
     const auto& vec = parameters["c"].toVector();
     for (size_t i = 0; i < 4; ++i) {
      node->color[i] = i < vec.size() ? (float)vec[i].toDouble() : 1.0f;
      if (node->color[i] > 1 || node->color[i] < 0) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "color() expects numbers between 0.0 and 1.0. Value of %1$.1f is out of range", node->color[i]);
      }
     }
     } else if (parameters["c"].type() == Value::Type::STRING) {
   */
  boost::algorithm::to_lower(colorname);
  if (webcolors.find(colorname) != webcolors.end()) {
    node->color = webcolors.at(colorname);
  } else {
    // Try parsing it as a hex color such as "#rrggbb".
    const auto hexColor = parse_hex_color(colorname);
    if (hexColor) {
      node->color = *hexColor;
    } else {
      PyErr_SetString(PyExc_TypeError, "Cannot parse color");
//        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Unable to parse color \"%1$s\"", colorname);
//        LOG(message_group::None, Location::NONE, "", "Please see https://en.wikipedia.org/wiki/Web_colors");
      return NULL;
    }
  }
  node->color[3] = alpha;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_color_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_color(self, new_args, kwargs);
  return result;
}



PyObject *python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<RotateExtrudeNode>(instance);

  PyObject *obj = NULL;

  char *layer = NULL;
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *origin = NULL;
  double fn = NAN, fa = NAN, fs = NAN;

  get_fnas(fn,fa,fs);

  char *kwlist[] = {"obj", "layer", "convexity", "scale", "fn", "fa", "fs", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|siddO!ddd", kwlist,
                                   &obj,
                                   &layer,
                                   &convexity,
                                   &scale,
                                   &angle,
                                   &PyList_Type,
                                   &origin,
                                   &fn, &fa, &fs
                                   )) {

    PyErr_SetString(PyExc_TypeError, "error duing parsing rotate_extrude");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in rotate_extrude");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  if (layer != NULL) node->layername = layer;
  node->convexity = convexity;
  node->scale = scale;
  node->angle = angle;

  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }



  if (node->convexity <= 0) node->convexity = 2;
  if (node->scale <= 0) node->scale = 1;
  if ((node->angle <= -360) || (node->angle > 360)) node->angle = 360;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_rotate_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_rotate_extrude(self, new_args, kwargs);
  return result;
}


PyObject *python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<LinearExtrudeNode>(instance);

  PyObject *obj = NULL;
  double height = 1;
  char *layer = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  double twist = 0.0;
  double fn = NAN, fa = NAN, fs = NAN;

  char *kwlist[] = {"obj", "height", "layer", "convexity", "origin", "scale", "center", "slices", "segments", "twist", "fn", "fa", "fs", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|dsiO!O!Oiidddd", kwlist,
                                   &obj,
                                   &height,
                                   &layer,
                                   &convexity,
                                   &PyList_Type,
                                   &origin,
                                   &PyList_Type,
                                   &scale,
                                   &center,
                                   &slices,
                                   &segments,
                                   &twist,
                                   &fn, &fs, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing linear_extrude");
    return NULL;
  }

  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in linear_extrude\n");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  node->height = height;
  node->convexity = convexity;
  if (layer != NULL) node->layername = layer;

  node->origin_x = 0.0; node->origin_y = 0.0;
  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }

  node->scale_x = 1.0; node->scale_y = 1.0;
  if (scale != NULL && PyList_Check(scale) && PyList_Size(scale) == 2) {
    node->scale_x = PyFloat_AsDouble(PyList_GetItem(scale, 0));
    node->scale_y = PyFloat_AsDouble(PyList_GetItem(scale, 1));
  }

  if (center == Py_True) node->center = 1;

  node->slices = slices;
  node->has_slices = slices != 1?1:0;

  node->segments = segments;
  node->has_segments = segments != 1?1:0;

  node->twist = twist;
  node->has_twist = twist != 1?1:0;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_linear_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_linear_extrude(self, new_args, kwargs);
  return result;
}

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  char *kwlist[] = { "obj", NULL };
  PyObject *objs = NULL;
  PyObject *obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyList_Type, &objs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing csg");
    return NULL;
  }
  n = PyList_Size(objs);
  for (i = 0; i < n; i++) {
    obj = PyList_GetItem(objs, i);
    child = PyOpenSCADObjectToNode(obj);
    node->children.push_back(child);
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::UNION);
}

PyObject *python_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject *python_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_sub(self, args, kwargs, OpenSCADOperator::INTERSECTION);
}

PyObject *python_nb_sub(PyObject *arg1, PyObject *arg2, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  double x, y, z;
  if (mode == OpenSCADOperator::INTERSECTION &&  !python_vectorval(arg2, &x, &y, &z)) {
    child = PyOpenSCADObjectToNodeMulti(arg1);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    auto node = std::make_shared<TransformNode>(instance, "scale");
    Vector3d scalevec(x, y, z);
    node->matrix.scale(scalevec);
    node->children.push_back(child);
    return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  }

  if(arg1 == Py_None && mode == OpenSCADOperator::UNION) return arg2;
  if(arg2 == Py_None && mode == OpenSCADOperator::UNION) return arg1;
  if(arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) return arg1;

  auto node = std::make_shared<CsgOpNode>(instance, mode);

  child = PyOpenSCADObjectToNodeMulti(arg1);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
    return NULL;
  }
  node->children.push_back(child);
  child = PyOpenSCADObjectToNodeMulti(arg2);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
    return NULL;
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::UNION); }
PyObject *python_nb_substract(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::DIFFERENCE); }
PyObject *python_nb_multiply(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::INTERSECTION); }

PyObject *python_csg_oo_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  char *kwlist[] = { "obj", NULL };
  PyObject *more_obj = NULL;
  PyObject *obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &more_obj
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing csg_oo");
    return NULL;
  }

  child = PyOpenSCADObjectToNode(self);
  node->children.push_back(child);

  if (python_more_obj(node->children, more_obj)) {
    PyErr_SetString(PyExc_TypeError, "Unsupported  argument");
    return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_union_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_oo_sub(self, args, kwargs, OpenSCADOperator::UNION);
}

PyObject *python_difference_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_oo_sub(self, args, kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject *python_intersection_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_oo_sub(self, args, kwargs, OpenSCADOperator::INTERSECTION);
}


PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
  char *kwlist[] = { "obj", NULL };
  PyObject *objs = NULL;
  PyObject *obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyList_Type, &objs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing advanced csg");
    return NULL;
  }
  n = PyList_Size(objs);
  for (i = 0; i < n; i++) {
    obj = PyList_GetItem(objs, i);
    child = PyOpenSCADObjectToNode(obj);
    node->children.push_back(child);
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  int convexity = 2;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::MINKOWSKI);
  char *kwlist[] = { "obj", "convexity", NULL };
  PyObject *objs = NULL;
  PyObject *obj;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist,
                                   &PyList_Type, &objs,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing minkowski");
    return NULL;
  }
  n = PyList_Size(objs);
  for (i = 0; i < n; i++) {
    obj = PyList_GetItem(objs, i);
    child = PyOpenSCADObjectToNode(obj);
    node->children.push_back(child);
  }
  node->convexity = convexity;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_hull(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_adv_sub(self, args, kwargs, CgalAdvType::HULL);
}


PyObject *python_fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_csg_adv_sub(self, args, kwargs, CgalAdvType::FILL);
}


PyObject *python_resize(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  int convexity = 2;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::RESIZE);
  char *kwlist[] = { "obj", "newsize", "auto", "convexity", NULL };
  PyObject *obj;
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O!O!i", kwlist,
                                   &obj,
                                   &PyList_Type, &newsize,
                                   &PyList_Type, &autosize,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing resize");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in resize");
    return NULL;
  }

  if (newsize != NULL) {
    double x, y, z;
    if (python_vectorval(newsize, &x, &y, &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid resize dimensions");
      return NULL;
    }
    node->newsize[0] = x;
    node->newsize[1] = y;
    node->newsize[2] = z;
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

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_roof(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<RoofNode>(instance);
  double fn = NAN, fa = NAN, fs = NAN;

  char *kwlist[] = {"obj", "method", "convexity", "fn", "fa", "fs", NULL};
  PyObject *obj = NULL;
  const char *method = NULL;
  int convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sdddd", kwlist,
                                   &obj,
                                   &method, convexity,
                                   &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing roof");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in roof");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

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
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_roof_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_roof(self, new_args, kwargs);
  return result;
}

PyObject *python_render(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<RenderNode>(instance);

  char *kwlist[] = {"obj", "convexity", NULL};
  PyObject *obj = NULL;
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist,
                                   &PyOpenSCADType, &obj,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing render");
    return NULL;
  }
  child = PyOpenSCADObjectToNode(obj);

  node->convexity = convexity;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_surface(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<SurfaceNode>(instance);

  char *kwlist[] = {"file", "center", "convexity", "invert", NULL};
  const char *file = NULL;
  PyObject *center = NULL;
  PyObject *invert = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OlO", kwlist,
                                   &file, &center, &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing surface");
    return NULL;
  }


  std::string fileval = file == NULL ? "" : file;
  std::string filename = lookup_file(fileval, instance->location().filePath().parent_path().string(), "");
  node->filename = filename;
  handle_dep(fs::path(filename).generic_string());

  if (center == Py_True) node->center = 1;
  node->convexity = 2;
  if (invert  == Py_True)  node->invert = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_text(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<TextNode>(instance);

  char *kwlist[] = {"text", "size", "font", "spacing", "direction", "language", "script", "halign", "valign", "fn", "fa", "fs", NULL};

  double size = 1.0, spacing = 1.0;
  double fn = NAN, fa = NAN, fs = NAN;

  get_fnas(fn, fa, fs);

  const char *text = "", *font = NULL, *direction = "ltr", *language = "en", *script = "latin", *valign = "baseline", *halign = "left";

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssssddd", kwlist,
                                   &text, &size, &font,
                                   &spacing, &direction, &language,
                                   &script, &valign, &halign,
                                   &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing text");
    return NULL;
  }

  node->params.set_fn(fn);
  node->params.set_fa(fa);
  node->params.set_fs(fs);
  node->params.set_size(size);
  if (text != NULL) node->params.set_text(text);
  node->params.set_spacing(spacing);
  if (font != NULL) node->params.set_font(font);
  if (direction != NULL) node->params.set_direction(direction);
  if (language != NULL) node->params.set_language(language);
  if (script != NULL) node->params.set_script(script);
  if (valign != NULL) node->params.set_halign(halign);
  if (halign != NULL) node->params.set_valign(valign);
  node->params.set_loc(instance->location());

/*
   node->params.set_documentPath(session->documentRoot());
   node->params.detect_properties();
   }
 */

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_textmetrics(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<TextNode>(instance);

  char *kwlist[] = {"text", "size", "font", "spacing", "direction", "language", "script", "halign", "valign", NULL};

  double size = 1.0, spacing = 1.0;

  const char *text = "", *font = NULL, *direction = "ltr", *language = "en", *script = "latin", *valign = "baseline", *halign = "left";

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|dsdsssss", kwlist,
                                   &text, &size, &font,
                                   &spacing, &direction, &language,
                                   &script, &valign, &halign
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing textmetrics");
    return NULL;
  }

  FreetypeRenderer::Params ftparams;

  ftparams.set_size(size);
  if (text != NULL) ftparams.set_text(text);
  ftparams.set_spacing(spacing);
  if (font != NULL) ftparams.set_font(font);
  if (direction != NULL) ftparams.set_direction(direction);
  if (language != NULL) ftparams.set_language(language);
  if (script != NULL) ftparams.set_script(script);
  if (valign != NULL) ftparams.set_halign(halign);
  if (halign != NULL) ftparams.set_valign(valign);
  ftparams.set_loc(instance->location());

  FreetypeRenderer::TextMetrics metrics(ftparams);
  if (!metrics.ok) {
    PyErr_SetString(PyExc_TypeError, "Invalid Metric");
    return NULL;
  }
  PyObject *offset = PyList_New(2);
  PyList_SetItem(offset, 0, PyFloat_FromDouble(metrics.x_offset));
  PyList_SetItem(offset, 1, PyFloat_FromDouble(metrics.y_offset));

  PyObject *advance = PyList_New(2);
  PyList_SetItem(advance, 0, PyFloat_FromDouble(metrics.advance_x));
  PyList_SetItem(advance, 1, PyFloat_FromDouble(metrics.advance_y));

  PyObject *position = PyList_New(2);
  PyList_SetItem(position, 0, PyFloat_FromDouble(metrics.bbox_x));
  PyList_SetItem(position, 1, PyFloat_FromDouble(metrics.bbox_y));

  PyObject *dims = PyList_New(2);
  PyList_SetItem(dims, 0, PyFloat_FromDouble(metrics.bbox_w));
  PyList_SetItem(dims, 1, PyFloat_FromDouble(metrics.bbox_h));

  PyObject *dict;
  dict = PyDict_New();
  PyDict_SetItemString(dict, "ascent", PyFloat_FromDouble(metrics.ascent));
  PyDict_SetItemString(dict, "descent", PyFloat_FromDouble(metrics.descent));
  PyDict_SetItemString(dict, "offset", offset);
  PyDict_SetItemString(dict, "advance", advance);
  PyDict_SetItemString(dict, "position", position);
  PyDict_SetItemString(dict, "size", dims);
  return (PyObject *)dict;
}

PyObject *python_version(PyObject *self, PyObject *args, PyObject *kwargs)
{

  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing version");
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


PyObject *python_version_num(PyObject *self, PyObject *args, PyObject *kwargs)
{

  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing version_num");
    return NULL;
  }

  double version = OPENSCAD_YEAR * 10000 + OPENSCAD_MONTH * 100;
#ifdef OPENSCAD_DAY
  version += OPENSCAD_DAY;
#endif
  return PyFloat_FromDouble(version);
}


PyObject *python_offset(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<OffsetNode>(instance);

  char *kwlist[] = {"obj", "r", "delta", "chamfer", "fn", "fa", "fs", NULL};
  PyObject *obj = NULL;
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od|dOddd", kwlist,
                                   &obj,
                                   &r, &delta, &chamfer,
                                   &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing offset");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in offset");
    return NULL;
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;


  node->delta = 1;
  node->chamfer = false;
  node->join_type = ClipperLib::jtRound;
  if (!isnan(r)) {
    node->delta = r;
  } else if (!isnan(delta)) {
    node->delta = delta;
    node->join_type = ClipperLib::jtMiter;
    if (chamfer == Py_True) {
      node->chamfer = true;
      node->join_type = ClipperLib::jtSquare;
    }
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_offset_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_offset(self, new_args, kwargs);
  return result;
}



PyObject *python_projection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<ProjectionNode>(instance);

  char *kwlist[] = {"obj", "cut", "convexity", NULL};
  PyObject *obj = NULL;
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sl", kwlist,
                                   &obj,
                                   &cutmode, &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing projection");
    return NULL;
  }
  child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in projection");
    return NULL;
  }

  node->convexity = convexity;
  node->cut_mode = 0;
  if (cutmode != NULL && !strcasecmp(cutmode, "cut")) node->cut_mode = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_projection_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *new_args = python_oo_args(self, args);
  PyObject *result = python_projection(self, new_args, kwargs);
  return result;
}


PyObject *python_group(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<GroupNode>(instance);

  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  const char *cutmode = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyOpenSCADType, &obj
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing group");
    return NULL;
  }
  child = PyOpenSCADObjectToNode(obj);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *do_import_python(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type)
{
  DECLARE_INSTANCE
  char *kwlist[] = {"file", "layer", "convexity", "origin", "scale", "width", "height", "filename", "center", "dpi", "id", NULL};
  double fn = NAN, fa = NAN, fs = NAN;

  std::string filename;
  const char *v = NULL, *layer = NULL,  *id = NULL;
  PyObject *center = NULL;
  int convexity = 2;
  double scale = 1.0, width = 1, height = 1, dpi = 1.0;
  PyObject *origin = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|slO!dddsfOddd", kwlist,
                                   &v,
                                   &layer,
                                   &convexity,
                                   &PyList_Type, origin,
                                   &scale,
                                   &width, &height,
                                   &center, &dpi, &id,
                                   &fn, &fa, &fs

                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing osimport");
    return NULL;
  }

  filename = lookup_file(v == NULL ? "" : v, instance->location().filePath().parent_path().string(), "");
  if (!filename.empty()) handle_dep(filename);
  ImportType actualtype = type;
  if (actualtype == ImportType::UNKNOWN) {
    std::string extraw = fs::path(filename).extension().generic_string();
    std::string ext = boost::algorithm::to_lower_copy(extraw);
    if (ext == ".stl") actualtype = ImportType::STL;
    else if (ext == ".off") actualtype = ImportType::OFF;
    else if (ext == ".dxf") actualtype = ImportType::DXF;
    else if (ext == ".nef3") actualtype = ImportType::NEF3;
    else if (ext == ".3mf") actualtype = ImportType::_3MF;
    else if (ext == ".amf") actualtype = ImportType::AMF;
    else if (ext == ".svg") actualtype = ImportType::SVG;
  }

  auto node = std::make_shared<ImportNode>(instance, actualtype);

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

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

  node->scale = scale;
  if (node->scale <= 0) node->scale = 1;

  node->dpi = ImportNode::SVG_DEFAULT_DPI;
  double val = dpi;
  if (val < 0.001) {
    PyErr_SetString(PyExc_TypeError, "Invalid dpi value giving");
//      std::string filePath = boostfs_uncomplete(instance->location().filePath(), "");
//      LOG(message_group::Warning, Location::NONE, "",
//          "Invalid dpi value giving, using default of %1$f dpi. Value must be positive and >= 0.001, file %2$s, import() at line %3$d",
//          origin.toEchoStringNoThrow(), filePath, filePath, inst->location().firstLine()
//          );
    return NULL;
  } else {
    node->dpi = val;
  }

  node->width = width;
  node->height = height;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_import(PyObject *self, PyObject *args, PyObject *kwargs)
{ return do_import_python(self, args, kwargs, ImportType::STL); }

