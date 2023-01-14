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

#include "degree_trig.h"
#include "printutils.h"

//using namespace boost::assign; // bring 'operator+=()' into scope


// Colors extracted from https://drafts.csswg.org/css-color/ on 2015-08-02
// CSS Color Module Level 4 - Editor’s Draft, 29 May 2015
extern  std::unordered_map<std::string, Color4f> webcolors;
extern boost::optional<Color4f> parse_hex_color(const std::string& hex);



PyObject* python_cube(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<CubeNode>(&todo_fix_inst);

  char * kwlist[] ={"dim", "center",NULL};
  PyObject *dim = NULL; 

  double x=1,y=1,z=1;
  char *center=NULL;
	
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!s", kwlist, 
			   &PyList_Type,&dim, 
			   &center))
   	return NULL;

   if(dim != NULL && PyList_Check(dim) && PyList_Size(dim) == 3) {
     	x=PyFloat_AsDouble(PyList_GetItem(dim, 0));
     	y=PyFloat_AsDouble(PyList_GetItem(dim, 1));
     	z=PyFloat_AsDouble(PyList_GetItem(dim, 2));
   }
   node->x=x;
   node->y=y;
   node->z=z;
   if(center != NULL)
	   if(!strcasecmp(center,"true")) node->center=1;

   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_sphere(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<SphereNode>(&todo_fix_inst);

  char * kwlist[] ={"r","d","fn","fa","fs",NULL};
  double r = -1; 
  double d = -1; 
  double fn=-1,fa=-1,fs=-1;

  double vr=1;
	
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddd", kwlist,
			   &r,&d,&fn,&fa,&fs
			   )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
   }

   else if(r>= 0)  { vr=r; }
   else if(d>= 0)  { vr=d/2.0; }

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

   node->r=vr;


   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<CylinderNode>(&todo_fix_inst);

  char * kwlist[] ={"h","r","r1","r2","d","d1","d2", "center","fn","fa","fs",NULL};
  double h = -1; 
  double r = -1; 
  double r1 = -1; 
  double r2 = -1; 
  double d = -1; 
  double d1 = -1; 
  double d2 = -1; 

  double fn=-1,fa=-1,fs=-1;

  char *center=NULL;
  double vr1=1,vr2=1,vh=1;
	
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddddds", kwlist, &h,&r,&r1,&r2,&d,&d1,&d2, &center,&fn,&fa,&fs)) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
   }

   if(h>= 0)  vh=h;

   if(r1 >= 0 && r2 >= 0 ) { vr1=r1; vr2=r2; }
   else if(d1 >= 0 && d2 >= 0 ) { vr1=d1/2.0; vr2=d2/2.0; }
   else if(r>= 0)  { vr1=r; vr2=r; }
   else if(d>= 0)  { vr1=d/2.0; vr2=d/2.0; }

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

   node->r1=vr1;
   node->r2=vr2;
   node->h=vh;

   if(center != NULL)
	   if(!strcasecmp(center,"true")) node->center=1;

   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs)
{
  int i,j,pointIndex;
  auto node = std::make_shared<PolyhedronNode>(&todo_fix_inst);

  char * kwlist[] ={"points", "faces","convexity","triangles",NULL};
  PyObject *points = NULL; 
  PyObject *faces = NULL; 
  int convexity = 2;
  PyObject *triangles = NULL; 

  PyObject *element;
  point3d point;

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|iO!", kwlist, 
			   &PyList_Type,&points,
			   &PyList_Type,&faces,
			   &convexity,
			   &PyList_Type,&triangles
			   ))
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;

  if(points != NULL && PyList_Check(points)) {
	   for(i=0;i<PyList_Size(points);i++) {
		   element = PyList_GetItem(points,i);
		   if(PyList_Check(element) && PyList_Size(element)  == 3) {
     			point.x=PyFloat_AsDouble(PyList_GetItem(element, 0));
		     	point.y=PyFloat_AsDouble(PyList_GetItem(element, 1));
		     	point.z=PyFloat_AsDouble(PyList_GetItem(element, 2));
      			node->points.push_back(point);
		   }

	   }
   }

   if(triangles != NULL)
   {
	faces=triangles;
//	LOG(message_group::Deprecated, inst->location(), parameters.documentRoot(), "polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
    }

    if(faces != NULL && PyList_Check(faces) ) {
	   for(i=0;i<PyList_Size(faces);i++) {
		   element =  PyList_GetItem(faces,i);
		   if(PyList_Check(element)) {
                   	std::vector<size_t> face;
			   for(j=0;j<PyList_Size(element);j++) {
				pointIndex=PyLong_AsLong(PyList_GetItem(element, j));
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

  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_square(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<SquareNode>(&todo_fix_inst);

  char * kwlist[] ={"dim", "center",NULL};
  PyObject *dim = NULL; 

  double x=1,y=1;
  char *center=NULL;
	
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|s", kwlist, 
			   &PyList_Type,&dim, 
			   &center)) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
   }

   if(PyList_Check(dim) && PyList_Size(dim) == 2) {
     	x=PyFloat_AsDouble(PyList_GetItem(dim, 0));
     	y=PyFloat_AsDouble(PyList_GetItem(dim, 1));
   }
   node->x=x;
   node->y=y;
   if(center != NULL)
	   if(!strcasecmp(center,"true")) node->center=1;

   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_circle(PyObject *self, PyObject *args, PyObject *kwargs)
{
  auto node = std::make_shared<CircleNode>(&todo_fix_inst);

  char * kwlist[] ={"r","d","fn","fa","fs",NULL};
  double r = -1; 
  double d = -1; 
  double fn=-1,fa=-1,fs=-1;

  double vr=1;
	
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ddddd", kwlist, &r,&d,&fn,&fa,&fs)) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
   }

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

   if(r>= 0)  { vr=r; }
   else if(d>= 0)  { vr=d/2.0; }


   node->r=vr;


   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_polygon(PyObject *self, PyObject *args, PyObject *kwargs)
{
  int i,j,pointIndex;
  auto node = std::make_shared<PolygonNode>(&todo_fix_inst);

  char * kwlist[] ={"points", "paths","convexity",NULL};
  PyObject *points = NULL; 
  PyObject *paths = NULL; 
  int convexity = 2;

  PyObject *element;
  point2d point;

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!i", kwlist, 
			   &PyList_Type,&points,
			   &PyList_Type,&paths,
			   &convexity
			   ))
   	return NULL;

   if(points != NULL && PyList_Check(points) ) {
	   for(i=0;i<PyList_Size(points);i++) {
		   element = PyList_GetItem(points,i);
		   if(PyList_Check(element) && PyList_Size(element)  == 2) {
     			point.x=PyFloat_AsDouble(PyList_GetItem(element, 0));
		     	point.y=PyFloat_AsDouble(PyList_GetItem(element, 1));
      			node->points.push_back(point);
		   }

	   }
   }

   if(paths != NULL && PyList_Check(paths) ) {
	   for(i=0;i<PyList_Size(paths);i++) {
		   element =  PyList_GetItem(paths,i);
		   if(PyList_Check(element)) {
	                   std::vector<size_t> path;
			   for(j=0;j<PyList_Size(element);j++) {
				pointIndex=PyLong_AsLong(PyList_GetItem(element, j));
       	                 path.push_back(pointIndex);
		   	}
        	         node->paths.push_back(std::move(path));
		}
	   }
   }

  node->convexity = convexity;
  if (node->convexity < 1) node->convexity = 1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}


PyObject* python_scale(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(&todo_fix_inst, "scale");

  char * kwlist[] ={"obj","v",NULL};
  double x=1.0,y=1.0,z=1.0;

  PyObject *obj=NULL;
  PyObject *val_v = NULL; 
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist, 
			  &PyOpenSCADType, &obj, 
			  &PyList_Type,&val_v )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
  }

  child = PyOpenSCADObjectToNode(obj);

  if(PyList_Check(val_v) && PyList_Size(val_v) >= 2) {
     	x=PyFloat_AsDouble(PyList_GetItem(val_v, 0));
     	y=PyFloat_AsDouble(PyList_GetItem(val_v, 1));
  }
  if(PyList_Check(val_v) && PyList_Size(val_v) >= 3) {
     	z=PyFloat_AsDouble(PyList_GetItem(val_v, 2));
   }

  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0])|| !std::isfinite(scalevec[1])|| !std::isfinite(scalevec[2])) {
//      LOG(message_group::Warning, todo_fix_inst->location(), parameters.documentRoot(), "scale(%1$s)", parameters["v"].toEchoStringNoThrow());
    }
  }
  node->matrix.scale(scalevec);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);	
}

PyObject* python_scale_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_scale(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}


PyObject* python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(&todo_fix_inst, "rotate");

  char * kwlist[] ={"obj","a","v",NULL};

  PyObject *val_a = NULL; 
  PyObject *obj=NULL;
  float *val_v=0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|f", kwlist, &PyOpenSCADType, &obj, &PyList_Type,&val_a, &val_v )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
  }

  child = PyOpenSCADObjectToNode(obj);

  if(PyList_Check(val_a) && PyList_Size(val_a) > 0) {
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
#endif
 }
   
   node->children.push_back(child);
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);	
}

PyObject* python_rotate_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_rotate(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}


PyObject* python_mirror(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(&todo_fix_inst, "mirror");

  char * kwlist[] ={"obj","v",NULL};
  double x=1.0,y=1.0,z=1.0;

  PyObject *obj=NULL;
  PyObject *val_v = NULL; 
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist, 
			  &PyOpenSCADType, &obj, 
			  &PyList_Type,&val_v )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
  }

  child = PyOpenSCADObjectToNode(obj);

  if(PyList_Check(val_v) && PyList_Size(val_v) >= 2) {
     	x=PyFloat_AsDouble(PyList_GetItem(val_v, 0));
     	y=PyFloat_AsDouble(PyList_GetItem(val_v, 1));
  }
  if(PyList_Check(val_v) && PyList_Size(val_v) >= 3) {
     	z=PyFloat_AsDouble(PyList_GetItem(val_v, 2));
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
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);	
}

PyObject* python_mirror_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_mirror(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}


PyObject* python_translate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<TransformNode>(&todo_fix_inst, "translate");

  char * kwlist[] ={"obj","v",NULL};
  PyObject *v = NULL; 
  PyObject *obj = NULL;
  double x=0,y=0,z=0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist, 
			  &PyOpenSCADType, &obj,
			  &PyList_Type, &v 
			  )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
  }
  child = PyOpenSCADObjectToNode(obj);

  if(PyList_Check(v) && PyList_Size(v) >= 2) {
     	x=PyFloat_AsDouble(PyList_GetItem(v, 0));
     	y=PyFloat_AsDouble(PyList_GetItem(v, 1));
  }
  if(PyList_Check(v) && PyList_Size(v) >= 3) {
     	z=PyFloat_AsDouble(PyList_GetItem(v, 2));
   }
   Vector3d translatevec(x, y, z);

   node->matrix.translate(translatevec);
   node->children.push_back(child);
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);	
}

PyObject* python_translate_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_translate(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}


PyObject* python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;
  int i,j;

  auto node = std::make_shared<TransformNode>(&todo_fix_inst, "multmatrix");

  char * kwlist[] ={"obj","m",NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL; 
  PyObject *element = NULL; 
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", kwlist, 
			  &PyOpenSCADType, &obj,
			  &PyList_Type, &mat 
			  )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
   	return NULL;
  }

  child = PyOpenSCADObjectToNode(obj);


  if(mat != NULL) {
    Matrix4d rawmatrix{Matrix4d::Identity()};
    for(i=0;i<std::min(size_t(PyList_Size(mat)),size_t(4));i++) {
      element = PyList_GetItem(mat,i);
      for(j=0;j<std::min(size_t(PyList_Size(element)),size_t(4));j++) {
        rawmatrix(i,j) = PyFloat_AsDouble(PyList_GetItem(element, j));
      }
    }
    double w = rawmatrix(3, 3);
    if (w != 1.0) node->matrix = rawmatrix / w;
    else node->matrix = rawmatrix;
   }
   return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);	

}

PyObject* python_multmatrix_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_multmatrix(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}








PyObject* python_output(PyObject *self, PyObject *args, PyObject *kwargs)
{
   PyObject *object=NULL;
   char * kwlist[] ={"object",NULL};
   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!", kwlist, 
			   &PyOpenSCADType,&object
			   ))
   	return NULL;
   if(object != NULL) result_node=PyOpenSCADObjectToNode(object);
   return PyLong_FromLong(55);
}

PyObject* python_output_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_output(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}



PyObject* python_color(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<ColorNode>(&todo_fix_inst);

  char * kwlist[] ={"obj","c","alpha",NULL};
  PyObject *obj = NULL;
  char *colorname=NULL;
  double alpha=1.0;
  double x=0,y=0,z=0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|sd", kwlist, 
                          &PyOpenSCADType, &obj,
			  &colorname,&alpha
                          )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
        return NULL;
  }
  child = PyOpenSCADObjectToNode(obj);

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
        PyErr_SetString(PyExc_TypeError,"Cannot parse color");
//        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Unable to parse color \"%1$s\"", colorname);
//        LOG(message_group::None, Location::NONE, "", "Please see https://en.wikipedia.org/wiki/Web_colors");
        return NULL;
      }
    }
  node->color[3]=alpha;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);       
}

PyObject* python_color_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
        PyObject *new_args=python_oo_args(self,args);
        PyObject *result = python_color(self,new_args,kwargs);
//      Py_DECREF(&new_args);
        return result;
}



PyObject* python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<RotateExtrudeNode>(&todo_fix_inst);

  PyObject *obj = NULL;

  char *layer=NULL;
  int convexity=1;
  double scale=1.0;
  double angle=360.0;
  PyObject *origin=NULL;
  double fn=-1, fa=-1,fs=-1;


  char * kwlist[] ={"obj","layer","convexity","scale","fn","fa","fs",NULL};

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|siddO!ddd", kwlist, 
                          &PyOpenSCADType,
                          &obj,
			  &layer,
			  &convexity,
			  &scale,
			  &angle,
			  &PyList_Type,
			  &origin,
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

  if(layer != NULL) node->layername = layer;
  node->convexity = convexity;
  node->scale = scale;
  node->angle = angle;

  if(origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
	  node->origin_x=PyFloat_AsDouble(PyList_GetItem(origin, 0));
	  node->origin_y=PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }



  if (node->convexity <= 0) node->convexity = 2;
  if (node->scale <= 0) node->scale = 1;
  if ((node->angle <= -360) || (node->angle > 360)) node->angle = 360;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_rotate_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_rotate_extrude(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}


PyObject* python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::shared_ptr<AbstractNode> child;

  auto node = std::make_shared<LinearExtrudeNode>(&todo_fix_inst);

  PyObject *obj = NULL;
  double height=1;
  char *layer=NULL;
  int convexity=1;
  PyObject *origin=NULL;
  PyObject *scale=NULL;
  char *center=NULL;
  int slices=1;
  int segments=0;
  double twist=0.0;
  double fn=-1, fa=-1, fs=-1;

  char * kwlist[] ={"obj","height","layer","convexity","origin","scale","center","slices","segments","twist","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|dsiO!O!siidddd", kwlist, 
                          &PyOpenSCADType,
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
			  &fn,&fs,&fs
                          )) {
        PyErr_SetString(PyExc_TypeError,"error duing parsing\n");
        return NULL;
  }

  child = PyOpenSCADObjectToNode(obj);

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

  node->height=height;
  node->convexity=convexity;
  if(layer != NULL) node->layername=layer;

  node->origin_x=0.0; node->origin_y=0.0;
  if(origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
	  node->origin_x=PyFloat_AsDouble(PyList_GetItem(origin, 0));
	  node->origin_y=PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }

  node->scale_x=1.0; node->scale_y=1.0;
  if(scale != NULL && PyList_Check(scale) && PyList_Size(scale) == 2) {
	  node->scale_x=PyFloat_AsDouble(PyList_GetItem(scale, 0));
	  node->scale_y=PyFloat_AsDouble(PyList_GetItem(scale, 1));
  }

  if(center != NULL)
           if(!strcasecmp(center,"true")) node->center=1;

  node->slices=slices;
  node->has_slices=slices != 1?1:0;

  node->segments=segments;
  node->has_segments=segments != 1?1:0;

  node->twist=twist;
  node->has_twist=twist != 1?1:0;

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject* python_linear_extrude_oo(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *new_args=python_oo_args(self,args);
	PyObject *result = python_linear_extrude(self,new_args,kwargs);
//	Py_DECREF(&new_args);
	return result;
}

PyObject* python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs,OpenSCADOperator mode)
{
  std::shared_ptr<AbstractNode> child;
  int i;
  int n;

  auto node = std::make_shared<CsgOpNode>(&todo_fix_inst, mode);
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

PyObject* python_union(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::UNION);
}

PyObject* python_difference(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::DIFFERENCE);
}

PyObject* python_intersection(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_sub(self,args,kwargs, OpenSCADOperator::INTERSECTION);
}


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


PyObject* python_hull(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_adv_sub(self,args,kwargs,CgalAdvType::HULL);
}


PyObject* python_fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
	return python_csg_adv_sub(self,args,kwargs,CgalAdvType::FILL);
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


