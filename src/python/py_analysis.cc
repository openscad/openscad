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
#include <TransformNode.h>
#include <Tree.h>
#include <GeometryEvaluator.h>
#include <PolySetUtils.h>
#include <primitives.h>
#include "pyfunctions.h"

PyObject *python_mesh_core(PyObject *obj, bool tessellate, bool color)
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

  if (ps != nullptr) {
    if (tessellate == true) {
      ps = PolySetUtils::tessellate_faces(*ps);
    }
    // Now create Python Point array
    PyObject *ptarr = PyList_New(ps->vertices.size());
    for (unsigned int i = 0; i < ps->vertices.size(); i++) {
      PyObject *coord = PyList_New(3);
      for (int j = 0; j < 3; j++) PyList_SetItem(coord, j, PyFloat_FromDouble(ps->vertices[i][j]));
      PyList_SetItem(ptarr, i, coord);
      Py_XINCREF(coord);
    }
    Py_XINCREF(ptarr);
    // Now create Python Point array
    PyObject *polarr = PyList_New(ps->indices.size());
    for (unsigned int i = 0; i < ps->indices.size(); i++) {
      PyObject *face = PyList_New(ps->indices[i].size());
      for (unsigned int j = 0; j < ps->indices[i].size(); j++)
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
    const std::vector<Outline2d> outlines = polygon2d->outlines();
    PyObject *pyth_outlines = PyList_New(outlines.size());
    for (unsigned int i = 0; i < outlines.size(); i++) {
      const Outline2d& outline = outlines[i];
      int items = outline.vertices.size();
      if (color) items++;
      PyObject *pyth_outline = PyList_New(items);
      int ind = 0;
      if (color) {
        PyObject *pyth_col = PyList_New(4);
        PyList_SetItem(pyth_col, 0, PyFloat_FromDouble(outline.color.r()));
        PyList_SetItem(pyth_col, 1, PyFloat_FromDouble(outline.color.g()));
        PyList_SetItem(pyth_col, 2, PyFloat_FromDouble(outline.color.b()));
        PyList_SetItem(pyth_col, 3, PyFloat_FromDouble(outline.color.a()));
        PyList_SetItem(pyth_outline, ind++, pyth_col);
      }
      for (unsigned int j = 0; j < outline.vertices.size(); j++) {
        Vector2d pt = outline.vertices[j];
        PyObject *pyth_pt = PyList_New(2);
        for (int k = 0; k < 2; k++) PyList_SetItem(pyth_pt, k, PyFloat_FromDouble(pt[k]));
        PyList_SetItem(pyth_outline, ind++, pyth_pt);
      }
      PyList_SetItem(pyth_outlines, i, pyth_outline);
    }
    return pyth_outlines;
  }
  Py_RETURN_NONE;
}

PyObject *python_mesh(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "triangulate", "color", NULL};
  PyObject *obj = NULL;
  PyObject *tess = NULL;
  PyObject *color = Py_False;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO", kwlist, &obj, &tess, &color)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True, color == Py_True);
}

PyObject *python_oo_mesh(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"triangulate", "color", NULL};
  PyObject *tess = NULL;
  PyObject *color = Py_False;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &tess, &color)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_mesh_core(obj, tess == Py_True, color == Py_True);
}

PyObject *python_inside_core(PyObject *pyobj, PyObject *pypoint)
{
  PyObject *dummydict;
  Vector3d vec3;
  std::shared_ptr<AbstractNode> node = PyOpenSCADObjectToNode(pyobj, &dummydict);
  if (node == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Object must be a solid\n");
    return nullptr;
  }
  if (python_vectorval(pypoint, 1, 3, &(vec3[0]), &(vec3[1]), &(vec3[2]), nullptr)) {
    PyErr_SetString(PyExc_TypeError, "must specify a point to check\n");
    return nullptr;
  }
  const std::shared_ptr<const PolygonNode> polygonnode =
    std::dynamic_pointer_cast<const PolygonNode>(node);
  if (polygonnode != nullptr) {
    auto geom = polygonnode->createGeometry();
    const Polygon2d poly2 = dynamic_cast<const Polygon2d&>(*geom);
    Vector2d vec2(vec3[0], vec3[1]);
    if (poly2.point_inside(vec2)) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
  }

  const std::shared_ptr<const PolyhedronNode> polyhedronnode =
    std::dynamic_pointer_cast<const PolyhedronNode>(node);
  if (polyhedronnode != nullptr) {
    auto geom = polyhedronnode->createGeometry();
    const PolySet ps = dynamic_cast<const PolySet&>(*geom);
    if (ps.point_inside(vec3)) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
  }

  Tree tree(node, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (auto poly2 = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    Vector2d vec2(vec3[0], vec3[1]);
    if (poly2->point_inside(vec2)) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
  }
  if (ps != nullptr) {
    if (ps->point_inside(vec3)) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
  }
  Py_RETURN_NONE;
}

PyObject *python_inside(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "point", NULL};
  PyObject *obj = NULL;
  PyObject *pypoint;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &pypoint)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_inside_core(obj, pypoint);
}

PyObject *python_oo_inside(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"point", NULL};
  PyObject *pypoint;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &pypoint)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_inside_core(obj, pypoint);
}

PyObject *python_bbox_core(PyObject *obj)
{
  // Get position and size attributes from the object
  PyObject *position_key = PyUnicode_FromString("position");
  PyObject *size_key = PyUnicode_FromString("size");

  PyObject *position = python__getitem__(obj, position_key);
  PyObject *size = python__getitem__(obj, size_key);

  Py_DECREF(position_key);
  Py_DECREF(size_key);

  if (position == Py_None || size == Py_None) {
    if (position != Py_None) Py_DECREF(position);
    if (size != Py_None) Py_DECREF(size);
    Py_RETURN_NONE;
  }

  // Create bounding shape: square for 2D, cube for 3D
  PyObject *bbox_args = PyTuple_New(0);
  PyObject *bbox_kwargs = PyDict_New();
  bool is_2d = PyList_Check(size) && PyList_Size(size) == 2;
  PyDict_SetItemString(bbox_kwargs, is_2d ? "dim" : "size", size);
  PyDict_SetItemString(bbox_kwargs, "center", Py_False);

  PyObject *bbox_shape =
    is_2d ? python_square(NULL, bbox_args, bbox_kwargs) : python_cube(NULL, bbox_args, bbox_kwargs);
  if (bbox_shape == NULL) {
    Py_DECREF(position);
    Py_DECREF(size);
    Py_DECREF(bbox_args);
    Py_DECREF(bbox_kwargs);
    return NULL;
  }

  // Translate shape to the object's position
  PyObject *bbox_box = python_translate_core(bbox_shape, position);

  // Clean up
  Py_DECREF(position);
  Py_DECREF(size);
  Py_DECREF(bbox_args);
  Py_DECREF(bbox_kwargs);
  Py_DECREF(bbox_shape);

  return bbox_box;
}

PyObject *python_bbox(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_bbox_core(obj);
}

PyObject *python_oo_bbox(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_bbox_core(obj);
}

PyObject *python_size_core(PyObject *obj)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    Py_RETURN_NONE;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);

  // Handle 2D geometry (Polygon2d)
  if (auto poly2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    BoundingBox bbox = poly2d->getBoundingBox();
    Vector3d bmin = bbox.min();
    Vector3d bmax = bbox.max();
    PyObject *size_list = PyList_New(2);
    PyList_SetItem(size_list, 0, PyFloat_FromDouble(bmax[0] - bmin[0]));
    PyList_SetItem(size_list, 1, PyFloat_FromDouble(bmax[1] - bmin[1]));
    Py_INCREF(size_list);
    return size_list;
  }

  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (ps != nullptr && ps->vertices.size() > 0) {
    Vector3d pmin = ps->vertices[0];
    Vector3d pmax = pmin;
    for (const auto& pt : ps->vertices) {
      for (int i = 0; i < 3; i++) {
        if (pt[i] > pmax[i]) pmax[i] = pt[i];
        if (pt[i] < pmin[i]) pmin[i] = pt[i];
      }
    }
    // Calculate size directly
    Vector3d size = pmax - pmin;
    PyObject *size_list = python_fromvector(size);
    Py_INCREF(size_list);
    return size_list;
  }
  Py_RETURN_NONE;
}

PyObject *python_position_core(PyObject *obj)
{
  PyObject *dummydict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    Py_RETURN_NONE;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);

  // Handle 2D geometry (Polygon2d)
  if (auto poly2d = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    BoundingBox bbox = poly2d->getBoundingBox();
    Vector3d bmin = bbox.min();
    PyObject *position_list = PyList_New(2);
    PyList_SetItem(position_list, 0, PyFloat_FromDouble(bmin[0]));
    PyList_SetItem(position_list, 1, PyFloat_FromDouble(bmin[1]));
    Py_INCREF(position_list);
    return position_list;
  }

  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (ps != nullptr && ps->vertices.size() > 0) {
    Vector3d pmin = ps->vertices[0];
    for (const auto& pt : ps->vertices) {
      for (int i = 0; i < 3; i++) {
        if (pt[i] < pmin[i]) pmin[i] = pt[i];
      }
    }
    PyObject *position_list = python_fromvector(pmin);
    Py_INCREF(position_list);
    return position_list;
  }
  Py_RETURN_NONE;
}

PyObject *python_size(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing size(obj)\n");
    return NULL;
  }
  return python_size_core(obj);
}

PyObject *python_position(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing position(obj)\n");
    return NULL;
  }
  return python_position_core(obj);
}

PyObject *python_separate_core(PyObject *obj)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in separate \n");
    return NULL;
  }
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);

  if (ps != nullptr) {
    // setup databases
    intList empty_list;
    std::vector<intList> pt2tri;

    std::vector<int> vert_db;
    for (size_t i = 0; i < ps->vertices.size(); i++) {
      vert_db.push_back(-1);
      pt2tri.push_back(empty_list);
    }

    std::vector<int> tri_db;
    for (size_t i = 0; i < ps->indices.size(); i++) {
      tri_db.push_back(-1);
      for (auto ind : ps->indices[i]) pt2tri[ind].push_back(i);
    }

    // now sort for objects
    int obj_num = 0;
    for (size_t i = 0; i < vert_db.size(); i++) {
      if (vert_db[i] != -1) continue;
      std::vector<int> vert_todo;
      vert_todo.push_back(i);
      while (vert_todo.size() > 0) {
        int vert_ind = vert_todo[vert_todo.size() - 1];
        vert_todo.pop_back();
        if (vert_db[vert_ind] != -1) continue;
        vert_db[vert_ind] = obj_num;
        for (int tri_ind : pt2tri[vert_ind]) {
          if (tri_db[tri_ind] != -1) continue;
          tri_db[tri_ind] = obj_num;
          for (int vert1_ind : ps->indices[tri_ind]) {
            if (vert_db[vert1_ind] != -1) continue;
            vert_todo.push_back(vert1_ind);
          }
        }
      }
      obj_num++;
    }

    PyObject *objects = PyList_New(obj_num);
    for (int i = 0; i < obj_num; i++) {
      // create a polyhedron for each
      DECLARE_INSTANCE();
      auto node = std::make_shared<PolyhedronNode>(instance);
      node->convexity = 2;
      std::vector<int> vert_map;
      for (size_t j = 0; j < ps->vertices.size(); j++) {
        if (vert_db[j] == i) {
          vert_map.push_back(node->points.size());
          node->points.push_back(ps->vertices[j]);
        } else vert_map.push_back(-1);
      }
      for (size_t j = 0; j < ps->indices.size(); j++) {
        if (tri_db[j] == i) {
          IndexedFace face_map;
          for (auto ind : ps->indices[j]) {
            face_map.push_back(vert_map[ind]);
          }
          std::reverse(face_map.begin(), face_map.end());
          node->faces.push_back(face_map);
        }
      }
      PyList_SetItem(objects, i, PyOpenSCADObjectFromNode(type, node));
    }
    return objects;
  }
  Py_RETURN_NONE;
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
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_separate_core(obj);
}

PyObject *python_edges_core(PyObject *obj)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in faces \n");
    return NULL;
  }

  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  const std::shared_ptr<const Polygon2d> poly = std::dynamic_pointer_cast<const Polygon2d>(geom);
  if (poly == nullptr) Py_RETURN_NONE;
  int edgenum = 0;
  Vector3d zdir(0, 0, 0);
  Transform3d trans = poly->getTransform3d();
  for (auto ol : poly->untransformedOutlines()) {
    int n = ol.vertices.size();
    for (int i = 0; i < n; i++) {
      Vector3d p1 = trans * Vector3d(ol.vertices[i][0], ol.vertices[i][1], 0);
      Vector3d p2 = trans * Vector3d(ol.vertices[(i + 1) % n][0], ol.vertices[(i + 1) % n][1], 0);
      Vector3d p3 = trans * Vector3d(ol.vertices[(i + 2) % n][0], ol.vertices[(i + 2) % n][1], 0);
      zdir += (p1 - p2).cross(p2 - p3);
    }
    edgenum += n;
  }
  zdir.normalize();
  PyObject *pyth_edges = PyList_New(edgenum);
  int ind = 0;

  for (auto ol : poly->untransformedOutlines()) {
    int n = ol.vertices.size();
    for (int i = 0; i < n; i++) {
      Vector3d p1 = trans * Vector3d(ol.vertices[i][0], ol.vertices[i][1], 0);
      Vector3d p2 = trans * Vector3d(ol.vertices[(i + 1) % n][0], ol.vertices[(i + 1) % n][1], 0);
      Vector3d pt = (p1 + p2) / 2.0;
      Vector3d xdir = (p2 - p1).normalized();
      Vector3d ydir = xdir.cross(zdir).normalized();

      Matrix4d mat;
      mat << xdir[0], ydir[0], zdir[0], pt[0], xdir[1], ydir[1], zdir[1], pt[1], xdir[2], ydir[2],
        zdir[2], pt[2], 0, 0, 0, 1;

      DECLARE_INSTANCE();
      auto edge = std::make_shared<EdgeNode>(instance);
      edge->size = (p2 - p1).norm();
      edge->center = true;
      {
        DECLARE_INSTANCE();
        auto mult = std::make_shared<TransformNode>(instance, "multmatrix");
        mult->matrix = mat;
        mult->children.push_back(edge);

        PyObject *pyth_edge = PyOpenSCADObjectFromNode(type, mult);
        PyList_SetItem(pyth_edges, ind++, pyth_edge);
      }
    }
  }
  return pyth_edges;
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
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_edges_core(obj);
}

PyObject *python_faces_core(PyObject *obj, bool tessellate)
{
  PyObject *dummydict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &dummydict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for  Object in faces \n");
    return NULL;
  }

  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);

  if (ps != nullptr) {
    PolygonIndices inds;
    std::vector<int> face_parents;
    if (tessellate == true) {
      ps = PolySetUtils::tessellate_faces(*ps);
      inds = ps->indices;
      for (size_t i = 0; i < inds.size(); i++) face_parents.push_back(-1);
    } else {
      std::vector<Vector4d> normals, new_normals;
      normals = calcTriangleNormals(ps->vertices, ps->indices);
      inds = mergeTriangles(ps->indices, normals, new_normals, face_parents, ps->vertices);
    }
    int resultlen = 0, resultiter = 0;
    for (size_t i = 0; i < face_parents.size(); i++)
      if (face_parents[i] == -1) resultlen++;

    PyObject *pyth_faces = PyList_New(resultlen);

    for (size_t j = 0; j < inds.size(); j++) {
      if (face_parents[j] != -1) continue;
      auto& face = inds[j];
      if (face.size() < 3) continue;
      Vector3d zdir = calcTriangleNormal(ps->vertices, face).head<3>().normalized();
      // calc center of face
      Vector3d ptmin, ptmax;
      for (size_t i = 0; i < face.size(); i++) {
        Vector3d pt = ps->vertices[face[i]];
        for (int k = 0; k < 3; k++) {
          if (i == 0 || pt[k] < ptmin[k]) ptmin[k] = pt[k];
          if (i == 0 || pt[k] > ptmax[k]) ptmax[k] = pt[k];
        }
      }
      Vector3d pt =
        Vector3d((ptmin[0] + ptmax[0]) / 2.0, (ptmin[1] + ptmax[1]) / 2.0, (ptmin[2] + ptmax[2]) / 2.0);
      Vector3d xdir = (ps->vertices[face[1]] - ps->vertices[face[0]]).normalized();
      Vector3d ydir = zdir.cross(xdir);

      Matrix4d mat;
      mat << xdir[0], ydir[0], zdir[0], pt[0], xdir[1], ydir[1], zdir[1], pt[1], xdir[2], ydir[2],
        zdir[2], pt[2], 0, 0, 0, 1;

      Matrix4d invmat = mat.inverse();

      DECLARE_INSTANCE();
      auto poly = std::make_shared<PolygonNode>(instance, CurveDiscretizer(3));
      std::vector<size_t> path;
      for (size_t i = 0; i < face.size(); i++) {
        Vector3d pt = ps->vertices[face[i]];
        Vector4d pt4(pt[0], pt[1], pt[2], 1);
        pt4 = invmat * pt4;
        path.push_back(poly->points.size());
        Vector3d pt3 = pt4.head<3>();
        pt3[2] = 0;  // no radius
        poly->points.push_back(pt3);
      }

      // xdir should be parallel to the longest edge
      int n = poly->points.size();
      Vector3d maxline(0, 0, 0);
      for (int i = 0; i < n; i++) {
        Vector3d line = poly->points[(i + 1) % n] - poly->points[i];
        if (line.norm() > maxline.norm()) maxline = line;
      }
      double arcshift = atan2(maxline[1], maxline[0]);
      Vector3d newpt(0, 0, 0);
      for (auto& pt : poly->points) {
        newpt[0] = pt[0] * cos(-arcshift) - pt[1] * sin(-arcshift);
        newpt[1] = pt[0] * sin(-arcshift) + pt[1] * cos(-arcshift);
        pt = newpt;
      }

      poly->paths.push_back(path);

      // check if there are holes
      for (size_t k = 0; k < inds.size(); k++) {
        if ((size_t)face_parents[k] == j) {
          auto& hole = inds[k];

          std::vector<size_t> path;
          for (size_t i = 0; i < hole.size(); i++) {
            Vector3d pt = ps->vertices[hole[i]];
            Vector4d pt4(pt[0], pt[1], pt[2], 1);
            pt4 = invmat * pt4;
            path.push_back(poly->points.size());
            Vector3d pt3 = pt4.head<3>();
            newpt[0] = pt3[0] * cos(-arcshift) - pt3[1] * sin(-arcshift);
            newpt[1] = pt3[0] * sin(-arcshift) + pt3[1] * cos(-arcshift);
            poly->points.push_back(newpt);
          }
          poly->paths.push_back(path);
        }
      }
      {
        DECLARE_INSTANCE();
        auto mult = std::make_shared<TransformNode>(instance, "multmatrix");
        // mat um arcshift drehen
        Vector3d xvec(mat(0, 0), mat(1, 0), mat(2, 0));
        Vector3d yvec(mat(0, 1), mat(1, 1), mat(2, 1));

        // rotate xvec
        mat(0, 0) = xvec[0] * cos(arcshift) + yvec[0] * sin(arcshift);
        mat(1, 0) = xvec[1] * cos(arcshift) + yvec[1] * sin(arcshift);
        mat(2, 0) = xvec[2] * cos(arcshift) + yvec[2] * sin(arcshift);

        // rotate yvec
        mat(0, 1) = yvec[0] * cos(arcshift) - xvec[0] * sin(arcshift);
        mat(1, 1) = yvec[1] * cos(arcshift) - xvec[1] * sin(arcshift);
        mat(2, 1) = yvec[2] * cos(arcshift) - xvec[2] * sin(arcshift);

        mult->matrix = mat;
        mult->children.push_back(poly);

        PyObject *pyth_face = PyOpenSCADObjectFromNode(type, mult);
        PyList_SetItem(pyth_faces, resultiter++, pyth_face);
      }
    }
    return pyth_faces;
  }
  Py_RETURN_NONE;
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
  char *kwlist[] = {"triangulate", NULL};
  PyObject *tess = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &tess)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_faces_core(obj, tess == Py_True);
}

PyObject *python_children_core(PyObject *obj)
{
  PyObject *dummydict;
  auto solid = PyOpenSCADObjectToNode(obj, &dummydict);
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  if (solid == nullptr) {
    PyErr_SetString(PyExc_TypeError, "not a solid\n");
    return NULL;
  }
  int n = solid->children.size();
  PyObject *result = PyTuple_New(n);
  for (int i = 0; i < n; i++) {
    PyTuple_SetItem(result, i, PyOpenSCADObjectFromNode(type, solid->children[i]));
  }
  return result;
}

PyObject *python_children(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", NULL};
  PyObject *obj = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &obj)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_children_core(obj);
}

PyObject *python_oo_children(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
    PyErr_SetString(PyExc_TypeError, "error during parsing\n");
    return NULL;
  }
  return python_children_core(obj);
}
