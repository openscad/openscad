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
#include "src/python/pyopenscad.h"
#include "src/core/CsgOpNode.h"
#include "src/core/TransformNode.h"

#include "src/core/primitives.h"

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
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in output");
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

PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode)
{
  DECLARE_INSTANCE
  int i;

  auto node = std::make_shared<CsgOpNode>(instance, mode);
  PyObject *obj;
  std::shared_ptr<AbstractNode> child;
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    if(i == 0) child = PyOpenSCADObjectToNodeMulti(obj);
    else child = PyOpenSCADObjectToNodeMulti(obj);
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
  std::shared_ptr<AbstractNode> child;

  child = PyOpenSCADObjectToNodeMulti(self);
  if(child != NULL) node->children.push_back(child);
  
  for (i = 0; i < PyTuple_Size(args);i++) {
    obj = PyTuple_GetItem(args, i);
    if(i == 0) child = PyOpenSCADObjectToNodeMulti(obj);
    else child = PyOpenSCADObjectToNodeMulti(obj);
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

  if(arg1 == Py_None && mode == OpenSCADOperator::UNION) return arg2;
  if(arg2 == Py_None && mode == OpenSCADOperator::UNION) return arg1;
  if(arg2 == Py_None && mode == OpenSCADOperator::DIFFERENCE) return arg1;


  child[0] = PyOpenSCADObjectToNodeMulti(arg1);
  if (child[0] == NULL) {
    PyErr_SetString(PyExc_TypeError, "invalid argument left to operator");
    return NULL;
  }
  child[1] = PyOpenSCADObjectToNodeMulti(arg2);
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

  child = PyOpenSCADObjectToNodeMulti(arg1);
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

PyMethodDef PyOpenSCADFunctions[] = {
  {"cube", (PyCFunction) python_cube, METH_VARARGS | METH_KEYWORDS, "Create Cube."},
  {"translate", (PyCFunction) python_translate, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"show", (PyCFunction) python_show, METH_VARARGS | METH_KEYWORDS, "Show the result."},
  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {NULL, NULL, 0, NULL}
};

#define	OO_METHOD_ENTRY(name,desc) \
  {#name, (PyCFunction) python_oo_##name, METH_VARARGS | METH_KEYWORDS, desc},

PyMethodDef PyOpenSCADMethods[] = {
  OO_METHOD_ENTRY(translate,"Move Object")	
  OO_METHOD_ENTRY(union,"Union Object")	
  OO_METHOD_ENTRY(difference,"Difference Object")	
  OO_METHOD_ENTRY(intersection,"Intersection Object")	
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

