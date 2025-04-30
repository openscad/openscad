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
#include "export.h"
#include "GeometryUtils.h"
#include <Python.h>
#include "python/pyopenscad.h"
#include "core/primitives.h"
#include "core/CsgOpNode.h"
#include "core/ColorNode.h"
#include "core/ColorUtil.h"
#include "SourceFile.h"
#include "BuiltinContext.h"
#include <PolySetBuilder.h>
extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename, const std::string& mainFile, int debug);

#include <python/pydata.h>
#ifdef ENABLE_LIBFIVE
#include "python/FrepNode.h"
#endif
#include "GeometryUtils.h"
#include "core/TransformNode.h"
#include "core/LinearExtrudeNode.h"
#include "core/RotateExtrudeNode.h"
#include "core/PathExtrudeNode.h"
#include "core/PullNode.h"
#include "core/WrapNode.h"
#include "core/OversampleNode.h"
#include "core/DebugNode.h"
#include "core/FilletNode.h"
#include "core/SkinNode.h"
#include "core/ConcatNode.h"
#include "core/CgalAdvNode.h"
#include "Expression.h"
#include "core/RoofNode.h"
#include "core/RenderNode.h"
#include "core/SurfaceNode.h"
#include "core/TextNode.h"
#include "core/OffsetNode.h"
#include "core/TextureNode.h"
#include <hash.h>
#include "geometry/PolySetUtils.h"
#include "core/ProjectionNode.h"
#include "core/ImportNode.h"
#include "core/Tree.h"
#include "geometry/PolySet.h"
#include "geometry/GeometryEvaluator.h"
#include "utils/degree_trig.h"
#include "printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include <fstream>
#include <ostream>
#include <boost/functional/hash.hpp>
#include <ScopeContext.h>
#include "PlatformUtils.h"
#include <iostream>
#include <filesystem>

//using namespace boost::assign; // bring 'operator+=()' into scope


// Colors extracted from https://drafts.csswg.org/css-color/ on 2015-08-02
// CSS Color Module Level 4 - Editor’s Draft, 29 May 2015
extern std::unordered_map<std::string, Color4f> webcolors;

PyObject *python_edge(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<EdgeNode>(instance);

  char *kwlist[] = {"size", "center", NULL};
  double size = 1;

  PyObject *center = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dO", kwlist,
                                   &size,
                                   &center)){
    PyErr_SetString(PyExc_TypeError, "Error during parsing edge(size)");
    return NULL;
  }	  

  if(size < 0)  {
      PyErr_SetString(PyExc_TypeError, "Edge Length must be positive");
      return NULL;
  }
  node->size=size;
  if (center == Py_False || center == NULL ) ;
  else if (center == Py_True){
    node->center = true;
  }
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_marked(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<EdgeNode>(instance);

  char *kwlist[] = {"value", NULL};
  double value = 0.0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist,
                                   &value)){
    PyErr_SetString(PyExc_TypeError, "Error during parsing marked(value)");
    return NULL;
  }	  
  return PyDataObjectFromValue(&PyDataType, value);
}

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
    int flags=0;	  
    if (python_vectorval(size, 3, 3, &(node->dim[0]), &(node->dim[1]), &(node->dim[2]), nullptr, &(node->dragflags))) {
      PyErr_SetString(PyExc_TypeError, "Invalid Cube dimensions");
      return NULL;
    }
  }
  if(node->dim[0] <= 0 || node->dim[1] <= 0 || node ->dim[2] <= 0) {
      PyErr_SetString(PyExc_TypeError, "Cube Dimensions must be positive");
      return NULL;
  }
  for(int i=0;i<3;i++) node->center[i] = 1;
  if (center == Py_False || center == NULL ) ;
  else if (center == Py_True){
    for(int i=0;i<3;i++) node->center[i] = 0;
  } else if(PyUnicode_Check(center)) {
    PyObject* centerval = PyUnicode_AsEncodedString(center, "utf-8", "~");
    const char *centerstr =  PyBytes_AS_STRING(centerval);
    if(centerstr == nullptr) {
      PyErr_SetString(PyExc_TypeError, "Cannot parse center code");
      return NULL;
    }
    if(strlen(centerstr) != 3) {
      PyErr_SetString(PyExc_TypeError, "Center code must be exactly 3 characters");
      return NULL;
    }
    for(int i=0;i<3;i++) {
      switch(centerstr[i]) {
        case '|': node->center[i] = 0; break;	      
        case '0': node->center[i] = 0; break;	      
        case ' ': node->center[i] = 0; break;	      
        case '_': node->center[i] = 0; break;	      

        case '>': node->center[i] = -1; break;	      
        case ']': node->center[i] = -1; break;	      
        case ')': node->center[i] = -1; break;	      
        case '+': node->center[i] = -1; break;	      

        case '<': node->center[i] = 1; break;	      
        case '[': node->center[i] = 1; break;	      
        case '(': node->center[i] = 1; break;	      
        case '-': node->center[i] = 1; break;	      

        default:		  
          PyErr_SetString(PyExc_TypeError, "Center code chars not recognized, must be + - or 0");
	  return NULL;
      }	      
    }
  } else {
      PyErr_SetString(PyExc_TypeError, "Unknown Value for center parameter");
      return NULL;
  }
  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

Vector3d sphereCalcIndInt(PyObject *func, Vector3d dir)
{
  dir.normalize();
  PyObject *dir_p= PyList_New(3);
  for(int i=0;i<3;i++)
    PyList_SetItem(dir_p,i,PyFloat_FromDouble(dir[i]));
  PyObject* args = PyTuple_Pack(1,dir_p);
  PyObject* len_p = PyObject_CallObject(func, args);
  double len=0;
  if(len_p == nullptr) {
	  return dir;
  }
  python_numberval(len_p, &len);
  return dir * len;
}

int sphereCalcInd(PolySetBuilder &builder, std::vector<Vector3d> &vertices, PyObject *func, Vector3d dir)
{
  dir = sphereCalcIndInt(func, dir);	
  unsigned int ind=builder.vertexIndex(dir);
  if(ind == vertices.size()) vertices.push_back(dir);
  return ind;
}

class SphereEdgeDb
{
        public:
		SphereEdgeDb(int a, int b) {
			ind1=(a<b)?a:b;
			ind2=(a>b)?a:b;
		}
                int ind1, ind2;
                int operator==(const SphereEdgeDb ref)
                {
                        if(this->ind1 == ref.ind1 && this->ind2 == ref.ind2) return 1;
                        return 0;
                }
};

unsigned int hash_value(const SphereEdgeDb& r) {
        unsigned int i;
        i=r.ind1 |(r.ind2<<16) ;
        return i;
}

int operator==(const SphereEdgeDb &t1, const SphereEdgeDb &t2) 
{
        if(t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
        return 0;
}

int sphereCalcSplitInd(PolySetBuilder &builder, std::vector<Vector3d> &vertices, std::unordered_map<SphereEdgeDb, int, boost::hash<SphereEdgeDb> > &edges, PyObject *func, int ind1, int ind2)
{
  SphereEdgeDb edge(ind1, ind2);
  if(edges.count(edge) > 0) {
    return edges[edge];
  }
  int result = sphereCalcInd(builder, vertices, func, vertices[ind1]+vertices[ind2]);
  edges[edge]=result;
  return result;
}

std::unique_ptr<const Geometry> sphereCreateFuncGeometry(void *funcptr, double fs, int n)
{
  PyObject *func = (PyObject *) funcptr;
  std::unordered_map<SphereEdgeDb, int, boost::hash<SphereEdgeDb> > edges;

  PolySetBuilder builder;
  std::vector<Vector3d> vertices;

  int topind, botind, leftind, rightind, frontind, backind;
  leftind=sphereCalcInd(builder, vertices, func, Vector3d(-1,0,0));
  rightind=sphereCalcInd(builder, vertices, func, Vector3d(1,0,0));
  frontind=sphereCalcInd(builder, vertices, func, Vector3d(0,-1,0));
  backind=sphereCalcInd(builder, vertices, func, Vector3d(0,1,0));
  botind=sphereCalcInd(builder, vertices, func, Vector3d(0,0,-1));
  topind=sphereCalcInd(builder, vertices, func, Vector3d(0,0,1));

  std::vector<IndexedTriangle> triangles;
  std::vector<IndexedTriangle> tri_new;
  tri_new.push_back(IndexedTriangle(leftind, frontind, topind));
  tri_new.push_back(IndexedTriangle(frontind, rightind, topind));
  tri_new.push_back(IndexedTriangle(rightind, backind, topind));
  tri_new.push_back(IndexedTriangle(backind, leftind, topind));
  tri_new.push_back(IndexedTriangle(leftind, botind, frontind));
  tri_new.push_back(IndexedTriangle(frontind, botind, rightind));
  tri_new.push_back(IndexedTriangle(rightind, botind, backind));
  tri_new.push_back(IndexedTriangle(backind, botind, leftind));

  int round=0;
  unsigned int i1, i2, imid;
  Vector3d p1, p2, p3,pmin, pmax, pmid, pmid_test, dir1, dir2;
  double dist,ang,ang_test;
  do {
    triangles = tri_new;	  
    if(round == n) break;
    tri_new.clear();
    std::vector<int> midinds;
    for(const IndexedTriangle & tri: triangles) {
      int zeroang=0;	    
      unsigned int midind=-1;
      for(int i=0;i<3;i++) {
        i1=tri[i];
	i2=tri[(i+1)%3];
        SphereEdgeDb edge(i1, i2);
  	if(edges.count(edge) > 0) continue;
        dist=(vertices[i1]-vertices[i2]).norm();
	if(dist < fs) continue;
	p1=vertices[i1];
	p2=vertices[i2];
	pmin=p1;
	pmax=p2;
	pmid=(pmin+pmax)/2;
	pmid=sphereCalcIndInt(func, pmid);
	dir1=(pmid-p1).normalized();
	dir2=(p2-pmid).normalized();
	ang=acos(dir1.dot(dir2));
//	printf("ang=%g\n",ang*180/3.14);
        if(ang < 0.001) { zeroang++; continue; }
	do
	{
	  pmid_test=(pmin+pmid)/2;
	  pmid_test=sphereCalcIndInt(func, pmid_test);
	  dir1=(pmid_test-p1).normalized();
	  dir2=(p2-pmid_test).normalized();
	  ang_test=acos(dir1.dot(dir2));
	  if(ang_test > ang) {
		  pmax=pmid; ang=ang_test; pmid = pmid_test; 
		  if((pmax-pmin).norm() > fs) continue;
	  }

 	  pmid_test=(pmax+pmid)/2;
 	  pmid_test=sphereCalcIndInt(func, pmid_test);
	  dir1=(pmid_test-p1).normalized();
	  dir2=(p2-pmid_test).normalized();
	  ang_test=acos(dir1.dot(dir2));
	  if(ang_test > ang) {
		  pmin=pmid; ang=ang_test; pmid = pmid_test;
		  if((pmax-pmin).norm() > fs) continue;
	  }
	}
        while(0);	

  	imid=builder.vertexIndex(pmid);
        if(imid == vertices.size()) vertices.push_back(pmid);
	edges[edge]=imid;
      }
      if(zeroang == 3) {
	p1=vertices[tri[0]];
	p2=vertices[tri[1]];
	p3=vertices[tri[2]];
	pmid=(p1+p2+p3)/3.0;
	pmid=sphereCalcIndInt(func, pmid);
        Vector4d norm=calcTriangleNormal(vertices,{tri[0], tri[1], tri[2] });
	if(fabs(pmid.dot(norm.head<3>())- norm[3]) > 1e-3) {
  	  midind=builder.vertexIndex(pmid);
          if(midind == vertices.size()) vertices.push_back(pmid);
	}  
      }
      midinds.push_back(midind);
    }
    // create new triangles from split edges    
    int ind=0;
    for(const IndexedTriangle & tri: triangles) {

      int splitind[3];	    
      for(int i=0;i<3;i++) {
        SphereEdgeDb e(tri[i], tri[(i+1)%3]);
        splitind[i] = edges.count(e) > 0?edges[e]:-1;
      }

      if(midinds[ind] != -1) {
        for(int i=0;i<3;i++) {	     
          if(splitind[i] == -1) {		
            tri_new.push_back(IndexedTriangle(tri[i], tri[(i+1)%3], midinds[ind]));
	  }
	  else {
            tri_new.push_back(IndexedTriangle(tri[i], splitind[i], midinds[ind]));
            tri_new.push_back(IndexedTriangle(splitind[i], tri[(i+1)%3], midinds[ind]));
	  }  
	}  
        ind++;
        continue;	
      }

      int bucket= ((splitind[0]!= -1)?1:0) | ((splitind[1]!= -1)?2:0) | ((splitind[2]!= -1)?4:0) ;
      switch(bucket) {
          case 0:		
            tri_new.push_back(IndexedTriangle(tri[0], tri[1], tri[2]));
	    break;
	  case 1:
            tri_new.push_back(IndexedTriangle(tri[0], splitind[0], tri[2]));
            tri_new.push_back(IndexedTriangle(tri[2], splitind[0], tri[1]));
	    break;
	  case 2:
            tri_new.push_back(IndexedTriangle(tri[1], splitind[1], tri[0]));
            tri_new.push_back(IndexedTriangle(tri[0], splitind[1], tri[2]));
	    break;
	  case 3:
            tri_new.push_back(IndexedTriangle(tri[0], splitind[0], tri[2]));
            tri_new.push_back(IndexedTriangle(splitind[0], splitind[1],tri[2]));
            tri_new.push_back(IndexedTriangle(splitind[0], tri[1],splitind[1]));
	    break;
	  case 4:
            tri_new.push_back(IndexedTriangle(tri[2], splitind[2], tri[1]));
            tri_new.push_back(IndexedTriangle(tri[1], splitind[2], tri[0]));
	    break;
	  case 5:
            tri_new.push_back(IndexedTriangle(tri[0], splitind[0], splitind[2]));
            tri_new.push_back(IndexedTriangle(splitind[0], tri[2], splitind[2]));
            tri_new.push_back(IndexedTriangle(splitind[0], tri[1],tri[2]));
	    break;
	  case 6:
            tri_new.push_back(IndexedTriangle(tri[0], tri[1], splitind[2]));
            tri_new.push_back(IndexedTriangle(tri[1], splitind[1], splitind[2]));
            tri_new.push_back(IndexedTriangle(splitind[1], tri[2],splitind[2]));
	    break;
	  case 7:
            tri_new.push_back(IndexedTriangle(splitind[2], tri[0], splitind[0]));
            tri_new.push_back(IndexedTriangle(splitind[0], tri[1], splitind[1]));
            tri_new.push_back(IndexedTriangle(splitind[1], tri[2], splitind[2]));
            tri_new.push_back(IndexedTriangle(splitind[0], splitind[1], splitind[2]));
	    break;
      }
      ind++;
    }  
     
    round++;
  }
  while(tri_new.size() != triangles.size());
  for(const IndexedTriangle & tri: tri_new) {
    builder.appendPolygon({tri[0], tri[1], tri[2]});
  }
  auto ps = builder.build();
  return ps; 
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
    if(python_numberval(rp, &r, &(node->dragflags), 1))
    if(rp->ob_type == &PyFunction_Type) node->r_func = rp;
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

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CylinderNode>(instance);

  char *kwlist[] = {"h", "r1", "r2", "center",  "r", "d", "d1", "d2", "angle", "fn", "fa", "fs", NULL};
  PyObject *h_ = nullptr;
  PyObject *r_ = nullptr;
  double r1 = NAN;
  double r2 = NAN;
  double d = NAN;
  double d1 = NAN;
  double d2 = NAN;
  double angle = NAN;

  double fn = NAN, fa = NAN, fs = NAN;

  PyObject *center = NULL;
  double vr1 = 1, vr2 = 1, vh = 1;


  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OddOOddddddd", kwlist, &h_, &r1, &r2, &center, &r_, &d, &d1, &d2, &angle,  &fn, &fa, &fs)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing cylinder(h,r|r1+r2|d1+d2)");
    return NULL;
  }
  double r = NAN;
  double h = NAN;

  python_numberval(r_, &r, &(node->dragflags), 1);
  python_numberval(h_, &h, &(node->dragflags), 2);

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

  if (!isnan(angle)) node->angle = angle;
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

  python_retrieve_pyname(node);
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

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

#ifdef ENABLE_LIBFIVE
PyObject *python_frep(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<FrepNode>(instance);
  PyObject *expression=NULL;
  PyObject *bmin=NULL, *bmax=NULL;
  double res=10;

  char *kwlist[] = {"exp","min","max","res", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|d", kwlist,
                                   &expression,
				   &bmin, &bmax, &res
				   )) return NULL;

  python_vectorval(bmin, 3, 3, &(node->x1), &(node->y1), &(node->z1));
  python_vectorval(bmax, 3, 3, &(node->x2), &(node->y2), &(node->z2));
  node->res = res;

  if(expression->ob_type == &PyDataType) {
  	node->expression = expression;
  } else if(expression->ob_type == &PyFunction_Type) {
  	node->expression = expression;
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown frep expression type\n");
    return NULL;
  }

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}


PyObject *python_ifrep(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  PyObject *object = NULL;
  PyObject *dummydict;

  char *kwlist[] = {"obj", nullptr};
  std::shared_ptr<AbstractNode> child;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyOpenSCADType, &object
				   )) return NULL;

  child = PyOpenSCADObjectToNodeMulti(object,&dummydict);
  LeafNode *node = (LeafNode *)   child.get();
  const std::shared_ptr<const Geometry> geom = node->createGeometry();
  const std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(geom);
 
  return ifrep(ps);
}

#endif

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

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}
PyObject *python_circle(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<CircleNode>(instance);

  char *kwlist[] = {"r", "d", "angle", "fn", "fa", "fs", NULL};
  double r = NAN;
  double d = NAN;
  double angle = NAN;
  double fn = NAN, fa = NAN, fs = NAN;

  double vr = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|dddddd", kwlist, &r, &d, &angle, &fn, &fa, &fs)) {
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

  if (!isnan(angle)) node->angle = angle;

  get_fnas(node->fn, node->fa, node->fs);
  if (!isnan(fn)) node->fn = fn;
  if (!isnan(fa)) node->fa = fa;
  if (!isnan(fs)) node->fs = fs;

  node->r = vr;


  python_retrieve_pyname(node);
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
  Vector3d point;

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
      point[2]=0; // default no radius
      if (python_vectorval(element, 2, 3, &point[0], &point[1], &point[2])) {
        PyErr_SetString(PyExc_TypeError, "Coordinate must contain 2 or 3 numbers");
        return NULL;
      }
      node->points.push_back(point);
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

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_spline(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  unsigned int i;
  auto node = std::make_shared<SplineNode>(instance);

  char *kwlist[] = {"points", "fn", "fa", "fs", NULL};
  PyObject *points = NULL;
  double fn=0, fa=0, fs=0;

  PyObject *element;
  Vector2d point;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|ddd", kwlist,
                                   &PyList_Type, &points, &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing spline(points)");
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
  node->fn=fn;
  node->fa=fa;
  node->fs=fs;

  python_retrieve_pyname(node);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

int python_tomatrix(PyObject *pyt, Matrix4d &mat)
{
  if(pyt == nullptr) return 1;
  PyObject *row, *cell;
  double val;
  if(!PyList_Check(pyt)) return 1; // TODO crash wenn pyt eine funktion ist
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


PyObject *python_matrix_scale(PyObject *mat, Vector3d scalevec)
{
  Transform3d matrix=Transform3d::Identity();
  matrix.scale(scalevec);
  Matrix4d raw;
  if(python_tomatrix(mat, raw)) return nullptr;
  Vector3d n;
  for(int i=0;i<3;i++) {
    n =Vector3d(raw(0,i),raw(1,i),raw(2,i)); // TODO fix
    n = matrix * n;
    for(int j=0;j<3;j++) raw(j,i) = n[j];
  }  
  return python_frommatrix(raw);
}


PyObject *python_scale_sub(PyObject *obj, Vector3d scalevec)
{
  PyObject *mat = python_matrix_scale(obj, scalevec);
  if(mat != nullptr) return mat;

  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<TransformNode>(instance, "scale");
  PyObject *child_dict;
  child = PyOpenSCADObjectToNodeMulti(obj,&child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in scale");
    return NULL;
  }
  node->matrix.scale(scalevec);
  node->setPyName(child->getPyName());
  node->children.push_back(child);
  PyObject *pyresult =  PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyObject *value1 = python_matrix_scale(value,scalevec);
       if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
       else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;

}

PyObject *python_scale_core(PyObject *obj, PyObject *val_v)
{
 
  double x=1, y=1, z=1;
  if (python_vectorval(val_v, 2, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in scale, use 1 to 3 ordinates.");
    return NULL;
  }
  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0])|| !std::isfinite(scalevec[1])|| !std::isfinite(scalevec[2])) {
//      LOG(message_group::Warning, instance->location(), parameters.documentRoot(), "scale(%1$s)", parameters["v"].toEchoStringNoThrow());
    }
  }

  return python_scale_sub(obj,scalevec);
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
PyObject *python_matrix_rot(PyObject *mat, Matrix3d rotvec)
{
  Transform3d matrix=Transform3d::Identity();
  matrix.rotate(rotvec);
  Matrix4d raw;
  if(python_tomatrix(mat, raw)) return nullptr;
  Vector3d n;
  for(int i=0;i<4;i++) {
    n =Vector3d(raw(0,i),raw(1,i),raw(2,i));
    n = matrix * n;
    for(int j=0;j<3;j++) raw(j,i) = n[j];
  }  
  return python_frommatrix(raw);
}


PyObject *python_rotate_sub(PyObject *obj, Vector3d vec3, double angle, int dragflags)
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
  PyObject *mat = python_matrix_rot(obj, M);
  if(mat != nullptr) return mat;

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "rotate");
  node->dragflags = dragflags;

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in rotate");
    return NULL;
  }
  node->matrix.rotate(M);
  node->setPyName(child->getPyName());

  node->children.push_back(child);
  PyObject *pyresult =  PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyObject *value1 = python_matrix_rot(value,M);
       if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
       else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
}

PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, PyObject *val_v)
{
  Vector3d vec3(0,0,0);
  double angle;
  int dragflags=0;
  if (val_a != nullptr && PyList_Check(val_a) && val_v == nullptr) {
    python_vectorval(val_a, 1, 3, &(vec3[0]), &(vec3[1]), &(vec3[2]), nullptr, &dragflags);
    return python_rotate_sub(obj, vec3, NAN, dragflags);
  } else if (val_a != nullptr && val_v != nullptr && !python_numberval(val_a,&angle) && PyList_Check(val_v) && PyList_Size(val_v) == 3) {
    vec3[0]= PyFloat_AsDouble(PyList_GetItem(val_v, 0));
    vec3[1]= PyFloat_AsDouble(PyList_GetItem(val_v, 1));
    vec3[2]= PyFloat_AsDouble(PyList_GetItem(val_v, 2));
    return python_rotate_sub(obj, vec3, angle, dragflags);
  }
  PyErr_SetString(PyExc_TypeError, "Invalid arguments to rotate()");
  return nullptr;
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


PyObject *python_matrix_mirror(PyObject *mat, Matrix4d m)
{
  Matrix4d raw;
  if(python_tomatrix(mat, raw)) return nullptr;
  Vector4d n;
  for(int i=0;i<3;i++) {
    n =Vector4d(raw(0,i),raw(1,i),raw(2,i),0);
    n = m * n;
    for(int j=0;j<3;j++) raw(j,i) = n[j];
  }  
  return python_frommatrix(raw);
}


PyObject *python_mirror_sub(PyObject *obj, Matrix4d &m)
{
  PyObject *mat = python_matrix_mirror(obj,m);
  if(mat != nullptr) return mat;

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "mirror");
  node->matrix = m;
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in mirror");
    return NULL;
  }
  node->children.push_back(child);
  node->setPyName(child->getPyName());
  PyObject *pyresult =  PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
       PyObject *value1 = python_matrix_mirror(value,m);
       if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
       else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
}

PyObject *python_mirror_core(PyObject *obj, PyObject *val_v)
{
  Vector3d mirrorvec;
  double x = 1.0, y = 0.0, z = 0.0;
  if (python_vectorval(val_v, 2, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in mirror");
    return NULL;
  }
  // x /= sqrt(x*x + y*y + z*z)
  // y /= sqrt(x*x + y*y + z*z)
  // z /= sqrt(x*x + y*y + z*z)
  Matrix4d m=Matrix4d::Identity();
  if (x != 0.0 || y != 0.0 || z != 0.0) {
    // skip using sqrt to normalize the vector since each element of matrix contributes it with two multiplied terms
    // instead just divide directly within each matrix element
    // simplified calculation leads to less float errors
    double a = x * x + y * y + z * z;

    m << 1 - 2 * x * x / a, -2 * y * x / a, -2 * z * x / a, 0,
      -2 * x * y / a, 1 - 2 * y * y / a, -2 * z * y / a, 0,
      -2 * x * z / a, -2 * y * z / a, 1 - 2 * z * z / a, 0,
      0, 0, 0, 1;
  }
  return python_mirror_sub(obj,m);
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
                                   &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_matrix_trans(PyObject *mat, Vector3d transvec)
{
  Matrix4d raw;
  if(python_tomatrix(mat, raw)) return nullptr;
  for(int i=0;i<3;i++) raw(i,3) += transvec[i];
  return python_frommatrix(raw);
}

PyObject *python_translate_sub(PyObject *obj, Vector3d translatevec, int dragflags)
{
  PyObject *child_dict;
  PyObject *mat = python_matrix_trans(obj,translatevec);
  if(mat != nullptr) return mat;

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "translate");
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  node->setPyName(child->getPyName());
  node->dragflags=dragflags;
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
       PyObject *value1 = python_matrix_trans(value,translatevec);
       if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
       else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
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


PyObject *python_dir_sub_core(PyObject *obj, double arg, int mode)
{
  if(mode < 6)
  {
    Vector3d trans;
    switch(mode) {	  
      case 0: trans=Vector3d(arg,0,0); break;
      case 1: trans=Vector3d(-arg,0,0); break;
      case 2: trans=Vector3d(0,-arg,0); break;
      case 3: trans=Vector3d(0,arg,0); break;
      case 4: trans=Vector3d(0,0,-arg); break;
      case 5: trans=Vector3d(0,0,arg); break;
    }
    return python_translate_sub(obj, trans,0);
  }
  else 
  {
    Vector3d rot;
    switch(mode) {	  
      case 6: rot = Vector3d(arg,0,0); break;
      case 7: rot = Vector3d(0,arg,0); break;
      case 8: rot = Vector3d(0,0,arg); break;
    }		       
    return python_rotate_sub(obj, rot, NAN, 0);
  }
}

PyObject *python_dir_sub(PyObject *self, PyObject *args, PyObject *kwargs,int mode)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  double arg;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od", kwlist,
                                   &obj,
                                   &arg
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_dir_sub_core(obj,arg,mode);
}

PyObject *python_oo_dir_sub(PyObject *obj, PyObject *args, PyObject *kwargs,int mode)
{
  char *kwlist[] = {"v", NULL};
  double arg;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist,
                                   &arg
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_dir_sub_core(obj,arg,mode);
}

PyObject *python_right(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 0); }
PyObject *python_oo_right(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 0); }
PyObject *python_left(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 1); }
PyObject *python_oo_left(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 1); }
PyObject *python_front(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 2); }
PyObject *python_oo_front(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 2); }
PyObject *python_back(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 3); }
PyObject *python_oo_back(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 3); }
PyObject *python_down(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 4); }
PyObject *python_oo_down(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 4); }
PyObject *python_up(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 5); }
PyObject *python_oo_up(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 5); }
PyObject *python_rotx(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 6); }
PyObject *python_oo_rotx(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 6); }
PyObject *python_roty(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 7); }
PyObject *python_oo_roty(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 7); }
PyObject *python_rotz(PyObject *self, PyObject *args, PyObject *kwargs) { return python_dir_sub(self, args,kwargs, 8); }
PyObject *python_oo_rotz(PyObject *self, PyObject *args, PyObject *kwargs) { return python_oo_dir_sub(self, args,kwargs, 8); }

PyObject *python_multmatrix_sub(PyObject *pyobj, PyObject *pymat, int div)
{
  Matrix4d mat;
  if(!python_tomatrix(pymat, mat)) {
    double w = mat(3, 3);
    if (w != 1.0) mat = mat / w;
  } else {
    PyErr_SetString(PyExc_TypeError, "Matrix vector should be 4x4 array");
    return NULL;
  }
  if(div){
    auto tmp =  mat.inverse().eval();
    mat=tmp;
  }

  Matrix4d objmat;
  if(!python_tomatrix(pyobj, objmat)){
    objmat = mat * objmat;
    return python_frommatrix(objmat);
  } 

  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "multmatrix");
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict;
  child = PyOpenSCADObjectToNodeMulti(pyobj, &child_dict);
  node->setPyName(child->getPyName());
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in multmatrix");
    return NULL;
  }

  node->matrix = mat;
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  if(child_dict != nullptr ) { 
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while(PyDict_Next(child_dict, &pos, &key, &value)) {
      Matrix4d raw;
      if(python_tomatrix(value, raw)) return nullptr;
      PyObject *value1 = python_frommatrix(node->matrix * raw );
      if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;

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
  return python_multmatrix_sub(obj, mat,0);
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
  return python_multmatrix_sub(obj, mat,0);
}

PyObject *python_divmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist,
                                   &obj,
                                   &PyList_Type, &mat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat,1);
}

PyObject *python_oo_divmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist,
                                   &PyList_Type, &mat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat,1);
}

PyObject *python_pull_core(PyObject *obj, PyObject *anchor, PyObject *dir)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<PullNode>(instance);
  PyObject *dummydict;
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
  node->anchor = Vector3d(x,y,z);

  if (python_vectorval(dir, 3, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in dir\n");
    return NULL;
  }
  node->dir = Vector3d(x,y,z);

  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_pull(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "src", "dst",NULL};
  PyObject *obj = NULL;
  PyObject *anchor = NULL;
  PyObject *dir = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|", kwlist,
                                   &obj,
                                   &anchor,
				   &dir
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_pull_core(obj, anchor, dir);
}

PyObject *python_oo_pull(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"src", "dst",NULL};
  PyObject *anchor = NULL;
  PyObject *dir = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|", kwlist,
                                   &anchor,
				   &dir
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_pull_core(obj, anchor, dir);
}

PyObject *python_wrap_core(PyObject *obj, double r, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  auto node = std::make_shared<WrapNode>(instance);
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in Wrap\n");
    return NULL;
  }


  node->r = r;
  get_fnas(node->fn, node->fa, node->fs);
  if(!isnan(fn)) node->fn=fn;
  if(!isnan(fa)) node->fa=fa;
  if(!isnan(fs)) node->fs=fs;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_wrap(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "r","fn","fa","fs", NULL};
  PyObject *obj = NULL;
  double r, fn, fa, fs;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od|ddd", kwlist,
                                   &obj,
                                   &r, &fn, &fa, &fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing wrap\n");
    return NULL;
  }
  return python_wrap_core(obj, r, fn,fa, fs);
}

PyObject *python_oo_wrap(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"r","fn","fa","fs",NULL};
  double r,fn=NAN,fa=NAN,fs=NAN;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|ddd", kwlist,
                                   &r,&fn,&fa,&fs
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_wrap_core(obj, r, fn, fa, fs);
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
  python_result_obj = obj;
  mapping_name.clear();
  mapping_code.clear();
  mapping_level.clear();
  python_build_hashmap(child,0);
  python_result_handle.clear();
  Matrix4d raw;
  SelectedObject sel;
  std::string varname=child->getPyName();
  if(child_dict != nullptr) {
    while(PyDict_Next(child_dict, &pos, &key, &value)) {
       if(python_tomatrix(value, raw)) continue;
       PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
       const char *value_str =  PyBytes_AS_STRING(value1);
       sel.pt.clear();
       sel.pt.push_back(Vector3d(raw(0,3),raw(1,3),raw(2,3)));
       sel.pt.push_back(Vector3d(raw(0,0),raw(1,0),raw(2,0)));
       sel.pt.push_back(Vector3d(raw(0,1),raw(1,1),raw(2,1)));
       sel.pt.push_back(Vector3d(raw(0,2),raw(1,2),raw(2,2)));
       sel.type=SelectionType::SELECTION_HANDLE;
       sel.name=varname+"."+value_str;
       python_result_handle.push_back(sel);
    }
  }
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

PyObject *python_output(PyObject *obj, PyObject *args, PyObject *kwargs){
  LOG(message_group::Deprecated, "output is deprecated, please use show() instead");
  return python_show(obj, args, kwargs);	
  
}

PyObject *python_oo_output(PyObject *obj, PyObject *args, PyObject *kwargs){
  LOG(message_group::Deprecated, "output is deprecated, please use show() instead");
  return python_oo_show(obj, args, kwargs);	
}


void Export3mfPartInfo::writeProps(void *obj) const
{
  if(this->props == nullptr) return;
  PyObject *prop = (PyObject *) this->props;
  if(!PyDict_Check(prop)) return;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(prop, &pos, &key, &value)) {
    PyObject* key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
    const char *key_str =  PyBytes_AS_STRING(key1);
    if(key_str == nullptr) continue;
    if(PyFloat_Check(value)) {
      writePropsFloat(obj, key_str,PyFloat_AsDouble(value));
    }
    if(PyLong_Check(value)) {
      writePropsLong(obj, key_str,PyLong_AsLong(value));
    }
    if(PyUnicode_Check(value)) {
      PyObject* val1 = PyUnicode_AsEncodedString(value, "utf-8", "~");
      const char *val_str =  PyBytes_AS_STRING(val1);
      writePropsString(obj, key_str,val_str);
    }
  }
}

void python_export_obj_att(std::ostream& output)
{
  PyObject *child_dict= nullptr;
  if(python_result_obj == nullptr) return;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(python_result_obj, &child_dict);
  if(child_dict == nullptr) return;
  if(!PyDict_Check(child_dict)) return;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(child_dict, &pos, &key, &value)) {
    PyObject* key1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
    const char *key_str =  PyBytes_AS_STRING(key1);
    if(key_str == nullptr) continue;

    if(PyLong_Check(value))  
      output <<  "# " << key_str << " = " << PyLong_AsLong(value) << "\n" ;

    if(PyFloat_Check(value))  
      output <<  "# " << key_str << " = " << PyFloat_AsDouble(value) << "\n" ;

    if(PyUnicode_Check(value)) {
      auto valuestr = std::string(PyUnicode_AsUTF8(value));
      output <<  "# " << key_str << " = \"" << valuestr << "\"\n" ;
    }  
  }

}	

PyObject *python_export_core(PyObject *obj, char *file)
{
  const auto path = fs::path(file);
  std::string suffix = path.has_extension() ? path.extension().generic_string().substr(1) : "";
  boost::algorithm::to_lower(suffix);
  python_result_obj = obj;

  FileFormat exportFileFormat = FileFormat::BINARY_STL;
  if (!fileformat::fromIdentifier(suffix, exportFileFormat)) {
    LOG("Invalid suffix %1$s. Defaulting to binary STL.", suffix);
  }

  std::vector<Export3mfPartInfo> export3mfPartInfos;
  std::vector<std::string>  names;

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if(child != nullptr ) {
    Tree tree(child, "parent");
    GeometryEvaluator geomevaluator(tree);
    Export3mfPartInfo info(geomevaluator.evaluateGeometry(*tree.root(), false), "OpenSCAD Model", nullptr);
    export3mfPartInfos.push_back(info);
  } else if(PyDict_Check(obj)) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(obj, &pos, &key, &value)) {
      PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str =  PyBytes_AS_STRING(value1);
      if(value_str == nullptr) continue;
      std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(value, &child_dict);
      if(child == nullptr) continue;

      void *prop = nullptr;
      if(child_dict != nullptr && PyDict_Check(child_dict)) {
        PyObject *key = PyUnicode_FromStringAndSize("props_3mf",9);
        prop = PyDict_GetItem(child_dict, key);
      }
      Tree tree(child, "parent");
      GeometryEvaluator geomevaluator(tree);
      Export3mfPartInfo info(geomevaluator.evaluateGeometry(*tree.root(), false),value_str, prop);
      export3mfPartInfos.push_back(info);
    }
  }
  if ( export3mfPartInfos.size() == 0) {
    PyErr_SetString(PyExc_TypeError, "Object not recognized");
    return NULL;
  }  


  Export3mfOptions options3mf;
  options3mf .decimalPrecision=6;
  options3mf.color="#f9d72c";
  ExportInfo exportInfo = {.format = exportFileFormat, .sourceFilePath = file, .options3mf = std::make_shared<Export3mfOptions>(options3mf)};
 
  if(exportFileFormat == FileFormat::_3MF) {
    std::ofstream fstream(file,  std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fstream.is_open()) {
      LOG(_("Can't open file \"%1$s\" for export"), file);
      return nullptr;
    }
    export_3mf(export3mfPartInfos, fstream, exportInfo);
  }
  else{
    if(export3mfPartInfos.size() > 1) {
      LOG("This Format can at most export one object");
      return nullptr;
    }	    
    exportFileByName(export3mfPartInfos[0].geom, file, exportInfo);
  }
  return Py_None;
}

PyObject *python_export(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  char *file= nullptr;
  char *kwlist[] = {"obj", "file", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Os|O", kwlist,
                                   &obj,&file
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_export_core(obj,file);
}


PyObject *python_oo_export(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"file", NULL};
  char *file = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing output(object)");
    return NULL;
  }
  return python_export_core(obj, file);
}

PyObject *python_find_face_core(PyObject *obj, PyObject *vec_p)
{
  Vector3d vec;	
  double dummy;
  PyObject *child_dict;	  
  if(python_vectorval(vec_p, 3, 3, &vec[0], &vec[1], &vec[2], &dummy)) return Py_None;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);

  if(child == nullptr ) return Py_None;


  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);

  double dmax=-1;
  Vector4d vectormax;
  for(auto ind: ps->indices) {
    Vector4d norm =calcTriangleNormal(ps->vertices,ind);
    double d=norm.head<3>().dot(vec);
    if(d > dmax) {
      dmax = d;	    
      vectormax=norm;
    }

  }
  PyObject *coord = PyList_New(4);
  for(int i=0;i<4;i++) 
    PyList_SetItem(coord, i, PyFloat_FromDouble(vectormax[i]));
  return coord;
}

PyObject *python_find_face(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "vec", NULL};
  PyObject *obj = nullptr;
  PyObject *vec = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &vec
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing find_face(object)");
    return NULL;
  }
  return python_find_face_core(obj, vec);
}

PyObject *python_oo_find_face(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"vec", NULL};
  PyObject *vec = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &vec
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing find_face(object)");
    return NULL;
  }
  return python_find_face_core(obj, vec);
}

PyObject *python_sitonto_core(PyObject *pyobj, PyObject *vecx_p, PyObject *vecy_p, PyObject *vecz_p)
{
  Vector4d vecx, vecy, vecz;
  Vector3d cut;
  if(python_vectorval(vecx_p, 3, 3, &vecx[0], &vecx[1], &vecx[2], &vecx[3])) return Py_None;
  if(python_vectorval(vecy_p, 3, 3, &vecy[0], &vecy[1], &vecy[2], &vecy[3])) return Py_None;
  if(python_vectorval(vecz_p, 3, 3, &vecz[0], &vecz[1], &vecz[2], &vecz[3])) return Py_None;

  if(cut_face_face_face(
			  vecx.head<3>()*vecx[3], vecx.head<3>(),
			  vecy.head<3>()*vecy[3], vecy.head<3>(),
			  vecz.head<3>()*vecz[3], vecz.head<3>(),
			  cut)) return Py_None;
  DECLARE_INSTANCE
  auto node = std::make_shared<TransformNode>(instance, "sitontonode");
  std::shared_ptr<AbstractNode> child;
  PyObject *dummydict;
  child = PyOpenSCADObjectToNodeMulti(pyobj, &dummydict);
  node->setPyName(child->getPyName());
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in multmatrix");
    return NULL;
  }
  Matrix4d mat;
  Vector3d vecx_n = vecz.head<3>().cross(vecy.head<3>()).normalized();
  Vector3d vecy_n = vecx.head<3>().cross(vecz.head<3>()).normalized();
  if(vecx.head<3>().cross(vecy.head<3>()).dot(vecz.head<3>()) < 0){
    vecx_n = -vecx_n;
    vecy_n = -vecy_n;    
  }
  mat <<	vecx_n[0],vecy_n[0],vecz[0],cut[0],
		vecx_n[1],vecy_n[1],vecz[1],cut[1],
		vecx_n[2],vecy_n[2],vecz[2],cut[2],
		0,0,0      ,1;
 
  node->matrix = mat;
  node->children.push_back(child);
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_sitonto(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "vecz","vecx","vecy", NULL};
  PyObject *obj = nullptr;
  PyObject *vecx = nullptr;
  PyObject *vecy = nullptr;
  PyObject *vecz = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOOO", kwlist, &obj, &vecz, &vecx, &vecy
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing sitonto(object)");
    return NULL;
  }
  return python_sitonto_core(obj, vecx, vecy, vecz);
}

PyObject *python_oo_sitonto(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"vecz"," vecx", "vecy",  NULL};
  PyObject *vecx = nullptr;
  PyObject *vecy = nullptr;
  PyObject *vecz = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", kwlist, &vecz, &vecx, &vecy
                                   ))  {
    PyErr_SetString(PyExc_TypeError, "Error during parsing sitonto(object)");
    return NULL;
  }
  return python_sitonto_core(obj, vecx, vecy, vecz);
}


PyObject *python__getitem__(PyObject *obj, PyObject *key)
{
  PyOpenSCADObject *self = (PyOpenSCADObject *) obj;
  if (self->dict == nullptr) {
    return nullptr;
  }
  PyObject *result = PyDict_GetItem(self->dict, key);
  if (result == NULL){
    PyObject* keyname = PyUnicode_AsEncodedString(key, "utf-8", "~");
    std::string keystr = PyBytes_AS_STRING(keyname);
    result = Py_None;
    if(keystr == "matrix") {
      PyObject *dummy_dict;
      std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(obj, &dummy_dict);
      std::shared_ptr<const TransformNode> trans = std::dynamic_pointer_cast<const TransformNode>(node);
      Matrix4d matrix=Matrix4d::Identity();
      if(trans != nullptr) matrix = trans->matrix.matrix();		
      result = python_frommatrix(matrix);
    }
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


PyObject *python_color_core(PyObject *obj, PyObject *color, double alpha, int textureind)
{
  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child;
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
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
    const auto color = OpenSCAD::parse_color(colorname);
    if (color) {
      node->color = *color;
      node->color[3]=alpha;
    } else {
      PyErr_SetString(PyExc_TypeError, "Cannot parse color");
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown color representation");
    return nullptr;
  }
	
  node->textureind=textureind;
  if(textureind != -1 && color == NULL) {
	node->color[0]=0.5;
	node->color[1]=0.5;
	node->color[2]=0.5;
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
  char *kwlist[] = {"obj", "c", "alpha", "texture",NULL};
  PyObject *obj = NULL;
  PyObject *color = NULL;
  double alpha = 1.0;
  int textureind=-1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Odi", kwlist,
                                   &obj,
                                   &color, &alpha, &textureind
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha, textureind);
}

PyObject *python_oo_color(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"c", "alpha", "texture",NULL};
  PyObject *color = NULL;
  double alpha = 1.0;
  int textureind=-1;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Odi", kwlist,
                                   &color, &alpha, &textureind
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing color");
    return NULL;
  }
  return python_color_core(obj, color, alpha, textureind);
}

typedef std::vector<int> intList;

PyObject *python_mesh_core(PyObject *obj, bool tessellate)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in mesh \n");
    return NULL;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);


  if(ps != nullptr){
    if(tessellate == true) {
      ps = PolySetUtils::tessellate_faces(*ps);
    }
  // Now create Python Point array
    PyObject *ptarr = PyList_New(ps->vertices.size());  
    for(unsigned int i=0;i<ps->vertices.size();i++) {
      PyObject *coord = PyList_New(3);
      for(int j=0;j<3;j++) 
          PyList_SetItem(coord, j, PyFloat_FromDouble(ps->vertices[i][j]));
      PyList_SetItem(ptarr, i, coord);
      Py_XINCREF(coord);
    }
    Py_XINCREF(ptarr);
    // Now create Python Point array
    PyObject *polarr = PyList_New(ps->indices.size());  
    for(unsigned int i=0;i<ps->indices.size();i++) {
      PyObject *face = PyList_New(ps->indices[i].size());
      for(unsigned int j=0;j<ps->indices[i].size();j++) 
          PyList_SetItem(face, j, PyLong_FromLong(ps->indices[i][j]));
      PyList_SetItem(polarr, i, face);
      Py_XINCREF(face);
    }
    Py_XINCREF(polarr);

    PyObject *result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, ptarr);
    PyTuple_SetItem(result, 1, polarr);

    return result;
  }  
  if (auto polygon2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    const std::vector<Outline2d> outlines =  polygon2d->outlines();
    PyObject *pyth_outlines = PyList_New(outlines.size());
    for(unsigned int i=0;i<outlines.size();i++) {
      const Outline2d &outline = outlines[i];	    
      PyObject *pyth_outline = PyList_New(outline.vertices.size());
      for(unsigned int j=0;j<outline.vertices.size();j++) {
        Vector2d pt = outline.vertices[j];	      
        PyObject *pyth_pt = PyList_New(2);
        for(int k=0;k<2;k++) 
          PyList_SetItem(pyth_pt, k, PyFloat_FromDouble(pt[k]));
        PyList_SetItem(pyth_outline, j, pyth_pt);
      }
      PyList_SetItem(pyth_outlines, i, pyth_outline);
    }
    return pyth_outlines;
  }
  return Py_None;
}

PyObject *python_mesh(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "triangulate", NULL};
  PyObject *obj = NULL;
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True);
}

PyObject *python_oo_mesh(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = { "triangulate", NULL};
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True);
}

PyObject *python_separate_core(PyObject *obj)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in separate \n");
    return NULL;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);

  if(ps != nullptr){
    // setup databases	  
    intList empty_list;
    std::vector<intList> pt2tri;	  

    std::vector<int> vert_db;
    for(int i=0;i<ps->vertices.size();i++){
      vert_db.push_back(-1);
      pt2tri.push_back(empty_list);
    }

    std::vector<int> tri_db;
    for(int i=0;i<ps->indices.size();i++) {
      tri_db.push_back(-1);
      for(auto ind: ps->indices[i])
        pt2tri[ind].push_back(i);	      
    }

    // now sort for objects
    int obj_num=0;
    for(int i=0;i<vert_db.size();i++) {
      if(vert_db[i] != -1) continue;
      std::vector<int> vert_todo;
      vert_todo.push_back(i);
      while(vert_todo.size() > 0)
      {
        int vert_ind=vert_todo[vert_todo.size()-1];	      
	vert_todo.pop_back();
	if(vert_db[vert_ind] != -1) continue;
        vert_db[vert_ind]= obj_num;
	for(int tri_ind : pt2tri[vert_ind]) {
          if(tri_db[tri_ind] != -1) continue;
	  tri_db[tri_ind]= obj_num;
          for(int vert1_ind: ps->indices[tri_ind]) {
            if(vert_db[vert1_ind] != -1) continue;		  
	    vert_todo.push_back(vert1_ind);
	  }	  
	}
      }
      obj_num++;      
    }

    PyObject *objects = PyList_New(obj_num);  
    for(int i=0;i<obj_num;i++) {
      // create a polyhedron for each	    
      DECLARE_INSTANCE
      auto node = std::make_shared<PolyhedronNode>(instance);
      node->convexity=2;
      std::vector<int> vert_map;
      for(int j=0;j<ps->vertices.size();j++) {
        if(vert_db[j] == i) {
          vert_map.push_back(node->points.size());
          node->points.push_back(ps->vertices[j]);		
	} else vert_map.push_back(-1);
      }
      for(int j=0;j<ps->indices.size();j++) {
        if(tri_db[j] == i) {
          IndexedFace face_map;				
          for(auto ind: ps->indices[j]) {
            face_map.push_back(vert_map[ind]);
	  }		  
	  node->faces.push_back(face_map);
	}		
      }
      PyList_SetItem(objects, i, PyOpenSCADObjectFromNode(&PyOpenSCADType, node));
    }
    return objects;
  }  
  return Py_None;
}

PyObject *python_separate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_separate_core(obj);
}

PyObject *python_oo_separate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = { NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_separate_core(obj);
}

PyObject *python_edges_core(PyObject *obj)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in faces \n");
    return NULL;
  }

  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  const std::shared_ptr<const Polygon2d> poly = std::dynamic_pointer_cast<const Polygon2d>(geom);
  if(poly == nullptr) return Py_None;
  int edgenum=0;
  Vector3d zdir(0,0,0);
  Transform3d trans = poly->getTransform3d();
  for(auto ol: poly->untransformedOutlines()) {
    int n=ol.vertices.size();
    for(int i=0;i<n;i++)
    {
      Vector3d p1=trans*Vector3d(ol.vertices[i][0], ol.vertices[i][1],0);
      Vector3d p2=trans*Vector3d(ol.vertices[(i+1)%n][0], ol.vertices[(i+1)%n][1],0);
      Vector3d p3=trans*Vector3d(ol.vertices[(i+2)%n][0], ol.vertices[(i+2)%n][1],0);
      zdir += (p1-p2).cross(p2-p3);
    }
    edgenum += n;
  }
  zdir.normalize();
  PyObject *pyth_edges = PyList_New(edgenum);
  int ind=0;

  for(auto ol: poly->untransformedOutlines()) {
    int n= ol.vertices.size();
    for(int i=0;i<n;i++) {
      Vector3d p1=trans*Vector3d(ol.vertices[i][0], ol.vertices[i][1],0);
      Vector3d p2=trans*Vector3d(ol.vertices[(i+1)%n][0], ol.vertices[(i+1)%n][1],0);
      Vector3d pt=(p1+p2)/2.0;
      Vector3d xdir=(p2-p1).normalized();
      Vector3d ydir=xdir.cross(zdir).normalized();

      Matrix4d mat;
      mat <<  xdir[0], ydir[0], zdir[0], pt[0],
              xdir[1], ydir[1], zdir[1], pt[1],
              xdir[2], ydir[2], zdir[2], pt[2],
              0      , 0      , 0      , 1;

      DECLARE_INSTANCE
      auto edge = std::make_shared<EdgeNode>(instance);
      edge->size=(p2-p1).norm();
      edge->center=true;
      {
        DECLARE_INSTANCE
        auto mult = std::make_shared<TransformNode>(instance,"multmatrix");
	mult->matrix = mat;
	mult->children.push_back(edge);

        PyObject *pyth_edge = PyOpenSCADObjectFromNode(&PyOpenSCADType, mult);
        PyList_SetItem(pyth_edges, ind++, pyth_edge);
      }

    }
  }  
  return  pyth_edges;
}

PyObject *python_edges(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_edges_core(obj);
}

PyObject *python_oo_edges(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = { NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_edges_core(obj);
}

PyObject *python_faces_core(PyObject *obj, bool tessellate)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in faces \n");
    return NULL;
  }

  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);


  if(ps != nullptr){
    PolygonIndices inds;	 
    std::vector<int> face_parents;
    if(tessellate == true){
      ps = PolySetUtils::tessellate_faces(*ps);
      inds = ps->indices;
      for(int i=0;i<inds.size();i++) face_parents.push_back(-1);
    }
    else {
      std::vector<Vector4d> normals, new_normals;      
      normals = calcTriangleNormals(ps->vertices, ps->indices);
      inds  = mergeTriangles(ps->indices, normals, new_normals, face_parents, ps->vertices);
    }
    int resultlen=0, resultiter=0;
    for(int i=0;i<face_parents.size();i++)
      if(face_parents[i] == -1) resultlen++;	    

    PyObject *pyth_faces = PyList_New(resultlen);

    for (size_t j=0;j<inds.size();j++) {
      if(face_parents[j] != -1) continue;	    
      auto &face = inds[j];	    
      if(face.size() < 3) continue;	    
      Vector3d zdir=calcTriangleNormal(ps->vertices,face).head<3>().normalized();
      // calc center of face
      Vector3d ptmin, ptmax;
      for(size_t i=0;i<face.size();i++) {
        Vector3d pt = ps->vertices[face[i]];
	for(int k=0;k<3;k++) {
	  if(i == 0 || pt[k] < ptmin[k]) ptmin[k]=pt[k];
	  if(i == 0 || pt[k] > ptmax[k]) ptmax[k]=pt[k];
	}  
      }
      Vector3d pt=Vector3d((ptmin[0]+ptmax[0])/2.0, (ptmin[1]+ptmax[1])/2.0,(ptmin[2]+ptmax[2])/2.0);
      Vector3d xdir = (ps->vertices[face[1]]-ps->vertices[face[0]]).normalized();
      Vector3d ydir = zdir.cross(xdir);

      Matrix4d mat;
      mat <<  xdir[0], ydir[0], zdir[0], pt[0],
              xdir[1], ydir[1], zdir[1], pt[1],
              xdir[2], ydir[2], zdir[2], pt[2],
              0      , 0      , 0      , 1;

      Matrix4d invmat = mat.inverse();       	    
      
      DECLARE_INSTANCE
      auto poly = std::make_shared<PolygonNode>(instance);
      std::vector<size_t> path;
      for(size_t i=0;i<face.size();i++) {
        Vector3d pt = ps->vertices[face[i]];
        Vector4d pt4(pt[0], pt[1], pt[2], 1);
        pt4 = invmat * pt4 ;
	path.push_back(poly->points.size());
	Vector3d pt3 = pt4.head<3>();
	pt3[2]=0; // no radius
        poly->points.push_back(pt3);
      }
      poly->paths.push_back(path);

        // check if there are holes
      for (size_t k=0;k<inds.size();k++) {
        if(face_parents[k] == j){
          auto &hole = inds[k];		

          std::vector<size_t> path;
          for(size_t i=0;i<hole.size();i++) {
            Vector3d pt = ps->vertices[hole[i]];
            Vector4d pt4(pt[0], pt[1], pt[2], 1);
            pt4 = invmat * pt4 ;
	    path.push_back(poly->points.size());
	    Vector3d pt3 = pt4.head<3>();
	    pt3[2]=0; // no radius
            poly->points.push_back(pt3);
          }
          poly->paths.push_back(path);

        }
      }
      {
        DECLARE_INSTANCE
        auto mult = std::make_shared<TransformNode>(instance,"multmatrix");
	mult->matrix = mat;
	mult->children.push_back(poly);

        PyObject *pyth_face = PyOpenSCADObjectFromNode(&PyOpenSCADType, mult);
        PyList_SetItem(pyth_faces, resultiter++, pyth_face);
      }

    }
    return  pyth_faces;
  }  
  return Py_None;
}

PyObject *python_faces(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "triangulate", NULL};
  PyObject *obj = NULL;
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_faces_core(obj, tess == Py_True);
}

PyObject *python_oo_faces(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = { "triangulate", NULL};
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_faces_core(obj, tess == Py_True);
}

PyObject *python_oversample_core(PyObject *obj, int n, PyObject *round)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in oversample \n");
    return NULL;
  }

  DECLARE_INSTANCE
  auto node = std::make_shared<OversampleNode>(instance);
  node->children.push_back(child);
  node->n = n;
  node->round=0;
  if(round == Py_True) node->round=1;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_oversample(PyObject *self, PyObject *args, PyObject *kwargs)
{
  int n=2;
  char *kwlist[] = {"obj", "n","round",NULL};
  PyObject *obj = NULL;
  PyObject *round= NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|O", kwlist, &obj,&n,&round)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_oversample_core(obj,n,round);
}

PyObject *python_oo_oversample(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  int n=2;
  char *kwlist[] = {"n","round",NULL};
  PyObject *round= NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|O", kwlist,&n,&round)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_oversample_core(obj,n,round);
}

PyObject *python_debug_core(PyObject *obj, PyObject *faces)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in debug \n");
    return NULL;
  }

  DECLARE_INSTANCE
  auto node = std::make_shared<DebugNode>(instance);
  node->children.push_back(child);
  if(faces != nullptr) {
    std::vector<int> intfaces = python_intlistval(faces);
    node->faces = intfaces;
  }  
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_debug(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "faces",NULL};
  PyObject *obj = NULL;
  PyObject *faces= NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &obj,&faces)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_debug_core(obj,faces);
}

PyObject *python_oo_debug(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"faces",NULL};
  PyObject *faces= NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &faces)) {
    PyErr_SetString(PyExc_TypeError, "error duing parsing\n");
    return NULL;
  }
  return python_debug_core(self,faces);
}


PyObject *python_fillet_core(PyObject *obj, double  r, int fn, PyObject *sel, double minang)
{
  PyObject *dummydict;
  DECLARE_INSTANCE
  auto node = std::make_shared<FilletNode>(instance);
  node->r = r;
  node->fn = fn;
  node->minang=minang;
  if (obj != nullptr) node->children.push_back(PyOpenSCADObjectToNodeMulti(obj, &dummydict));
  else {	 
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in fillet \n");
    return NULL;
  }

  if(sel != nullptr) 
    node->children.push_back(PyOpenSCADObjectToNodeMulti(sel, &dummydict));

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_fillet(PyObject *self, PyObject *args, PyObject *kwargs)
{
  double r=1.0;
  double fn=NAN;
  double minang=30;
  char *kwlist[] = {"obj", "r","sel","n", "minang", NULL};
  PyObject *obj = NULL;
  PyObject *sel = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od|Odd", kwlist, &obj,&r,&sel,&fn,&minang)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  double dummy;
  if(isnan(fn)) get_fnas(fn, dummy, dummy);
  return python_fillet_core(obj,r,fn, sel, minang);
}

PyObject *python_oo_fillet(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  double r=1.0;
  double fn=NAN;
  double minang=30;
  PyObject *sel = nullptr;
  char *kwlist[] = {"r","sel", "fn","minang", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|Odd", kwlist,&r,&sel,&fn,&minang)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  double dummy;
  if(isnan(fn)) get_fnas(fn, dummy, dummy);
  return python_fillet_core(obj,r,fn,sel, minang);
}

PyObject *rotate_extrude_core(PyObject *obj,  int convexity, double scale, double angle, PyObject *twist, PyObject *origin, PyObject *offset, PyObject *vp, char *method, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<RotateExtrudeNode>(instance);
  node->profile_func = NULL;
  node->twist_func = NULL;
  if(obj->ob_type == &PyFunction_Type) {
    Py_XINCREF(obj); // TODO there to decref it ?
    node->profile_func = obj;
    auto dummy_node = std::make_shared<SquareNode>(instance);
    node->children.push_back(dummy_node);
  } else {
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
  node->scale = scale;
  node->angle = angle;
  if(twist!= NULL) {
  if(twist->ob_type == &PyFunction_Type) {
       	       Py_XINCREF(twist); // TODO there to decref it ?
	       node->twist_func = twist;
       }
       else node->twist=PyFloat_AsDouble(twist);
  }

  if (origin != NULL && PyList_Check(origin) && PyList_Size(origin) == 2) {
    node->origin_x = PyFloat_AsDouble(PyList_GetItem(origin, 0));
    node->origin_y = PyFloat_AsDouble(PyList_GetItem(origin, 1));
  }
  if (offset != NULL && PyList_Check(offset) && PyList_Size(offset) == 2) {
    node->offset_x = PyFloat_AsDouble(PyList_GetItem(offset, 0));
    node->offset_y = PyFloat_AsDouble(PyList_GetItem(offset, 1));
  }
  double dummy;
  Vector3d v(0,0,0);
  if(vp != nullptr && !python_vectorval(vp,3, 3, &v[0],&v[1],&v[2],&dummy )){
  }
  node->v = v;	  
  if(method != nullptr) node->method=method; else node->method = "centered";

  if (node->convexity <= 0) node->convexity = 2;
  if (node->scale <= 0) node->scale = 1;
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

  node->profile_func = NULL;
  node->twist_func = NULL;
  get_fnas(node->fn, node->fa, node->fs);
  if(obj->ob_type == &PyFunction_Type) {
        Py_XINCREF(obj); // TODO there to decref it ?
	node->profile_func = obj;
	node->fn=2;
  	auto dummy_node = std::make_shared<SquareNode>(instance);
	node->children.push_back(dummy_node);
  } else {
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
    node->has_heightvector=true;
  }

  node->convexity = convexity;

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

  if(twist!= NULL) {
	if(twist->ob_type == &PyFunction_Type){
                Py_XINCREF(twist); // TODO there to decref it ?
	       	node->twist_func = twist;
	}
	else node->twist=PyFloat_AsDouble(twist);
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

PyObject *path_extrude_core(PyObject *obj, PyObject *path, PyObject *xdir, int convexity, PyObject *origin, PyObject *scale, PyObject *twist, PyObject *closed, PyObject *allow_intersect, double fn, double fa, double fs)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<PathExtrudeNode>(instance);
  node->profile_func = NULL;
  node->twist_func = NULL;
  if(obj->ob_type == &PyFunction_Type) {
        Py_XINCREF(obj); // TODO there to decref it ?
	node->profile_func = obj;
  	auto dummy_node = std::make_shared<SquareNode>(instance);
	node->children.push_back(dummy_node);
  } else {
	  PyObject *dummydict;	  
	  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
	  if(child == NULL) {
        	PyErr_SetString(PyExc_TypeError,"Invalid type for  Object in path_extrude\n");
	   	return NULL;
	  }
	  node->children.push_back(child);
   }
   if(path != NULL && PyList_Check(path) ) {
	   int n=PyList_Size(path);
	   for(int i=0;i<n;i++) {
	  	PyObject *point=PyList_GetItem(path, i);
		double x,y,z,w=0;
  		if(python_vectorval(point,3, 4, &x,&y,&z,&w )){
			PyErr_SetString(PyExc_TypeError,"Cannot parse vector in path_extrude path\n");
			return NULL;
		}
		Vector4d pt3d(x,y,z,w);
		if(i > 0 &&  node->path[i-1] == pt3d) continue; //  prevent double pts
		node ->path.push_back(pt3d);
	   }
   }
   node->xdir_x=1;
   node->xdir_y=0;
   node->xdir_z=0;
   node->closed=false;
   if (closed == Py_True) node->closed = true;
   if (allow_intersect == Py_True) node->allow_intersect = true;
   if(xdir != NULL) {
	   if(python_vectorval(xdir,3, 3, &(node->xdir_x), &(node->xdir_y),&(node->xdir_z))) {
    		PyErr_SetString(PyExc_TypeError,"error in path_extrude xdir parameter\n");
		return NULL;
	   }
   }
   if(fabs(node->xdir_x) < 0.001 && fabs(node->xdir_y) < 0.001 && fabs(node->xdir_z) < 0.001) {
    		PyErr_SetString(PyExc_TypeError,"error in path_extrude xdir parameter has zero size\n");
		return NULL;
   }

   get_fnas(node->fn,node->fa,node->fs);
   if(fn != -1) node->fn=fn;
   if(fa != -1) node->fa=fa;
   if(fs != -1) node->fs=fs;

  node->convexity=convexity;

  node->origin_x=0.0; node->origin_y=0.0;
  if(origin != NULL) {
	  double dummy;
	  if(python_vectorval(origin,2, 2, &(node->origin_x), &(node->origin_y), &dummy)) {
    		PyErr_SetString(PyExc_TypeError,"error in path_extrude origin parameter\n");
		return NULL;
	  }
  }

  node->scale_x=1.0; node->scale_y=1.0;
  if(scale != NULL) {
	  double dummy;
	  if(python_vectorval(scale,2, 2, &(node->scale_x), &(node->scale_y), &dummy)) {
    		PyErr_SetString(PyExc_TypeError,"error in path_extrude scale parameter\n");
		return NULL;
	  }
  }

  if(scale != NULL && PyList_Check(scale) && PyList_Size(scale) == 2) {
	  node->scale_x=PyFloat_AsDouble(PyList_GetItem(scale, 0));
	  node->scale_y=PyFloat_AsDouble(PyList_GetItem(scale, 1));
  }
  if(twist!= NULL) {
       if(twist->ob_type == &PyFunction_Type){
                Py_XINCREF(twist); // TODO there to decref it ?
	        node->twist_func = twist;
	}
       else node->twist=PyFloat_AsDouble(twist);
       node->has_twist = 1;
  } else  node->has_twist=0;

  return PyOpenSCADObjectFromNode(&PyOpenSCADType,node);
}

PyObject *python_path_extrude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *obj = NULL;
  int convexity=1;
  PyObject *origin=NULL;
  PyObject *scale=NULL;
  PyObject *path=NULL;
  PyObject *xdir=NULL;
  PyObject *closed=NULL;
  PyObject *allow_intersect=NULL;
  PyObject *twist=NULL;
  double fn=-1, fa=-1, fs=-1;

  char * kwlist[] ={"obj","path","xdir","convexity","origin","scale","twist","closed","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!|O!iOOOOOddd", kwlist, 
                          &obj,
			  &PyList_Type, &path,
			  &PyList_Type,&xdir,
			  &convexity,
			  &origin,
			  &scale,
			  &twist,
			  &closed,
			  &allow_intersect,
			  &fn,&fs,&fs
                          )) {
        PyErr_SetString(PyExc_TypeError,"error during parsing\n");
        return NULL;
  }

  return path_extrude_core(obj, path, xdir, convexity, origin, scale, twist, closed, allow_intersect, fn, fa, fs);
}

PyObject *python_concat(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<ConcatNode>(instance);
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
        PyErr_SetString(PyExc_TypeError, "Error during concat. arguments must be solids");
	return nullptr;
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

PyObject *python_skin(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<SkinNode>(instance);
  PyObject *obj;
  PyObject *child_dict = nullptr;	  
  PyObject *dummy_dict = nullptr;	  
  std::shared_ptr<AbstractNode> child;
  if(kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str =  PyBytes_AS_STRING(value1);
      double tmp;
      if(value_str == nullptr) {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
	  return nullptr;
      } else if(strcmp(value_str,"convexity") == 0) {
	      python_numberval(value,&tmp);
	      node->convexity=(int) tmp;
      } else if(strcmp(value_str,"align_angle") == 0) {
	      python_numberval(value,&tmp);
	      node->align_angle=tmp;
	      node->has_align_angle=true;
      } else if(strcmp(value_str,"segments") == 0) {
	      python_numberval(value,&tmp);
	      node->has_segments=true;
	      node->segments=(int)tmp;
      } else if(strcmp(value_str,"interpolate") == 0) {
	      python_numberval(value,&tmp);
	      node->has_interpolate=true;
	      node->interpolate=tmp;
      } else {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in skin.");
	  return nullptr;
      }

    }	    
  }
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    if(i == 0) child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
    else child = PyOpenSCADObjectToNodeMulti(obj, &dummy_dict);
    if(child != NULL) {
      node->children.push_back(child);
    } else {
        PyErr_SetString(PyExc_TypeError, "Error during skin. arguments must be solids or arrays.");
	return nullptr;
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



PyObject *python_oo_path_extrude(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  int convexity=1;
  PyObject *origin=NULL;
  PyObject *scale=NULL;
  PyObject *path=NULL;
  PyObject *xdir=NULL;
  PyObject *closed=NULL;
  PyObject *allow_intersect=NULL;
  PyObject *twist=NULL;
  double fn=-1, fa=-1, fs=-1;

  char * kwlist[] ={"path","xdir","convexity","origin","scale","twist","closed","fn","fa","fs",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!iOOOOOddd", kwlist,
			  &PyList_Type, &path, &PyList_Type,&xdir, &convexity, &origin, &scale, &twist, &closed, &allow_intersect, &fn,&fs,&fs))
  {
        PyErr_SetString(PyExc_TypeError,"error during parsing\n");
        return NULL;
  }

  return path_extrude_core(obj, path, xdir, convexity, origin, scale, twist, closed, allow_intersect, fn, fa, fs);
}

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  node->r=0;
  node->fn=1;
  PyObject *obj;
  std::vector<PyObject *>child_dict;
  std::shared_ptr<AbstractNode> child;
  if(kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str =  PyBytes_AS_STRING(value1);
      if(value_str == nullptr) {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
	  return nullptr;
      } else if(strcmp(value_str,"r") == 0) {
	      python_numberval(value,&(node->r));
      } else if(strcmp(value_str,"fn") == 0) {
	      double fn;
	      python_numberval(value,&fn);
	      node->fn=(int)fn;
      } else {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
	  return nullptr;
      }

    }	    
  }
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    PyObject *dict = nullptr;	  
    child = PyOpenSCADObjectToNodeMulti(obj, &dict);
    child_dict.push_back(dict);
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
        case OpenSCADOperator::OFFSET:	    
      	  break;
      }
      return NULL;
    }
  }

  PyObject *pyresult =PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  for(int i=child_dict.size()-1;i>=0; i--) // merge from back  to give 1st child most priority
  {
    auto &dict = child_dict[i];	  
    if(dict == nullptr) continue;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(dict, &pos, &key, &value)) {
       PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
       const char *value_str =  PyBytes_AS_STRING(value1);
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
  node->r=0;
  node->fn=1;


  PyObject *obj;
  PyObject *child_dict;	  
  PyObject *dummy_dict;	  
  std::shared_ptr<AbstractNode> child;

  child = PyOpenSCADObjectToNodeMulti(self, &child_dict);
  if(child != NULL) node->children.push_back(child);
  
  if(kwargs != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
      PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
      const char *value_str =  PyBytes_AS_STRING(value1);
      if(value_str == nullptr) {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
	  return nullptr;
      } else if(strcmp(value_str,"r") == 0) {
	      python_numberval(value,&(node->r));
      } else if(strcmp(value_str,"fn") == 0) {
	      double fn;
	      python_numberval(value,&fn);
	      node->fn=(int)fn;
      } else {
          PyErr_SetString(PyExc_TypeError, "Unkown parameter name in CSG.");
	  return nullptr;
      }

    }	    
  }
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
        case OpenSCADOperator::OFFSET:	    
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
  python_retrieve_pyname(node);
  PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
  for(int i=1;i>=0;i--) {
    if(child_dict[i] != nullptr) {
      std::string name=child[i]->getPyName();
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while(PyDict_Next(child_dict[i], &pos, &key, &value)) {
        if(name.size() > 0) {	      
          PyObject *key1=PyUnicode_AsEncodedString(key, "utf-8", "~");
          const char *key_str =  PyBytes_AS_STRING(key1);
          std::string handle_name=name+"_"+key_str;
          PyObject *key_mod = PyUnicode_FromStringAndSize(handle_name.c_str(),strlen(handle_name.c_str()));
          PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key_mod, value);
	} else if(i == 0) {
          PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
	}

      }
    }
  }  
  return pyresult;
}

PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode) // 0: translate, 1: scale, 2: translateneg, 3=translate-exp
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict;	  

  child = PyOpenSCADObjectToNodeMulti(arg1, &child_dict);
  std::vector<Vector3d> vecs;
  int dragflags=0;
  if(mode == 3) {
    if(!PyList_Check(arg2)) {
      PyErr_SetString(PyExc_TypeError, "explode arg must be a list");
      return NULL;
    }
    int n=PyList_Size(arg2);
    if(PyList_Size(arg2) > 3) {
      PyErr_SetString(PyExc_TypeError, "explode arg list can have maximal 3 directions");
      return NULL;
    }
    double dmy;
    std::vector<float> vals[3];
    for(int i=0;i<3;i++) vals[i].push_back(0.0);
    for(int i=0;i<n;i++) {
      vals[i].clear();	    
      auto *item =PyList_GetItem(arg2,i);	    // TODO fix here
      if (!python_numberval(item, &dmy,&dragflags,1<<i)) vals[i].push_back(dmy);
      else if(PyList_Check(item)) {
        int m = PyList_Size(item);	      
	for(int j=0;j<m;j++) {
          auto *item1 =PyList_GetItem(item,j);	    
          if (!python_numberval(item1, &dmy)) vals[i].push_back(dmy);
	}  
      } else {
        PyErr_SetString(PyExc_TypeError, "Unknown explode spec");
        return NULL;
      }

    }
    for(auto z : vals[2])
      for(auto y : vals[1])
        for(auto x : vals[0])
          vecs.push_back(Vector3d(x,y,z));		
  } else vecs = python_vectors(arg2,2,3, &dragflags);

  if(mode == 0 && vecs.size() == 1) {
    PyObject *mat = python_matrix_trans(arg1,vecs[0]);
    if(mat != nullptr) return mat;
  }

  if (vecs.size() > 0) {
    if (child == NULL) {
      PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
      return NULL;
    }
    std::vector<std::shared_ptr<TransformNode>> nodes;
    for(size_t j=0;j<vecs.size();j++) {
      std::shared_ptr<TransformNode> node;
      switch(mode) {
        case 0:
        case 3:		
          node = std::make_shared<TransformNode>(instance, "translate");
          node->matrix.translate(vecs[j]);
	  break;
        case 1:
          node = std::make_shared<TransformNode>(instance, "scale");
          node->matrix.scale(vecs[j]);
          break;	  
        case 2:
          node = std::make_shared<TransformNode>(instance, "translate");
          node->matrix.translate(-vecs[j]);
          break;	  
      }
      node->children.push_back(child);
      nodes.push_back(node);
    }  
    if(nodes.size() == 1) {
      nodes[0]->dragflags = dragflags;	    
      PyObject *pyresult = PyOpenSCADObjectFromNode(&PyOpenSCADType, nodes[0]);
      if(child_dict != nullptr ) { 
        PyObject *key, *value;
        Py_ssize_t pos = 0;
         while(PyDict_Next(child_dict, &pos, &key, &value)) {
           PyObject *value1 = python_matrix_trans(value,vecs[0]);
           if(value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value1);
           else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
        }
      }
      return pyresult;
    }
    else {
      auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);
      DECLARE_INSTANCE
      for(auto x : nodes) node->children.push_back(x);
      return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
    }  
  }
  PyErr_SetString(PyExc_TypeError, "invalid argument right to operator");
  return NULL;
}

PyObject *python_nb_add(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 0); }  // translate
PyObject *python_nb_xor(PyObject *arg1, PyObject *arg2) { return python_nb_sub_vec3(arg1, arg2, 3); }
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

PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode)
{
  DECLARE_INSTANCE
  std::shared_ptr<AbstractNode> child;
  int i;
  PyObject *dummydict;	  

  auto node = std::make_shared<CgalAdvNode>(instance, mode);
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
        case CgalAdvType::RESIZE:	    
	  break;
        case CgalAdvType::MINKOWSKI:	    
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

  auto node = std::make_shared<CgalAdvNode>(instance, CgalAdvType::RESIZE);
  PyObject *dummydict;	  
  child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in resize");
    return NULL;
  }

  if (newsize != NULL) {
    double x, y, z;
    if (python_vectorval(newsize, 3, 3, &x, &y, &z)) {
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

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
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
#endif

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

#ifdef _WIN32
  std::string cur_dir = ".";
#else 
#ifdef __APPLE__
    std::string cur_dir = ".";
#else
  std::string cur_dir = get_current_dir_name();
#endif
#endif  
  std::string filename = lookup_file(fileval,  cur_dir, instance->location().filePath().parent_path().string());
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
                                   &script, &halign, &valign,
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
   }
 */
   node->params.detect_properties();

  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node);
}

PyObject *python_texture(PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE

  char *kwlist[] = {"file", "uv", NULL};
  char *texturename = NULL;
  double uv=10.0;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|f", kwlist,
                                   &texturename,&uv
                                   )) {
    PyErr_SetString(PyExc_TypeError, "error during parsing texture");
    return NULL;
  }
  TextureUV txt(texturename, uv);
  textures.push_back(txt);
  return Py_None;
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
  node->join_type = Clipper2Lib::JoinType::Round;
  if (!isnan(r)) {
    node->delta = r;
  } else if (!isnan(delta)) {
    node->delta = delta;
    node->join_type = Clipper2Lib::JoinType::Miter;
    if (chamfer == Py_True) {
      node->chamfer = true;
      node->join_type = Clipper2Lib::JoinType::Square;
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

PyObject *python_align_core(PyObject *obj, PyObject *pyrefmat, PyObject *pydstmat)
{
  if(obj->ob_type != &PyOpenSCADType) {
    PyErr_SetString(PyExc_TypeError, "Must specify Object as 1st parameter");
    return nullptr;
  }
  PyObject *child_dict=nullptr;	  
  std::shared_ptr<AbstractNode> dstnode = PyOpenSCADObjectToNode(obj, &child_dict);
  if(dstnode == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Invalid align object");
    return Py_None;
  }
  DECLARE_INSTANCE
  auto multmatnode = std::make_shared<TransformNode>(instance, "align");
  multmatnode->children.push_back(dstnode);
  Matrix4d mat;
  Matrix4d MT=Matrix4d::Identity();

  if(!python_tomatrix(pyrefmat, mat)) MT = MT * mat;	  
  if(!python_tomatrix(pydstmat, mat)) MT = MT * mat.inverse();	  

  multmatnode -> matrix = MT ;
  multmatnode->setPyName(dstnode->getPyName());

  PyObject *pyresult =PyOpenSCADObjectFromNode(&PyOpenSCADType, multmatnode);
  if(child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
     while(PyDict_Next(child_dict, &pos, &key, &value)) {
//       PyObject* value1 = PyUnicode_AsEncodedString(key, "utf-8", "~");
//       const char *value_str =  PyBytes_AS_STRING(value1);
       if(!python_tomatrix(value, mat)){
         mat = MT * mat;
         PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, python_frommatrix(mat));
       } else PyDict_SetItem(((PyOpenSCADObject *) pyresult)->dict,key, value);
    }
  }
  return pyresult;
}

PyObject *python_align(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj","refmat","objmat",NULL};
  PyObject *obj=NULL;
  PyObject *pyrefmat=NULL;
  PyObject *pyobjmat=NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O", kwlist,
                                   &obj, 
				   &pyrefmat,
				   &pyobjmat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat);
}

PyObject *python_oo_align(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"refmat","objmat",NULL};
  PyObject *pyrefmat=NULL;
  PyObject *pyobjmat=NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist,
				   &pyrefmat,
				   &pyobjmat
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during align");
    return NULL;
  }
  return python_align_core(obj, pyrefmat, pyobjmat);
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

#ifdef _WIN32
  std::string cur_dir = ".";
#else 
#ifdef __APPLE__
    std::string cur_dir = ".";
#else
  std::string cur_dir = get_current_dir_name();
#endif
#endif  
  filename = lookup_file(v == NULL ? "" : v, cur_dir, instance->location().filePath().parent_path().string());
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
    else if (ext == ".stp") actualtype = ImportType::STEP;
    else if (ext == ".step") actualtype = ImportType::STEP;
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
  return do_import_python(self, args, kwargs, ImportType::UNKNOWN);
}



#ifndef OPENSCAD_NOGUI
extern int curl_download(std::string url, std::string path);
PyObject *python_nimport(PyObject *self, PyObject *args, PyObject *kwargs)
{
  static bool called_already=false;	
  char *kwlist[] = {"url", NULL};
  const char *c_url = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist,
                                   &c_url)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing nimport(filename)");
    return NULL;
  }
  if(c_url == nullptr) return Py_None;

  std::string url = c_url;
  std::string filename , path, importcode;
  filename = url.substr(url.find_last_of("/") + 1);
  importcode = "from "+filename.substr(0,filename.find_last_of("."))+" import *";

  path =PlatformUtils::userLibraryPath() + "/" + filename;
  bool do_download=false; 
  if(!called_already) {
    do_download=true;	  
  }
  called_already=true; // TODO per file 
  std::ifstream f(path.c_str());
  if(!f.good()) {
    do_download=true;	  
  }

  if(do_download) {
    curl_download(url, path);	  
	  
  }

  PyRun_SimpleString(importcode.c_str());
  return Py_None;
}
#endif

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
//    annotationList.push_back(Annotation("Parameter",std::make_shared<Literal>("Parameter")));
//    annotationList.push_back(Annotation("Description",std::make_shared<Literal>("Description")));
//    annotationList.push_back(Annotation("Group",std::make_shared<Literal>("Group")));
    auto assignment = std::make_shared<Assignment>(name,lit);
//    assignment->addAnnotations(&annotationList);
    customizer_parameters.push_back(assignment);
    PyObject *value_effective = value;
    for(unsigned int i=0;i<customizer_parameters_finished.size();i++) {
      if(customizer_parameters_finished[i]->getName() == name)
      {
        auto expr = customizer_parameters_finished[i]->getExpr();
        const auto &lit=std::dynamic_pointer_cast<Literal>(expr);
	if(lit != nullptr) {
	  if(lit->isDouble()) value_effective=PyFloat_FromDouble(lit->toDouble());
          if(lit->isString()) value_effective=PyUnicode_FromString(lit->toString().c_str());
	  }  
      }
    }
    PyObject *maindict = PyModule_GetDict(pythonMainModule.get());
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
  parsed_file->handleDependencies(true);

  EvaluationSession session{"python"};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> resultnode = parsed_file->instantiate(*builtin_context, &file_context);
  resultnode = resultnode->clone(); // instmod will go out of scope
  delete parsed_file;
  parsed_file = nullptr;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType,resultnode);
}

PyObject *python_osuse_include(int mode, PyObject *self, PyObject *args, PyObject *kwargs)
{
  DECLARE_INSTANCE
  auto empty = std::make_shared<CubeNode>(instance);
  char *kwlist[] = {"file", NULL};
  const char *file = NULL;
  std::ostringstream stream;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist,
                                   &file
                                   )) {
    if(mode) PyErr_SetString(PyExc_TypeError, "Error during parsing osinclude(path)");
    else PyErr_SetString(PyExc_TypeError, "Error during parsing osuse(path)");
    return NULL;
  }
  const std::string filename = lookup_file(file, ".",".");
  stream << "include <" << filename << ">\n";

  SourceFile *source;
  if(!parse(source, stream.str(), "python", "python", false)) {
    PyErr_SetString(PyExc_TypeError, "Error in SCAD code");
    return Py_None;
  }
  if(mode == 0) source->scope.moduleInstantiations.clear();	  
  source->handleDependencies(true);

  EvaluationSession *session = new EvaluationSession("python");
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(session)};

  std::shared_ptr<const FileContext> osinclude_context;
  std::shared_ptr<AbstractNode> resultnode = source->instantiate(*builtin_context, &osinclude_context); // TODO keine globakle var, kollision!

  LocalScope scope = source->scope;
  PyOpenSCADObject *result = (PyOpenSCADObject *) PyOpenSCADObjectFromNode(&PyOpenSCADType, empty); 

  for(auto mod : source->scope.modules) { // copy modules
    std::shared_ptr<UserModule> usmod = mod.second;
    InstantiableModule m;
//    m.defining_context=osinclude_context;
//    m.module=mod.second.get();
//    boost::optional<InstantiableModule> res(m);
    PyDict_SetItemString(result->dict, mod.first.c_str(),PyDataObjectFromModule(&PyDataType, filename, mod.first ));
  }

 for(auto fun : source->scope.functions) { // copy functions
    std::shared_ptr<UserFunction> usfunc = fun.second; // install lambda functions ?
//    printf("%s\n",fun.first.c_str());
//    InstantiableModule m;
//    m.defining_context=osinclude_context;
//    m.module=mod.second.get();
//    boost::optional<InstantiableModule> res(m);
//    PyDict_SetItemString(result->dict, mod.first.c_str(),PyDataObjectFromModule(&PyDataType, res ));
  }

  for(auto ass : source->scope.assignments) { // copy assignments
//    printf("Var %s\n",ass->getName().c_str());						   
    const std::shared_ptr<Expression> expr = ass->getExpr();
    Value val = expr->evaluate(osinclude_context);
    if(val.isDefined()) {
      PyObject *res= python_fromopenscad(std::move(val));
      PyDict_SetItemString(result->dict, ass->getName().c_str(),res);
    }
  }
 // SourceFileCache::instance()->clear();
  return (PyObject *) result;
}

PyObject *python_osuse(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_osuse_include(0,self, args, kwargs);
}

PyObject *python_osinclude(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_osuse_include(1,self, args, kwargs);
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
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, node); // TODO 1st loswerden
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

#ifndef OPENSCAD_NOGUI
extern void  add_menuitem_trampoline(const char *menuname, const char *itemname, const char *callback);
PyObject *python_add_menuitem(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"menuname","itemname","callback", NULL};
  const char *menuname = nullptr, *itemname = nullptr, *callback = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss", kwlist,
                                   &menuname, &itemname, &callback
                                   )) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing add_menuitem");
    return NULL;
  }
  add_menuitem_trampoline(menuname, itemname, callback);
  return Py_None;
}
#endif

PyObject *python_model(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing model");
    return NULL;
  }
  if(python_result_node == nullptr) return Py_None;
  return PyOpenSCADObjectFromNode(&PyOpenSCADType, python_result_node);
}

PyObject *python_modelpath(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing model");
    return NULL;
  }
  return PyUnicode_FromString(python_scriptpath.c_str());
}

PyMethodDef PyOpenSCADFunctions[] = {
  {"edge", (PyCFunction) python_edge, METH_VARARGS | METH_KEYWORDS, "Create Edge."},
  {"square", (PyCFunction) python_square, METH_VARARGS | METH_KEYWORDS, "Create Square."},
  {"circle", (PyCFunction) python_circle, METH_VARARGS | METH_KEYWORDS, "Create Circle."},
  {"polygon", (PyCFunction) python_polygon, METH_VARARGS | METH_KEYWORDS, "Create Polygon."},
  {"spline", (PyCFunction) python_spline, METH_VARARGS | METH_KEYWORDS, "Create Spline."},
  {"text", (PyCFunction) python_text, METH_VARARGS | METH_KEYWORDS, "Create Text."},
  {"textmetrics", (PyCFunction) python_textmetrics, METH_VARARGS | METH_KEYWORDS, "Get textmetrics."},

  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"cylinder", (PyCFunction) python_cylinder, METH_VARARGS | METH_KEYWORDS, "Create Cylinder."},
  {"sphere", (PyCFunction) python_sphere, METH_VARARGS | METH_KEYWORDS, "Create Sphere."},
  {"polyhedron", (PyCFunction) python_polyhedron, METH_VARARGS | METH_KEYWORDS, "Create Polyhedron."},
#ifdef ENABLE_LIBFIVE  
  {"frep", (PyCFunction) python_frep, METH_VARARGS | METH_KEYWORDS, "Create F-Rep."},
  {"ifrep", (PyCFunction) python_ifrep, METH_VARARGS | METH_KEYWORDS, "Create Inverse F-Rep."},
#endif  

  {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"right", (PyCFunction) python_right, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"left", (PyCFunction) python_left, METH_VARARGS | METH_KEYWORDS, "Move Left Object."},
  {"back", (PyCFunction) python_back, METH_VARARGS | METH_KEYWORDS, "Move Back Object."},
  {"front", (PyCFunction) python_front, METH_VARARGS | METH_KEYWORDS, "Move Front Object."},
  {"up", (PyCFunction) python_up, METH_VARARGS | METH_KEYWORDS, "Move Up Object."},
  {"down", (PyCFunction) python_down, METH_VARARGS | METH_KEYWORDS, "Move Down Object."},
  {"rotx", (PyCFunction) python_rotx, METH_VARARGS | METH_KEYWORDS, "Rotate X Object."},
  {"roty", (PyCFunction) python_roty, METH_VARARGS | METH_KEYWORDS, "Rotate Y Object."},
  {"rotz", (PyCFunction) python_rotz, METH_VARARGS | METH_KEYWORDS, "Rotate Z Object."},
  {"rotate", (PyCFunction) python_rotate, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction) python_scale, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"mirror", (PyCFunction) python_mirror, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction) python_multmatrix, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"divmatrix", (PyCFunction) python_divmatrix, METH_VARARGS | METH_KEYWORDS, "Divmatrix Object."},
  {"offset", (PyCFunction) python_offset, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  {"roof", (PyCFunction) python_roof, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
#endif
  {"pull", (PyCFunction) python_pull, METH_VARARGS | METH_KEYWORDS, "Pull apart Object."},
  {"wrap", (PyCFunction) python_wrap, METH_VARARGS | METH_KEYWORDS, "Wrap Object around cylidner."},
  {"color", (PyCFunction) python_color, METH_VARARGS | METH_KEYWORDS, "Color Object."},
  {"output", (PyCFunction) python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"show", (PyCFunction) python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {"separate", (PyCFunction) python_separate, METH_VARARGS | METH_KEYWORDS, "Split into separate parts."},
  {"export", (PyCFunction) python_export, METH_VARARGS | METH_KEYWORDS, "Export the result."},
  {"find_face", (PyCFunction) python_find_face, METH_VARARGS | METH_KEYWORDS, "find_face."},
  {"sitonto", (PyCFunction) python_sitonto, METH_VARARGS | METH_KEYWORDS, "sitonto"},

  {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},
  {"path_extrude", (PyCFunction) python_path_extrude, METH_VARARGS | METH_KEYWORDS, "Path_extrude Object."},
  {"skin", (PyCFunction) python_skin, METH_VARARGS | METH_KEYWORDS, "Path_extrude Object."},

  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"hull", (PyCFunction) python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction) python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction) python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction) python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},
  {"concat", (PyCFunction) python_concat, METH_VARARGS | METH_KEYWORDS, "Concatenate Object."},

  {"highlight", (PyCFunction) python_highlight, METH_VARARGS | METH_KEYWORDS, "Highlight Object."},
  {"background", (PyCFunction) python_background, METH_VARARGS | METH_KEYWORDS, "Background Object."},
  {"only", (PyCFunction) python_only, METH_VARARGS | METH_KEYWORDS, "Only Object."},

  {"projection", (PyCFunction) python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction) python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},
  {"texture", (PyCFunction) python_texture, METH_VARARGS | METH_KEYWORDS, "Include a texture."},
  {"mesh", (PyCFunction) python_mesh, METH_VARARGS | METH_KEYWORDS, "exports mesh."},
  {"faces", (PyCFunction) python_faces, METH_VARARGS | METH_KEYWORDS, "exports a list of faces."},
  {"edges", (PyCFunction) python_edges, METH_VARARGS | METH_KEYWORDS, "exports a list of edges from a face."},
  {"oversample", (PyCFunction) python_oversample, METH_VARARGS | METH_KEYWORDS, "oversample."},
  {"debug", (PyCFunction) python_debug, METH_VARARGS | METH_KEYWORDS, "debug a face."},
  {"fillet", (PyCFunction) python_fillet, METH_VARARGS | METH_KEYWORDS, "fillet."},

  {"group", (PyCFunction) python_group, METH_VARARGS | METH_KEYWORDS, "Group Object."},
  {"render", (PyCFunction) python_render, METH_VARARGS | METH_KEYWORDS, "Render Object."},
  {"osimport", (PyCFunction) python_import, METH_VARARGS | METH_KEYWORDS, "Import Object."},
  {"osuse", (PyCFunction) python_osuse, METH_VARARGS | METH_KEYWORDS, "Use OpenSCAD Library."},
  {"osinclude", (PyCFunction) python_osinclude, METH_VARARGS | METH_KEYWORDS, "Include OpenSCAD Library."},
  {"version", (PyCFunction) python_osversion, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"version_num", (PyCFunction) python_osversion_num, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"add_parameter", (PyCFunction) python_add_parameter, METH_VARARGS | METH_KEYWORDS, "Add Parameter for Customizer."},
  {"scad", (PyCFunction) python_scad, METH_VARARGS | METH_KEYWORDS, "Source OpenSCAD code."},
  {"align", (PyCFunction) python_align, METH_VARARGS | METH_KEYWORDS, "Align Object to another."},
#ifndef OPENSCAD_NOGUI  
  {"add_menuitem", (PyCFunction) python_add_menuitem, METH_VARARGS | METH_KEYWORDS, "Add Menuitem to the the openscad window."},
  {"nimport", (PyCFunction) python_nimport, METH_VARARGS | METH_KEYWORDS, "Import Networked Object."},
#endif  
  {"model", (PyCFunction) python_model, METH_VARARGS | METH_KEYWORDS, "Yield Model"},
  {"modelpath", (PyCFunction) python_modelpath, METH_VARARGS | METH_KEYWORDS, "Returns absolute Path to script"},
  {"marked", (PyCFunction) python_marked, METH_VARARGS | METH_KEYWORDS, "Create a marked value."},
  {NULL, NULL, 0, NULL}
};

#define	OO_METHOD_ENTRY(name,desc) \
  {#name, (PyCFunction) python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {
  OO_METHOD_ENTRY(translate,"Move Object")
  OO_METHOD_ENTRY(rotate,"Rotate Object")	
  OO_METHOD_ENTRY(right,"Right Object")	
  OO_METHOD_ENTRY(left,"Left Object")	
  OO_METHOD_ENTRY(back,"Back Object")	
  OO_METHOD_ENTRY(front,"Front Object")	
  OO_METHOD_ENTRY(up,"Up Object")	
  OO_METHOD_ENTRY(down,"Lower Object")	

  OO_METHOD_ENTRY(union,"Union Object")	
  OO_METHOD_ENTRY(difference,"Difference Object")	
  OO_METHOD_ENTRY(intersection,"Intersection Object")

  OO_METHOD_ENTRY(rotx,"Rotx Object")	
  OO_METHOD_ENTRY(roty,"Roty Object")	
  OO_METHOD_ENTRY(rotz,"Rotz Object")	

  OO_METHOD_ENTRY(scale,"Scale Object")	
  OO_METHOD_ENTRY(mirror,"Mirror Object")	
  OO_METHOD_ENTRY(multmatrix,"Multmatrix Object")	
  OO_METHOD_ENTRY(divmatrix,"Divmatrix Object")	
  OO_METHOD_ENTRY(offset,"Offset Object")	
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  OO_METHOD_ENTRY(roof,"Roof Object")	
#endif
  OO_METHOD_ENTRY(color,"Color Object")	
  OO_METHOD_ENTRY(separate,"Split into separate Objects")	
  OO_METHOD_ENTRY(export,"Export Object")	
  OO_METHOD_ENTRY(find_face,"Find Face")	
  OO_METHOD_ENTRY(sitonto,"Sit onto")	

  OO_METHOD_ENTRY(linear_extrude,"Linear_extrude Object")	
  OO_METHOD_ENTRY(rotate_extrude,"Rotate_extrude Object")	
  OO_METHOD_ENTRY(path_extrude,"Path_extrude Object")
  OO_METHOD_ENTRY(resize,"Resize Object")	

  OO_METHOD_ENTRY(mesh, "Mesh Object")	
  OO_METHOD_ENTRY(faces, "Create Faces list")	
  OO_METHOD_ENTRY(edges, "Create Edges list")	
  OO_METHOD_ENTRY(oversample,"Oversample Object")	
  OO_METHOD_ENTRY(debug,"Debug Object Faces")	
  OO_METHOD_ENTRY(fillet,"Fillet Object")	
  OO_METHOD_ENTRY(align,"Align Object to another")	

  OO_METHOD_ENTRY(highlight,"Highlight Object")	
  OO_METHOD_ENTRY(background,"Background Object")	
  OO_METHOD_ENTRY(only,"Only Object")	
  OO_METHOD_ENTRY(show,"Show Object")	
  OO_METHOD_ENTRY(projection,"Projection Object")	
  OO_METHOD_ENTRY(pull,"Pull Obejct apart")	
  OO_METHOD_ENTRY(wrap,"Wrap Object around Cylinder")	
  OO_METHOD_ENTRY(render,"Render Object")	
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
     python_nb_neg,	//unaryfunc nb_negative
     python_nb_pos,	//unaryfunc nb_positive
     0,			//unaryfunc nb_absolute
     0,			//inquiry nb_bool
     python_nb_invert,  //unaryfunc nb_invert
     0,			//binaryfunc nb_lshift
     0,			//binaryfunc nb_rshift
     python_nb_and,	//binaryfunc nb_and 
     python_nb_xor,      //binaryfunc nb_xor
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

