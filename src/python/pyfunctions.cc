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
#include "linalg.h"
#include "GeometryUtils.h"
#include <Python.h>
#include "pyopenscad.h"
#include "SourceFile.h"
#include "BuiltinContext.h"
extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename, const std::string& mainFile, int debug);

#include "primitives.h"
#include "TransformNode.h"
#include "RotateExtrudeNode.h"
#include "LinearExtrudeNode.h"
#include "CgalAdvNode.h"
#include "CsgOpNode.h"
#include "ColorNode.h"
#include "Expression.h"
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

#define FUNC_HEADER(name) PyObject *python_##name(PyObject *self, PyObject *args, PyObject *kwargs); \
PyObject *python_oo_##name(PyObject *self, PyObject *args, PyObject *kwargs) \
{ \
  PyObject *new_args = python_oo_args(self, args); \
  PyObject *result = python_##name(Py_None, new_args, kwargs); \
  return result; \
} \
PyObject *python_##name(PyObject *self, PyObject *args, PyObject *kwargs) 

//  Py_XDECREF(new_args); \

PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CubeNode>(instance);

  char *kwlist[] = {"size", "center", NULL};
  PyObject *size = NULL;

  double x = 1, y = 1, z = 1;
  PyObject *center = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &size,
                                   &center)){
    PyErr_SetString(PyExc_TypeError, "Error during parsing cube(size)");
    return NULL;
  }	  

  if (size != NULL) {
    double x, y, z;
    if (python_vectorval(size, &(node->x), &(node->y), &(node->z))) {
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
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing sphere(r|d)");
    return NULL;
  } 
  if (!isnan(r)) {
    if(r <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter r must be positive");
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
      PyErr_SetString(PyExc_TypeError, "Parameter d must be positive");
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing cylinder(h,r|r1+r2|d1+d2)");
    return NULL;
  }

  if(h <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder height must be positive");
    return NULL;
  }
  vh = h;

  if(!isnan(r) && r <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r must be positive");
    return NULL;
  }
  if(!isnan(d) && d <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r must be positive");
    return NULL;
  }
  if(!isnan(r1) && r1 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r1 must not be negative");
    return NULL;
  }
  if(!isnan(r2) && r2 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder r2 must not be negative");
    return NULL;
  }
  if(!isnan(d1) && d1 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d1 must not be negative");
    return NULL;
  }
  if(!isnan(d2) && d2 < 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d2 must not be negative");
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
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }

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
  Vector3d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|iO!", kwlist,
                                   &PyList_Type, &points,
                                   &PyList_Type, &faces,
                                   &convexity,
                                   &PyList_Type, &triangles
                                   )) {
	  PyErr_SetString(PyExc_TypeError, "Error during parsing polyhedron(points, faces)");
	  return NULL;
  } 

  if (points != NULL && PyList_Check(points)) {
    if(PyList_Size(points) == 0) {
      PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polyhedron");
      return NULL;
    }
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 3) {
        point[0] = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point[1] = PyFloat_AsDouble(PyList_GetItem(element, 1));
        point[2] = PyFloat_AsDouble(PyList_GetItem(element, 2));
        node->points.push_back(point);
      } else {
        PyErr_SetString(PyExc_TypeError, "Coordinate must exactly contain 3 numbers");
        return NULL;
      }

    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polyhedron Points must be a list of coordinates");
    return NULL;
  }

  if (triangles != NULL) {
    faces = triangles;
//	LOG(message_group::Deprecated, inst->location(), parameters.documentRoot(), "polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
  }

  if (faces != NULL && PyList_Check(faces) ) {
    if(PyList_Size(faces) == 0) {
      PyErr_SetString(PyExc_TypeError, "must specify at least 1 face");
      return NULL;
    }
    for (i = 0; i < PyList_Size(faces); i++) {
      element = PyList_GetItem(faces, i);
      if (PyList_Check(element)) {
        IndexedFace face;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
	  if(pointIndex < 0 || pointIndex >= node->points.size()) {
    		PyErr_SetString(PyExc_TypeError, "Polyhedron Point Index out of range");
		    return NULL;
	  }
          face.push_back(pointIndex);
        }
        if (face.size() >= 3) {
          node->faces.push_back(std::move(face));
        } else {
    	  PyErr_SetString(PyExc_TypeError, "Polyhedron Face must sepcify at least 3 indices");
  	  return NULL;
	}

      } else {
    	PyErr_SetString(PyExc_TypeError, "Polyhedron Face must be a list of indices");
	return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polyhedron faces must be a list of indices");
    return NULL;
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
  double z=NAN;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &dim,
                                   &center)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing square(dim)");
    return NULL;
  }
  if (dim != NULL) {
    if (python_vectorval(dim, &(node->x), &(node->y), &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid Square dimensions");
      return NULL;
    }
    if(!isnan(z)) {
      PyErr_SetString(PyExc_TypeError, "Cannot specify z coordinate for square");
      return NULL;
    }
  }
  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }

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
    PyErr_SetString(PyExc_TypeError, "Error during parsing circle(r|d)");
    return NULL;
  }

  if (!isnan(r)) {
    if(r <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter r must be positive");
      return NULL;
    }	    
    vr = r;
    if(!isnan(d)) {
      PyErr_SetString(PyExc_TypeError, "Cant specify r and d at the same time for circle");
      return NULL;
    }
  } 
  if (!isnan(d)) {
    if(d <= 0) {
      PyErr_SetString(PyExc_TypeError, "Parameter d must be positive");
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
  Vector2d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!i", kwlist,
                                   &PyList_Type, &points,
                                   &PyList_Type, &paths,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing polygon(points,paths)");
    return NULL;
  }

  if (points != NULL && PyList_Check(points) ) {
    if(PyList_Size(points) == 0) {
      PyErr_SetString(PyExc_TypeError, "There must at least be one point in the polygon");
      return NULL;
    }
    for (i = 0; i < PyList_Size(points); i++) {
      element = PyList_GetItem(points, i);
      if (PyList_Check(element) && PyList_Size(element) == 2) {
        point[0] = PyFloat_AsDouble(PyList_GetItem(element, 0));
        point[1] = PyFloat_AsDouble(PyList_GetItem(element, 1));
        node->points.push_back(point);
      } else {
        PyErr_SetString(PyExc_TypeError, "Coordinate must exactly contain 2 numbers");
        return NULL;
      }

    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Polygon points must be a list of coordinates");
    return NULL;
  }

  if (paths != NULL && PyList_Check(paths) ) {
    if(PyList_Size(paths) == 0) {
      PyErr_SetString(PyExc_TypeError, "must specify at least 1 path when specified");
      return NULL;
    }
    for (i = 0; i < PyList_Size(paths); i++) {
      element = PyList_GetItem(paths, i);
      if (PyList_Check(element)) {
        std::vector<size_t> path;
        for (j = 0; j < PyList_Size(element); j++) {
          pointIndex = PyLong_AsLong(PyList_GetItem(element, j));
	  if(pointIndex < 0 || pointIndex >= node->points.size()) {
    		PyErr_SetString(PyExc_TypeError, "Polyhedron Point Index out of range");
		    return NULL;
	  }
          path.push_back(pointIndex);
        }
        node->paths.push_back(std::move(path));
      } else {
    	PyErr_SetString(PyExc_TypeError, "Polygon path must be a list of indices");
	return NULL;
      }
    }
  }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_scale_core(PyObject *obj, PyObject *val_v)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<TransformNode>(instance, "scale");
  PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(obj,&dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in scale");
    return NULL;
  }

  double x, y, z;
  if (python_vectorval(val_v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in scale, use 1 to 3 ordinates.");
    return NULL;
  }
  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0])|| !std::isfinite(scalevec[1])|| !std::isfinite(scalevec[2])) {
    }
  }
  node->matrix.scale(scalevec);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_scale(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist,
                                   &obj,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj,val_v);
}

PyObject *python_oo_scale(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj,val_v);
}

int python_tomatrix(PyObject *pyt, Matrix4d &mat)
{
  if(pyt == nullptr) return 1;
  PyObject *row, *cell;
  double val;
  if(!PyList_Check(pyt)) return 1;
  if(PyList_Size(pyt) != 4) return 1;
  for(int i=0;i<4;i++) {
    row=PyList_GetItem(pyt,i);
    if(!PyList_Check(row)) return 1;
    if(PyList_Size(row) != 4) return 1;
    for(int j=0;j<4;j++) {
      cell=PyList_GetItem(row,j);
      if(python_numberval(cell,&val)) return 1;
      mat(i,j)=val;
    }
  }
  return 0;
}
PyObject *python_frommatrix(const Matrix4d &mat) {
  PyObject *pyo=PyList_New(4);
  PyObject *row;
  for(int i=0;i<4;i++) {
    row=PyList_New(4);
    for(int j=0;j<4;j++)
      PyList_SetItem(row,j,PyFloat_FromDouble(mat(i,j)));
    PyList_SetItem(pyo,i,row);
//      Py_XDECREF(row);
  }
  return pyo;
}

PyObject *python_rotate_sub(PyObject *obj, Vector3d rot)
{
  PyObject *child_dict;
  double sx = 0, sy = 0, sz = 0;
  double cx = 1, cy = 1, cz = 1;
  double a = 0.0;
  bool ok = true;
//    const auto& vec_a = val_a.toVector();
  if(rot[2] != 0) {
    a = rot[2];
    sz = sin_degrees(a);
    cz = cos_degrees(a);
  }
  if(rot[1] != 0) {
    a = rot[1];
    sy = sin_degrees(a);
    cy = cos_degrees(a);
  }
  if(rot[0] != 0) {
    a = rot[0];
    sx = sin_degrees(a);
    cx = cos_degrees(a);
  }

  Matrix3d M;
  M << cy * cz,  cz *sx *sy - cx * sz,   cx *cz *sy + sx * sz,
    cy *sz,  cx *cz + sx * sy * sz,  -cz * sx + cx * sy * sz,
    -sy,       cy *sx,                  cx *cy;
  
  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "rotate");

  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in rotate");
    return NULL;
  }
  node->matrix.rotate(M);

  node->children.push_back(child);
  PyObject *pyresult =  PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
}

PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, float val_v)
{
  Vector3d rot(0,0,0);
  if (PyList_Check(val_a) && PyList_Size(val_a) > 0) {
    if(PyList_Size(val_a) >= 1) rot[0]= PyFloat_AsDouble(PyList_GetItem(val_a, 0));
    if(PyList_Size(val_a) >= 2) rot[1]= PyFloat_AsDouble(PyList_GetItem(val_a, 1));
    if(PyList_Size(val_a) >= 3) rot[2]= PyFloat_AsDouble(PyList_GetItem(val_a, 2));
    return python_rotate_sub(obj, rot);
  }
  return obj;
}

PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "a", "v", NULL};
  PyObject *val_a = NULL;
  PyObject *obj = NULL;
  float val_v = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!|f", kwlist,
                                   &obj,
                                   &PyList_Type, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_oo_rotate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"a", "v", NULL};
  PyObject *val_a = NULL;
  float val_v = 0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|f", kwlist,
                                   &PyList_Type, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_mirror_core(PyObject *obj, PyObject *val_v)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "mirror");
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in mirror");
    return NULL;
  }

  Vector3d mirrorvec;
  double x = 1.0, y = 1.0, z = 1.0;
  if (python_vectorval(val_v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in mirror");
    return NULL;
  }
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

PyObject *python_mirror(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};

  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist,
                                   &obj,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_oo_mirror(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};

  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &obj,
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_translate_sub(PyObject *obj, Vector3d translatevec)
{
  PyObject *child_dict;

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "translate");
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in translate");
    return NULL;
  }
  node->matrix.translate(translatevec);

  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) { // TODO dies ueberall
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
}

PyObject *python_translate_core(PyObject *obj, PyObject *v) 
{
  double x = 0, y = 0, z = 0;
  if (python_vectorval(v, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in trans");
    return NULL;
  }
  Vector3d translatevec(x, y, z);
  return python_translate_sub(obj, translatevec);
}	

PyObject *python_translate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_translate_core(obj,v);
}

PyObject *python_oo_translate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_translate_core(obj,v);
}



PyObject *python_multmatrix_sub(PyObject *pyobj, PyObject *pymat)
{
  int i, j;

  PyObject *element = NULL;
  Matrix4d mat;
  if(!python_tomatrix(pymat, mat)) {
    double w = mat(3, 3);
    if (w != 1.0) mat = mat / w;
  } else {
    PyErr_SetString(PyExc_TypeError, "Matrix vector should be 4x4 array");
    return NULL;
  }

  Matrix4d objmat;
  if(!python_tomatrix(pyobj, objmat)){
    objmat = mat * objmat;
    return python_frommatrix(objmat);
  } 

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "multmatrix");
  std::shared_ptr<AbstractNode> child;
  PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(pyobj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in multmatrix");
    return NULL;
  }
 

  node->matrix = mat;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);

}

PyObject *python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist,
                                   &obj,
                                   &PyList_Type, &mat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat);
}

PyObject *python_oo_multmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyList_Type, &mat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat);
}



PyObject *python_output_core(PyObject *obj)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in output");
    return NULL;
  }
  python_result_node = child;
  return Py_None;
}

PyObject *python_output(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *kwlist[] = {"obj", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &obj
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(obj)");
    return NULL;
  }
  return python_output_core(obj);
}

PyObject *python_oo_output(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(obj)");
    return NULL;
  }
  return python_output_core(obj);
}

PyObject *python__getitem__(PyObject *dict, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *) dict;
  if (self->dict == NULL) {
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
    return 0;
  }
  Py_INCREF(v);
  PyDict_SetItem(self->dict, key, v);
  return 0;
}


PyObject *python_color_core(PyObject *obj, char *colorname, double alpha)
{
  PyObject *dummydict;
  double x = 0, y = 0, z = 0;
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in color");
    return NULL;
  }
  DECLARE_INSTANCE
  auto node = std::make_shared<ColorNode>(instance);

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
      return NULL;
    }
  }
  node->color[3] = alpha;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_color(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "c", "alpha", NULL};
  PyObject *obj = NULL;
  char *colorname = NULL;
  double alpha = 1.0;
  int textureind=-1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sd", kwlist,
                                   &obj,
                                   &colorname, &alpha
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing color");
    return NULL;
  }
  return python_color_core(obj, colorname, alpha);
}

PyObject *python_oo_color(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"c", "alpha", "texture",NULL};
  char *colorname = NULL;
  double alpha = 1.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sd", kwlist,
                                   &colorname, &alpha
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing color");
    return NULL;
  }
  return python_color_core(obj, colorname, alpha);
}


PyObject *rotate_extrude_core(PyObject *obj, char *layer, int convexity, double scale, double angle, PyObject *twist, PyObject *origin, PyObject *offset, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RotateExtrudeNode>(instance);
    PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if(child == NULL) {
    PyErr_SetString(PyExc_TypeError,"Invalid type for  Object in rotate_extrude\n");
    return NULL;
    node->children.push_back(child);
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

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *layer = NULL;
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist=NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  get_fnas(fn,fa,fs);
  char *kwlist[] = {"obj", "layer", "convexity", "scale", "angle", "twist", "origin", "offset", "fn", "fa", "fs", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|siddOOOddd", kwlist, 
                          &obj, &layer, &convexity, &scale, &angle, &twist, &origin, &offset, &fn,&fa,&fs))
    {

    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return rotate_extrude_core(obj, layer, convexity, scale, angle, twist, origin, offset, fn, fa,fs);
}

PyObject *python_oo_rotate_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *layer = NULL;
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist=NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  get_fnas(fn,fa,fs);
  char *kwlist[] = {"layer", "convexity", "scale", "angle", "twist", "origin", "offset", "fn", "fa", "fs", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|siddOOOddd", kwlist, 
			  &layer, &convexity, &scale, &angle, &twist, &origin, &offset, &fn,&fa,&fs))
   {

    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return rotate_extrude_core(obj, layer, convexity, scale, angle, twist, origin, offset, fn, fa,fs);
}

PyObject *linear_extrude_core(PyObject *obj, double height, char *layer, int convexity, PyObject *origin, PyObject *scale, PyObject *center, int slices, int segments, double twist, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<LinearExtrudeNode>(instance);

  PyObject *dummydict;	  
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in linear_extrude\n");
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
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }

  node->slices = slices;
  node->has_slices = slices != 1?1:0;

  node->segments = segments;
  node->has_segments = segments != 1?1:0;

  node->twist = twist;
  node->has_twist = twist != 1?1:0;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  double height = 1;
  char *layer = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  double twist = 0;
  double fn = NAN, fa = NAN, fs = NAN;

  char * kwlist[] ={"obj","height","layer","convexity","origin","scale","center","slices","segments","twist","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|dsiOOOiiiddd", kwlist, 
                                   &obj, &height, &layer, &convexity, &origin, &scale, &center, &slices, &segments, &twist, &fn, &fs, &fs))
   {
    PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj,height, layer, convexity, origin, scale, center, slices,segments,twist,fn,fa,fs);
}

PyObject *python_oo_linear_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double height = 1;
  char *layer = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  double twist = 0;
  double fn = NAN, fa = NAN, fs = NAN;

  char * kwlist[] ={"height","layer","convexity","origin","scale","center","slices","segments","twist","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dsiOOOiiiddd", kwlist, 
                                  &height, &layer, &convexity, &origin, &scale, &center, &slices, &segments, &twist, &fn, &fs, &fs))
   {
    PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj,height, layer, convexity, origin, scale, center, slices,segments,twist,fn,fa,fs);
}

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  int i;
  int n;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  char *kwlist[] = { "obj", NULL };
  PyObject *objs = NULL;
  PyObject *obj;
  PyObject *child_dict;	  
  PyObject *dummy_dict;	  
  std::shared_ptr<AbstractNode> child;
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    if(i == 0) child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
    else child = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    if(child != NULL) {
      node->children.push_back(child);
    } else {
      switch(mode) {
        case OpenSCADOperator::UNION:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing union. arguments must be solids or arrays.");
  	break;
        case OpenSCADOperator::DIFFERENCE:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing difference. arguments must be solids or arrays.");
  	break;
        case OpenSCADOperator::INTERSECTION:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing intersection. arguments must be solids or arrays.");
	  break;
      }
      return NULL;
    }
  }

  PyObject *pyresult =PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr ) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict, key, value);
    }
  }
  return pyresult;
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
  PyObject *child_dict;	  
  PyObject *dummy_dict;	  

  double x, y, z;
  if(arg1 == Py_None && mode == OpenSCADOperator::UNION) return arg2;
  if(arg2 == Py_None && mode == OpenSCADOperator::UNION) return arg1;
  if(arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) return arg1;

  auto node = std::make_shared<CsgOpNode>(instance, mode);

  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
    return NULL;
  }
  node->children.push_back(child);
  child = PyOpenSCADObjectToNodeMulti(arg2, &dummy_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
    return NULL;
  }
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }

  return pyresult;
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode) // 0: translate, 1: scale
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  PyObject *dummydict;	  

  double x, y, z;
  if (!python_vectorval(arg2, &x, &y, &z)) {
    child = PyOpenSCADObjectToNodeMulti(arg1, &dummydict);
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    if(mode == 0) {
	    auto node = std::make_shared<TransformNode>(instance, "translate");
	    Vector3d transvec(x, y, z);
	    node->matrix.translate(transvec);
	    node->children.push_back(child);
	    return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
    }
    if(mode == 1) {
	    auto node = std::make_shared<TransformNode>(instance, "scale"); 
	    Vector3d scalevec(x, y, z);
	    node->matrix.scale(scalevec);
	    node->children.push_back(child);
	    return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
    }
  }
  PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
  return NULL;
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 0); }  // translate
PyObject *python_nb_mul(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 1); } // scale
PyObject *python_nb_or(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::UNION); }
PyObject *python_nb_andnot(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::DIFFERENCE); }
PyObject *python_nb_and(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::INTERSECTION); }






PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;
  PyObject *dummydict;	  

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
  char *kwlist[] = { "obj", NULL };
  PyObject *objs = NULL;
  PyObject *obj;
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if(child != NULL) {
      node->children.push_back(child);
    } else {
      switch(mode) {
        case CgalAdvType::HULL:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing hull. arguments must be solids or arrays.");
  	  break;
        case CgalAdvType::FILL:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing fill. arguments must be solids or arrays.");
	  break;
      }
      return NULL;
    }
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
  PyObject *dummydict;	  

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist,
                                   &PyList_Type, &objs,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing minkowski(object)");
    return NULL;
  }
  n = PyList_Size(objs);
  for (i = 0; i < n; i++) {
    obj = PyList_GetItem(objs, i);
    if (Py_TYPE(obj) == &PyOpenSCADType) {
     child = PyOpenSCADObjectToNode(obj, &dummydict);
     node->children.push_back(child);
    } else {
      PyErr_SetString(PyExc_TypeError, "minkowski input data must be shapes");
      return NULL;
    }
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

PyObject *python_resize_core(PyObject *obj, PyObject *newsize, PyObject *autosize, int convexity)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::RESIZE);
  PyObject *dummydict;	  
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in resize");
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

  node->children.push_back(child);
  node->convexity = convexity;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_resize(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = { "obj", "newsize", "auto", "convexity", NULL };
  PyObject *obj;
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;
  int convexity = 2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O!O!i", kwlist,
                                   &obj,
                                   &PyList_Type, &newsize,
                                   &PyList_Type, &autosize,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing resize(object,vec3)");
    return NULL;
  }
  return python_resize_core(obj, newsize, autosize, convexity);
}  

PyObject *python_oo_resize(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"newsize", "auto", "convexity", NULL };
  PyObject *newsize = NULL;
  PyObject *autosize = NULL;
  int convexity = 2;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!O!i", kwlist,
                                   &PyList_Type, &newsize,
                                   &PyList_Type, &autosize,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing resize(object,vec3)");
    return NULL;
  }
  return python_resize_core(obj, newsize, autosize, convexity);
}  

PyObject *python_roof_core(PyObject *obj, const char *method, int convexity,double fn,double  fa,double fs)
{	
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RoofNode>(instance);
  PyObject *dummydict;	  
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in roof");
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
      node->method = "voronoi";
    }
  }

  double tmp_convexity = convexity;
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_roof(PyObject *self, PyObject *args, PyObject *kwargs)
{
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, fn, fa, fs);
}

PyObject *python_oo_roof(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double fn = NAN, fa = NAN, fs = NAN;
  char *kwlist[] = {"method", "convexity", "fn", "fa", "fs", NULL};
  const char *method = NULL;
  int convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sdddd", kwlist,
                                   &method, convexity,
                                   &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing roof(object)");
    return NULL;
  }
  return python_roof_core(obj, method, convexity, fn, fa, fs);
}

PyObject *python_render_core(PyObject *obj, int convexity)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<RenderNode>(instance);

  PyObject *dummydict;	  
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj, &dummydict);
  node->convexity = convexity;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_render(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "convexity", NULL};
  PyObject *obj = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i", kwlist,
                                   &PyOpenSCADType, &obj,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_oo_render(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"convexity", NULL};
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist,
                                   &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing render(object)");
    return NULL;
  }
  return python_render_core(obj, convexity);
}

PyObject *python_surface_core(const char *file, PyObject *center, PyObject *invert, int convexity)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<SurfaceNode>(instance);

  std::string fileval = file == NULL ? "" : file;
  std::string filename = lookup_file(fileval, instance->location().filePath().parent_path().string(), "");
  node->filename = filename;
  handle_dep(fs::path(filename).generic_string());

  if (center == Py_True) node->center = 1;
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }
  node->convexity = 2;
  if (invert  == Py_True)  node->invert = 1;
  else if (center == Py_False || center == NULL )  node->center = 0;
  else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for invert parameter");
      return NULL;
  }

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_surface(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"file", "center", "convexity", "invert", NULL};
  const char *file = NULL;
  PyObject *center = NULL;
  PyObject *invert = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OlO", kwlist,
                                   &file, &center, &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing surface(object)");
    return NULL;
  }

  return python_surface_core(file, center, invert, convexity);
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing text(string, ...))");
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing textmetrics");
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


PyObject *python_version_num(PyObject *self, PyObject *args, PyObject *kwargs)
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


PyObject *python_offset_core(PyObject *obj,double r, double delta, PyObject *chamfer, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<OffsetNode>(instance);

  PyObject *dummydict;	  
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in offset");
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
    else if (chamfer == Py_False || chamfer == NULL )  node->chamfer = 0;
    else {
        PyErr_SetString(PyExc_TypeError, "Unknown Value for chamfer parameter");
        return NULL;
    }
  }
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_offset(PyObject *self, PyObject *args, PyObject *kwargs)
{
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
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r)");
    return NULL;
  }
  return python_offset_core(obj,r, delta, chamfer, fn, fa, fs);
}

PyObject *python_oo_offset(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"r", "delta", "chamfer", "fn", "fa", "fs", NULL};
  double r = NAN, delta = NAN;
  PyObject *chamfer = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|dOddd", kwlist,
                                   &r, &delta, &chamfer,
                                   &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing offset(object,r)");
    return NULL;
  }
  return python_offset_core(obj,r, delta, chamfer, fn, fa, fs);
}

PyObject *python_projection_core(PyObject *obj, const char *cutmode, int convexity)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<ProjectionNode>(instance);
  PyObject *dummydict;	  
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in projection");
    return NULL;
  }

  node->convexity = convexity;
  node->cut_mode = 0;
  if (cutmode != NULL && !strcasecmp(cutmode, "cut")) node->cut_mode = 1;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_projection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "cut", "convexity", NULL};
  PyObject *obj = NULL;
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|sl", kwlist,
                                   &obj,
                                   &cutmode, &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_oo_projection(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"cut", "convexity", NULL};
  const char *cutmode = NULL;
  long convexity = 2;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sl", kwlist,
                                   &cutmode, &convexity
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing projection(object)");
    return NULL;
  }
  return python_projection_core(obj, cutmode, convexity);
}

PyObject *python_group(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<GroupNode>(instance);

  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  PyObject *dummydict;	  
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyOpenSCADType, &obj
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  child = PyOpenSCADObjectToNode(obj, &dummydict);

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
    PyErr_SetString(PyExc_TypeError, "Error during parsing osimport(filename)");
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
    return NULL;
  } else {
    node->dpi = val;
  }

  node->width = width;
  node->height = height;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_import(PyObject *self, PyObject *args, PyObject *kwargs) {
  return do_import_python(self, args, kwargs, ImportType::STL);
}

PyObject *python_str(PyObject *self) {
	char str[40];
  PyObject *dummydict;	  
	auto node=PyOpenSCADObjectToNode(self, &dummydict);
	if(str != nullptr)
		sprintf(str,"OpenSCAD (%d)",node->index());
	else
		sprintf(str,"Invalid OpenSCAD Object");

	return PyUnicode_FromStringAndSize(str,strlen(str)+1);
}

PyObject *python_add_parameter(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type)
{
  char *kwlist[] = {"name", "default", NULL};
  char *name = NULL;
  PyObject *value = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO", kwlist,
                                   &name,
                                   &value
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing add_parameter(name,defval)");
    return NULL;
  }
  bool found = false;
  std::shared_ptr<Literal> lit;
  if(value == Py_True) {
    lit = std::make_shared<Literal>(true,Location::NONE);
    found=true;
  } else if(value == Py_False) {
    lit = std::make_shared<Literal>(false,Location::NONE);
    found=true;
  } else if(PyFloat_Check(value)) {
    lit  = std::make_shared<Literal>(PyFloat_AsDouble(value),Location::NONE);
    found=true;
  }
  else if(PyLong_Check(value)){
    lit = std::make_shared<Literal>(PyLong_AsLong(value)*1.0,Location::NONE);
    found=true;
  }
  else if(PyUnicode_Check(value)){
    PyObject* value1 = PyUnicode_AsEncodedString(value, "utf-8", "~");
    const char *value_str =  PyBytes_AS_STRING(value1);
    lit = std::make_shared<Literal>(value_str,Location::NONE);
    found=true;
  }

  if(found){
    AnnotationList annotationList;
    annotationList.push_back(Annotation("Parameter",std::make_shared<Literal>("Parameter")));
    annotationList.push_back(Annotation("Description",std::make_shared<Literal>("Description")));
    annotationList.push_back(Annotation("Group",std::make_shared<Literal>("Group")));
    auto assignment = std::make_shared<Assignment>(name,lit);
    assignment->addAnnotations(&annotationList);
    customizer_parameters.push_back(assignment);
    PyObject *value_effective = value;
    for(int i=0;i<customizer_parameters_finished.size();i++) {
      if(customizer_parameters_finished[i]->getName() == name)
      {
        auto expr = customizer_parameters_finished[i]->getExpr();
        const auto &lit=std::dynamic_pointer_cast<Literal>(expr);
        if(lit->isDouble()) value_effective=PyFloat_FromDouble(lit->toDouble());
        if(lit->isString()) value_effective=PyUnicode_FromString(lit->toString().c_str());
      }
    }
    PyObject *maindict = PyModule_GetDict(pythonMainModule);
    PyDict_SetItemString(maindict, name,value_effective);

  }
  return Py_None;
}

PyObject *python_scad(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  char *kwlist[] = {"code", NULL};
  const char *code = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist,
                                   &code
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scad(code)");
    return NULL;
  }

  SourceFile *parsed_file = NULL;
  if(!parse(parsed_file, code, "python", "python", false)) {
    PyErr_SetString(PyExc_TypeError, "Error in SCAD code");
    return Py_None;
  }

  EvaluationSession session{"python"};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> resultnode = parsed_file->instantiate(*builtin_context, &file_context);
  delete parsed_file;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,resultnode);
}

PyObject *python_debug_modifier(PyObject *arg,int mode) {
  DECLARE_INSTANCE
  PyObject *dummydict;
  auto child = PyOpenSCADObjectToNode(arg, &dummydict);
  switch(mode){
    case 0:	  instance->tag_highlight=true; break; // #
    case 1:	  instance->tag_background=true; break; // %
    case 2:	  instance->tag_root=true; break; // ! 
  }
  auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_debug_modifier_func(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyOpenSCADType, &obj
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  return python_debug_modifier(obj, mode);
}

PyObject *python_debug_modifier_func_oo(PyObject *obj, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = { NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing group(group)");
    return NULL;
  }
  return python_debug_modifier(obj, mode);
}
PyObject *python_highlight(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func(self, args, kwargs, 0); }
PyObject *python_oo_highlight(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func_oo(self, args, kwargs, 0); }
PyObject *python_background(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func(self, args, kwargs, 1); }
PyObject *python_oo_background(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func_oo(self, args, kwargs, 1); }
PyObject *python_only(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func(self, args, kwargs, 2); }
PyObject *python_oo_only(PyObject *self, PyObject *args, PyObject *kwargs)
{ return python_debug_modifier_func_oo(self, args, kwargs, 2); }

PyObject *python_nb_invert(PyObject *arg) { return python_debug_modifier(arg,0); }
PyObject *python_nb_neg(PyObject *arg) { return python_debug_modifier(arg,1); }
PyObject *python_nb_pos(PyObject *arg) { return python_debug_modifier(arg,2); }

PyMethodDef PyOpenSCADFunctions[] = {
  {"square", (PyCFunction) python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction) python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction) python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"text", (PyCFunction) python_text, METH_VARARGS | METH_KEYWORDS, "Create Text."},
  {"textmetrics", (PyCFunction) python_textmetrics, METH_VARARGS | METH_KEYWORDS, "Get textmetrics."},

  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction) python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction) python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction) python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},

  {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction) python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction) python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"mirror", (PyCFunction) python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction) python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"offset", (PyCFunction) python_offset, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
  {"roof", (PyCFunction) python_roof, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
  {"color", (PyCFunction) python_color, METH_VARARGS | METH_KEYWORDS, "Color Object."},
  {"output", (PyCFunction) python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},

  {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},

  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"hull", (PyCFunction) python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction) python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction) python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction) python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},

  {"highlight", (PyCFunction) python_highlight, METH_VARARGS | METH_KEYWORDS, "Highlight Object."},
  {"background", (PyCFunction) python_background, METH_VARARGS | METH_KEYWORDS, "Background Object."},
  {"only", (PyCFunction) python_only, METH_VARARGS | METH_KEYWORDS, "Only Object."},

  {"projection", (PyCFunction) python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction) python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},

  {"group", (PyCFunction) python_group, METH_VARARGS | METH_KEYWORDS, "Group Object."},
  {"render", (PyCFunction) python_render, METH_VARARGS | METH_KEYWORDS, "Render Object."},
  {"osimport", (PyCFunction) python_import, METH_VARARGS | METH_KEYWORDS, "Import Object."},
  {"version", (PyCFunction) python_version, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"version_num", (PyCFunction) python_version_num, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"add_parameter", (PyCFunction) python_add_parameter, METH_VARARGS | METH_KEYWORDS, "Add Parameter for Customizer."},
  {"scad", (PyCFunction) python_scad, METH_VARARGS | METH_KEYWORDS, "Source OpenSCAD code."},
  {NULL, NULL, 0, NULL}
};

#define	OO_METHOD_ENTRY(name,desc) \
  {#name, (PyCFunction) python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {

  OO_METHOD_ENTRY(translate,"Move Object")	
  OO_METHOD_ENTRY(rotate,"Rotate Object")	

  OO_METHOD_ENTRY(scale,"Scale Object")	
  OO_METHOD_ENTRY(mirror,"Mirror Object")	
  OO_METHOD_ENTRY(multmatrix,"Multmatrix Object")	

  OO_METHOD_ENTRY(offset,"Offset Object")	
  OO_METHOD_ENTRY(roof,"Roof Object")	
  OO_METHOD_ENTRY(color,"Color Object")	
  OO_METHOD_ENTRY(output,"Output Object")	

  OO_METHOD_ENTRY(linear_extrude,"Linear_extrude Object")	
  OO_METHOD_ENTRY(rotate_extrude,"Rotate_extrude Object")	

  OO_METHOD_ENTRY(resize,"Resize Object")	


  OO_METHOD_ENTRY(highlight,"Highlight Object")	
  OO_METHOD_ENTRY(background,"Background Object")	
  OO_METHOD_ENTRY(only,"Only Object")	

  OO_METHOD_ENTRY(projection,"Projection Object")	
  OO_METHOD_ENTRY(render,"Render Object")	

  {NULL, NULL, 0, NULL}
};

PyNumberMethods PyOpenSCADNumbers =
{
     python_nb_add,	//binaryfunc nb_add
     python_nb_andnot,	//binaryfunc nb_subtract
     python_nb_mul,	//binaryfunc nb_multiply
     0,			//binaryfunc nb_remainder
     0,			//binaryfunc nb_divmod
     0,			//ternaryfunc nb_power
     python_nb_neg,	//unaryfunc nb_negative
     python_nb_pos,	//unaryfunc nb_positive
     0,			//unaryfunc nb_absolute
     0,			//inquiry nb_bool
     python_nb_invert,  //unaryfunc nb_invert
     0,			//binaryfunc nb_lshift
     0,			//binaryfunc nb_rshift
     python_nb_and,	//binaryfunc nb_and 
     0,			//binaryfunc nb_xor
     python_nb_or,	//binaryfunc nb_or 
     0,			//unaryfunc nb_int
     0,			//void *nb_reserved
     0,			//unaryfunc nb_float

     0,			//binaryfunc nb_inplace_add
     0,			//binaryfunc nb_inplace_subtract
     0,			//binaryfunc nb_inplace_multiply
     0,			//binaryfunc nb_inplace_remainder
     0,			//ternaryfunc nb_inplace_power
     0,			//binaryfunc nb_inplace_lshift
     0,			//binaryfunc nb_inplace_rshift
     0,			//binaryfunc nb_inplace_and
     0,			//binaryfunc nb_inplace_xor
     0,			//binaryfunc nb_inplace_or

     0,			//binaryfunc nb_floor_divide
     0,			//binaryfunc nb_true_divide
     0,			//binaryfunc nb_inplace_floor_divide
     0,			//binaryfunc nb_inplace_true_divide

     0,			//unaryfunc nb_index

     0,			//binaryfunc nb_matrix_multiply
     0			//binaryfunc nb_inplace_matrix_multiply
};

PyMappingMethods PyOpenSCADMapping =
{
  0,
  python__getitem__,
  python__setitem__
};

