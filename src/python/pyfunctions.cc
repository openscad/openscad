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
#include "core/primitives.h"
#include "core/CsgOpNode.h"
#include "core/ColorNode.h"
#include "core/ColorUtil.h"
#include "core/TransformNode.h"
#include "core/LinearExtrudeNode.h"
#include "core/RotateExtrudeNode.h"
#include "utils/degree_trig.h"

extern std::unordered_map<std::string, Color4f> webcolors;

PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CubeNode>(instance);

  char *kwlist[] = {"size", "center", NULL};
  PyObject *size = NULL;

  PyObject *center = NULL;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &size,
                                   &center)){
    PyErr_SetString(PyExc_TypeError, "Error during parsing cube(size)");
    return NULL;
  }	  

  if (size != NULL) {
    if (python_vectorval(size, 3, 3, &(node->x), &(node->y), &(node->z))) {
      PyErr_SetString(PyExc_TypeError, "Invalid Cube dimensions");
      return NULL;
    }
  }
  if(node->x <= 0 || node->y <= 0 || node ->z <= 0) {
      PyErr_SetString(PyExc_TypeError, "Cube Dimensions must be positive");
      return NULL;
  }
  node->center = false;
  if (center == Py_False || center == NULL ) ;
  else if (center == Py_True){
    for(int i=0;i<3;i++) node->center=true;
  } else {
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
  PyObject *rp = nullptr;
  double d = NAN;
  double fn = NAN, fa = NAN, fs = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Odddd", kwlist,
                                   &rp, &d, &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing sphere(r|d)");
    return NULL;
  } 
  if(rp != nullptr) {
    python_numberval(rp, &r);
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

  char *kwlist[] = {"h", "r1", "r2", "center",  "r", "d", "d1", "d2",  "fn", "fa", "fs", NULL};
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


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dddOddddddd", kwlist, &h, &r1, &r2, &center, &r, &d, &d1, &d2, &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing cylinder(h,r|r1+r2|d1+d2)");
    return NULL;
  }

  if(h <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder height must be positive");
    return NULL;
  }
  vh = h;

  if(!isnan(d) && d <= 0) {
    PyErr_SetString(PyExc_TypeError, "Cylinder d must be positive");
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
  } else if (!isnan(r1) && isnan(r2)) {
    vr1 = r1; vr2 = r1;
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
  unsigned int i, j, pointIndex;
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

  PyObject *center = NULL;
  double z=NAN;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist,
                                   &dim,
                                   &center)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing square(dim)");
    return NULL;
  }
  if (dim != NULL) {
    if (python_vectorval(dim, 2, 2, &(node->x), &(node->y), &z)) {
      PyErr_SetString(PyExc_TypeError, "Invalid Square dimensions");
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
  double angle = NAN;
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
  unsigned int i, j, pointIndex;
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

PyObject *python_rotate_sub(PyObject *obj, Vector3d vec3, double angle)
{
  Matrix3d M;
  if(isnan(angle)) {	
    double sx = 0, sy = 0, sz = 0;
    double cx = 1, cy = 1, cz = 1;
    double a = 0.0;
    if(vec3[2] != 0) {
      a = vec3[2];
      sz = sin_degrees(a);
      cz = cos_degrees(a);
    }
    if(vec3[1] != 0) {
      a = vec3[1];
      sy = sin_degrees(a);
      cy = cos_degrees(a);
    }
    if(vec3[0] != 0) {
      a = vec3[0];
      sx = sin_degrees(a);
      cx = cos_degrees(a);
    }

    M << cy * cz,  cz *sx *sy - cx * sz,   cx *cz *sy + sx * sz,
      cy *sz,  cx *cz + sx * sy * sz,  -cz * sx + cx * sy * sz,
      -sy,       cy *sx,                  cx *cy;
  } else {
    M = angle_axis_degrees(angle, vec3);
  }

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "rotate");

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in rotate");
    return NULL;
  }
  node->matrix.rotate(M);

  node->children.push_back(child);
  PyObject *pyresult =  PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  return pyresult;
}

PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, PyObject *val_v)
{
  Vector3d vec3(0,0,0);
  double angle;
  if (PyList_Check(val_a) && val_v == nullptr) {
    if(PyList_Size(val_a) >= 1) vec3[0]= PyFloat_AsDouble(PyList_GetItem(val_a, 0));
    if(PyList_Size(val_a) >= 2) vec3[1]= PyFloat_AsDouble(PyList_GetItem(val_a, 1));
    if(PyList_Size(val_a) >= 3) vec3[2]= PyFloat_AsDouble(PyList_GetItem(val_a, 2));
    return python_rotate_sub(obj, vec3, NAN);
  } else if (!python_numberval(val_a,&angle) && PyList_Check(val_v) && PyList_Size(val_v) == 3) {
    vec3[0]= PyFloat_AsDouble(PyList_GetItem(val_v, 0));
    vec3[1]= PyFloat_AsDouble(PyList_GetItem(val_v, 1));
    vec3[2]= PyFloat_AsDouble(PyList_GetItem(val_v, 2));
    return python_rotate_sub(obj, vec3, angle);
  }
  return obj;
}

PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "a", "v", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  PyObject *obj = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O", kwlist, &obj, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_oo_rotate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"a", "v", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &val_a, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v);
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode);
PyObject *python_translate_core(PyObject *obj, PyObject *v) 
{
  if(v == nullptr) return obj;
  return  python_nb_sub_vec3(obj, v, 0);
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

PyObject *python_show_core(PyObject *obj)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in show");
    return NULL;
  }
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  python_result_node = child;
  return Py_None;
}

PyObject *python_show(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *kwlist[] = {"obj", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist,
                                   &obj
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_show_core(obj);
}

PyObject *python_oo_show(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_show_core(obj);
}

PyObject *python__getitem__(PyObject *obj, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *) obj;
  if (self->dict == nullptr) {
    return nullptr;
  }
  PyObject *result = PyDict_GetItem(self->dict, key);
  if (result == NULL){
    PyObject *dummy_dict;
    std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    PyObject* keyname = PyUnicode_AsEncodedString(key, "utf-8", "~");
    std::string keystr = PyBytes_AS_STRING(keyname);
    result = Py_None;
  }
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


PyObject *python_color_core(PyObject *obj, PyObject *color, double alpha)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj,&child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in color");
    return NULL;
  }
  DECLARE_INSTANCE
  auto node = std::make_shared<ColorNode>(instance);

  Vector4d col(0,0,0,alpha);
  if(!python_vectorval(color, 3, 4, &col[0], &col[1], &col[2], &col[3])) {
	  for(int i=0;i<4;i++) node->color[i] = col[i];
  }
  else if(PyUnicode_Check(color)) {
    PyObject* value = PyUnicode_AsEncodedString(color, "utf-8", "~");
    char *colorname =  PyBytes_AS_STRING(value);
    boost::algorithm::to_lower(colorname);
    if (webcolors.find(colorname) != webcolors.end()) {
      node->color = webcolors.at(colorname);
      node->color[3]=alpha;
    } else {
    // Try parsing it as a hex color such as "#rrggbb".
      const auto hexColor = OpenSCAD::parse_hex_color(colorname);
      if (hexColor) {
        node->color = *hexColor;
      } else {
	printf("Error created\n");
        PyErr_SetString(PyExc_TypeError, "Cannot parse color");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown color representation");
    return nullptr;
  }
	
  node->children.push_back(child);

  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr ) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict, key, value);
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
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Od", kwlist,
                                   &obj,
                                   &color, &alpha
                                   )) {
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
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Od", kwlist,
                                   &color, &alpha
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha);
}

PyObject *rotate_extrude_core(PyObject *obj,  int convexity, double scale, double angle, PyObject *twist, PyObject *origin, PyObject *offset, PyObject *vp, char *method, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RotateExtrudeNode>(instance);
  if(1){
    PyObject *dummydict;
    child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
    if(child == NULL) {
      PyErr_SetString(PyExc_TypeError,"Invalid type for  Object in rotate_extrude\n");
      return NULL;
    }
    node->children.push_back(child);
  }

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  node->convexity = convexity;

  double dummy;
  Vector3d v(0,0,0);
  if(vp != nullptr && !python_vectorval(vp,3, 3, &v[0],&v[1],&v[2],&dummy )){
  }

  if (node->convexity <= 0) node->convexity = 2;
  if (node->angle <= -360)  node->angle = 360;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist=NULL;
  PyObject *v = NULL;
  char *method = NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  get_fnas(fn,fa,fs);
  char *kwlist[] = {"obj", "convexity", "scale", "angle", "twist", "origin", "offset", "v","method", "fn", "fa", "fs", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iddOOOOsddd", kwlist, 
                          &obj, &convexity, &scale, &angle, &twist, &origin, &offset, &v,&method, &fn,&fa,&fs))
    {

    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate_extrude(object,...)");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method, fn, fa,fs);
}

PyObject *python_oo_rotate_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  int convexity = 1;
  double scale = 1.0;
  double angle = 360.0;
  PyObject *twist=NULL;
  PyObject *origin = NULL;
  PyObject *offset = NULL;
  double fn = NAN, fa = NAN, fs = NAN;
  get_fnas(fn,fa,fs);
  PyObject *v = NULL;
  char *method = NULL;
  char *kwlist[] = {"convexity", "scale", "angle", "twist", "origin", "offset", "v","method", "fn", "fa", "fs", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iddOOOOsddd", kwlist, 
			  &convexity, &scale, &angle, &twist, &origin, &offset,&v, &method, &fn,&fa,&fs))
   {

    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return rotate_extrude_core(obj, convexity, scale, angle, twist, origin, offset, v, method, fn, fa,fs);
}

PyObject *linear_extrude_core(PyObject *obj, PyObject *height, int convexity, PyObject *origin, PyObject *scale, PyObject *center, int slices, int segments, PyObject *twist, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<LinearExtrudeNode>(instance);

  get_fnas(node->fn, node->fa, node->fs);
  if(1)
  {
  	  PyObject *dummydict;	  
	  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
	  if(child == NULL) {
        	PyErr_SetString(PyExc_TypeError,"Invalid type for  Object in linear_extrude\n");
	   	return NULL;
	  }
	  node->children.push_back(child);
   }


  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  Vector3d height_vec(0,0,0);
  double dummy;
  if(!python_numberval(height, &height_vec[2])) {
    node->height = height_vec;
  } else if(!python_vectorval(height, 3, 3, &height_vec[0], &height_vec[1], &height_vec[2], &dummy)) {
    node->height = height_vec;
  }

  node->convexity = convexity;

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

  if(twist!= NULL) {
	node->twist=PyFloat_AsDouble(twist);
	node->has_twist = 1;
  } else  node->has_twist=0;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject *python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  PyObject *height = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  PyObject *twist = NULL;
  double fn = NAN, fa = NAN, fs = NAN;

  char * kwlist[] ={"obj","height","convexity","origin","scale","center","slices","segments","twist","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OiOOOiiOddd", kwlist, 
                                   &obj, &height, &convexity, &origin, &scale, &center, &slices, &segments, &twist, &fn, &fs, &fs))
   {
    PyErr_SetString(PyExc_TypeError,"error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj,height, convexity, origin, scale, center, slices,segments,twist,fn,fa,fs);
}

PyObject *python_oo_linear_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  PyObject *height = NULL;
  int convexity = 1;
  PyObject *origin = NULL;
  PyObject *scale = NULL;
  PyObject *center = NULL;
  int slices = 1;
  int segments = 0;
  PyObject *twist = NULL;
  double fn = NAN, fa = NAN, fs = NAN;

  char * kwlist[] ={"height","convexity","origin","scale","center","slices","segments","twist","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OiOOOiiOddd", kwlist, 
                                  &height, &convexity, &origin, &scale, &center, &slices, &segments, &twist, &fn, &fs, &fs))
   {
    PyErr_SetString(PyExc_TypeError,"error during parsing\n");
    return NULL;
  }

  return linear_extrude_core(obj,height, convexity, origin, scale, center, slices,segments,twist,fn,fa,fs);
}
PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  PyObject *obj;
  PyObject *child_dict = nullptr;	  
  PyObject *dummy_dict = nullptr;	  
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
	  return nullptr;
  	break;
        case OpenSCADOperator::DIFFERENCE:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing difference. arguments must be solids or arrays.");
	  return nullptr;
  	break;
        case OpenSCADOperator::INTERSECTION:	    
          PyErr_SetString(PyExc_TypeError, "Error during parsing intersection. arguments must be solids or arrays.");
	  return nullptr;
	  break;
        case OpenSCADOperator::MINKOWSKI:	    
      	  break;
        case OpenSCADOperator::HULL:	    
      	  break;
        case OpenSCADOperator::FILL:	    
      	  break;
        case OpenSCADOperator::RESIZE:	    
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


PyObject *python_oo_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);


  PyObject *obj;
  PyObject *child_dict;	  
  PyObject *dummy_dict;	  
  std::shared_ptr<AbstractNode> child;

  child = PyOpenSCADObjectToNodeMulti(self, &child_dict);
  if(child != NULL) node->children.push_back(child);
  
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
        case OpenSCADOperator::MINKOWSKI:	    
      	  break;
        case OpenSCADOperator::HULL:	    
      	  break;
        case OpenSCADOperator::FILL:	    
      	  break;
        case OpenSCADOperator::RESIZE:	    
      	  break;
      }
      return NULL;
    }
  }

  PyObject *pyresult =PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  return pyresult;
}

PyObject *python_oo_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::UNION);
}

PyObject *python_oo_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject *python_oo_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_csg_sub(self, args, kwargs, OpenSCADOperator::INTERSECTION);
}

PyObject *python_nb_sub(PyObject *arg1, PyObject *arg2, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child[2];
  PyObject *child_dict[2];	  

  if(arg1 == Py_None && mode == OpenSCADOperator::UNION) return arg2;
  if(arg2 == Py_None && mode == OpenSCADOperator::UNION) return arg1;
  if(arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) return arg1;


  child[0] = PyOpenSCADObjectToNodeMulti(arg1, &child_dict[0]);
  if (child[0] == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
    return NULL;
  }
  child[1] = PyOpenSCADObjectToNodeMulti(arg2, &child_dict[1]);
  if (child[1] == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
    return NULL;
  }
  auto node = std::make_shared<CsgOpNode>(instance, mode);
  node->children.push_back(child[0]);
  node->children.push_back(child[1]);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  return pyresult;
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode) // 0: translate, 1: scale, 2: translateneg, 3=translate-exp
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict;	  

  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict);
  std::vector<Vector3d> vecs;
		
  vecs = python_vectors(arg2,2,3);

    if (vecs.size() > 0) {
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    std::vector<std::shared_ptr<TransformNode>> nodes;
    for(size_t j=0;j<vecs.size();j++) {
      auto node = std::make_shared<TransformNode>(instance, "transform");
      if(mode == 0 || mode == 3) node->matrix.translate(vecs[j]);
      if(mode == 1) node->matrix.scale(vecs[j]);
      if(mode == 2) node->matrix.translate(-vecs[j]);
      node->children.push_back(child);
      nodes.push_back(node);
    }  
    if(nodes.size() == 1) {
      PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, nodes[0]);
      if(child_dict != nullptr ) { 
        PyObject *key, *value;
        Py_ssize_t pos = 0;
         while(PyDict_Next(child_dict, &pos, &key, &value)) {
           PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
        }
      }
      return pyresult;
    }
  }
  PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
  return NULL;
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 0); }  // translate
PyObject *python_nb_mul(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 1); } // scale
PyObject *python_nb_or(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::UNION); }
PyObject *python_nb_subtract(PyObject *arg1, PyObject *arg2)
{
  double dmy;	
  if(PyList_Check(arg2) && PyList_Size(arg2) > 0) {
    PyObject *sub = PyList_GetItem(arg2, 0);	  
    if (!python_numberval(sub, &dmy) || PyList_Check(sub)){
      return python_nb_sub_vec3(arg1, arg2, 2); 
    }
  }	  
  return python_nb_sub(arg1, arg2,  OpenSCADOperator::DIFFERENCE); // if its solid
}
PyObject *python_nb_and(PyObject *arg1, PyObject *arg2) { return python_nb_sub(arg1, arg2,  OpenSCADOperator::INTERSECTION); }
PyObject *python_str(PyObject *self) {
  std::ostringstream stream;
  PyObject *dummydict;	  
  std::shared_ptr<AbstractNode> node=PyOpenSCADObjectToNode(self, &dummydict);
  if(node != nullptr)
    stream << "OpenSCAD (" << (int) node->index() << ")";
  else
    stream << "Invalid OpenSCAD Object";

  return PyUnicode_FromStringAndSize(stream.str().c_str(),stream.str().size());
}

PyMethodDef PyOpenSCADFunctions[] = {
  {"square", (PyCFunction) python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction) python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction) python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction) python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction) python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction) python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},
  {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction) python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"color", (PyCFunction) python_color, METH_VARARGS | METH_KEYWORDS, "Color Object."},
  {"show", (PyCFunction) python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},
  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {NULL, NULL, 0, NULL}
};

#define	OO_METHOD_ENTRY(name,desc) \
  {#name, (PyCFunction) python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {
  OO_METHOD_ENTRY(translate,"Move Object")
  OO_METHOD_ENTRY(rotate,"Rotate Object")	

  OO_METHOD_ENTRY(union,"Union Object")	
  OO_METHOD_ENTRY(difference,"Difference Object")	
  OO_METHOD_ENTRY(intersection,"Intersection Object")
  OO_METHOD_ENTRY(color,"Color Object")	
  OO_METHOD_ENTRY(linear_extrude,"Linear_extrude Object")	
  OO_METHOD_ENTRY(rotate_extrude,"Rotate_extrude Object")	

  OO_METHOD_ENTRY(show,"Show Object")	
  {NULL, NULL, 0, NULL}
};

PyNumberMethods PyOpenSCADNumbers =
{
     python_nb_add,	//binaryfunc nb_add
     python_nb_subtract,//binaryfunc nb_subtract
     python_nb_mul,	//binaryfunc nb_multiply
     0,			//binaryfunc nb_remainder
     0,			//binaryfunc nb_divmod
     0,			//ternaryfunc nb_power
     0,			//unaryfunc nb_negative
     0,			//unaryfunc nb_positive
     0,			//unaryfunc nb_absolute
     0,			//inquiry nb_bool
     0,			  //unaryfunc nb_invert
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

