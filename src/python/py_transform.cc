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

#include "linalg.h"
#include "GeometryUtils.h"
#include "genlang/genlang.h"
#include <Python.h>
#include "pyopenscad.h"
#include "pyfunctions.h"
#include "pyconversion.h"
#include "GeometryEvaluator.h"
#include "TransformNode.h"
#include "utils/degree_trig.h"

PyObject *python_number_scale(PyObject *pynum, Vector3d scalevec, int vecs)
{
  Matrix4d mat;
  if (!python_tomatrix(pynum, mat)) {
    Transform3d matrix = Transform3d::Identity();
    matrix.scale(scalevec);
    Vector3d n;
    for (int i = 0; i < 3; i++) {    // row
      for (int j = 0; j < 4; j++) {  // col
        mat(j, i) = mat(j, i) * scalevec[i];
      }
    }
    return python_frommatrix(mat);
  }
  Vector3d vec;
  if (!python_tovector(pynum, vec)) {
    for (int i = 0; i < 3; i++) vec[i] *= scalevec[i];
    return python_fromvector(vec);
  }
  return nullptr;
}

PyObject *python_scale_sub(PyObject *obj, Vector3d scalevec)
{
  PyObject *mat = python_number_scale(obj, scalevec, 4);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  std::shared_ptr<AbstractNode> child;
  auto node = std::make_shared<TransformNode>(instance, "scale");
  PyObject *child_dict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in scale");
    return NULL;
  }
  node->matrix.scale(scalevec);
  node->setPyName(child->getPyName());
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_number_scale(value, scalevec, 4);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_scale_core(PyObject *obj, PyObject *val_v)
{
  double x = 1, y = 1, z = 1;
  if (python_vectorval(val_v, 2, 3, &x, &y, &z)) {
    PyErr_SetString(PyExc_TypeError, "Invalid vector specifiaction in scale, use 1 to 3 ordinates.");
    return NULL;
  }
  Vector3d scalevec(x, y, z);

  if (OpenSCAD::rangeCheck) {
    if (scalevec[0] == 0 || scalevec[1] == 0 || scalevec[2] == 0 || !std::isfinite(scalevec[0]) ||
        !std::isfinite(scalevec[1]) || !std::isfinite(scalevec[2])) {
      //      LOG(message_group::Warning, instance->location(), parameters.documentRoot(), "scale(%1$s)",
      //      parameters["v"].toEchoStringNoThrow());
    }
  }

  return python_scale_sub(obj, scalevec);
}

PyObject *python_scale(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj, val_v);
}

PyObject *python_oo_scale(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing scale(object, scale)");
    return NULL;
  }
  return python_scale_core(obj, val_v);
}

PyObject *python_explode(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing explode(object, list)");
    return NULL;
  }
  return python_nb_sub_vec3(obj, val_v, 3);
}

PyObject *python_oo_explode(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing explode(object, list)");
    return NULL;
  }
  return python_nb_sub_vec3(obj, val_v, 3);
}

PyObject *python_number_rot(PyObject *mat, Matrix3d rotvec, int vecs)
{
  Transform3d matrix = Transform3d::Identity();
  matrix.rotate(rotvec);
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  Vector3d n;
  for (int i = 0; i < vecs; i++) {
    n = Vector3d(raw(0, i), raw(1, i), raw(2, i));
    n = matrix * n;
    for (int j = 0; j < 3; j++) raw(j, i) = n[j];
  }
  return python_frommatrix(raw);
}

PyObject *python_rotate_sub(PyObject *obj, Vector3d vec3, double angle, PyObject *ref, int dragflags)
{
  Matrix3d M;
  if (isnan(angle)) {
    double sx = 0, sy = 0, sz = 0;
    double cx = 1, cy = 1, cz = 1;
    if (vec3[2] != 0) {
      sz = sin_degrees(vec3[2]);
      cz = cos_degrees(vec3[2]);
    }
    if (vec3[1] != 0) {
      sy = sin_degrees(vec3[1]);
      cy = cos_degrees(vec3[1]);
    }
    if (vec3[0] != 0) {
      sx = sin_degrees(vec3[0]);
      cx = cos_degrees(vec3[0]);
    }

    M << cy * cz, cz * sx * sy - cx * sz, cx * cz * sy + sx * sz, cy * sz, cx * cz + sx * sy * sz,
      -cz * sx + cx * sy * sz, -sy, cy * sx, cx * cy;
  } else {
    M = angle_axis_degrees(angle, vec3);
  }
  PyObject *mat = python_number_rot(obj, M, 4);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "rotate");
  node->dragflags = dragflags;

  PyObject *child_dict;
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in rotate");
    return NULL;
  }
  node->matrix.rotate(M);
  node->setPyName(child->getPyName());

  PyObject *pyresult;
  if (ref == nullptr) {
    node->children.push_back(child);
    pyresult = PyOpenSCADObjectFromNode(type, node);
  } else {
    Vector3d ref_point;
    python_vectorval(ref, 1, 3, &(ref_point[0]), &(ref_point[1]), &(ref_point[2]), nullptr, &dragflags);

    std::shared_ptr<TransformNode> prenode, postnode;
    {
      DECLARE_INSTANCE();
      prenode = std::make_shared<TransformNode>(instance, "translate");
      prenode->matrix.translate(-ref_point);
    }
    {
      DECLARE_INSTANCE();
      postnode = std::make_shared<TransformNode>(instance, "translate");
      postnode->matrix.translate(ref_point);
    }
    prenode->children.push_back(child);
    node->children.push_back(prenode);
    postnode->children.push_back(node);
    pyresult = PyOpenSCADObjectFromNode(type, postnode);
  }
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_number_rot(value, M, 4);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, PyObject *val_v, PyObject *ref)
{
  Vector3d vec3(0, 0, 0);
  double angle;
  int dragflags = 0;
  if (val_a != nullptr && PyList_Check(val_a) && val_v == nullptr) {
    python_vectorval(val_a, 1, 3, &(vec3[0]), &(vec3[1]), &(vec3[2]), nullptr, &dragflags);
    return python_rotate_sub(obj, vec3, NAN, ref, dragflags);
  } else if (val_a != nullptr && val_v != nullptr && !python_numberval(val_a, &angle, nullptr, 0) &&
             PyList_Check(val_v) && PyList_Size(val_v) == 3) {
    vec3[0] = PyFloat_AsDouble(PyList_GetItem(val_v, 0));
    vec3[1] = PyFloat_AsDouble(PyList_GetItem(val_v, 1));
    vec3[2] = PyFloat_AsDouble(PyList_GetItem(val_v, 2));
    return python_rotate_sub(obj, vec3, angle, ref, dragflags);
  }
  PyErr_SetString(PyExc_TypeError, "Invalid arguments to rotate()");
  return nullptr;
}

PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "a", "v", "ref", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  PyObject *obj = nullptr;
  PyObject *ref = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|OO", kwlist, &obj, &val_a, &val_v, &ref)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v, ref);
}

PyObject *python_oo_rotate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"a", "v", "ref", nullptr};
  PyObject *val_a = nullptr;
  PyObject *val_v = nullptr;
  PyObject *ref = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO", kwlist, &val_a, &val_v, &ref)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing rotate(object, vec3)");
    return NULL;
  }
  return python_rotate_core(obj, val_a, val_v, ref);
}

PyObject *python_number_mirror(PyObject *mat, Matrix4d m, int vecs)
{
  Matrix4d raw;
  if (python_tomatrix(mat, raw)) return nullptr;
  Vector4d n;
  for (int i = 0; i < vecs; i++) {
    n = Vector4d(raw(0, i), raw(1, i), raw(2, i), 0);
    n = m * n;
    for (int j = 0; j < 3; j++) raw(j, i) = n[j];
  }
  return python_frommatrix(raw);
}

PyObject *python_mirror_sub(PyObject *obj, Matrix4d& m)
{
  PyObject *mat = python_number_mirror(obj, m, 4);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "mirror");
  node->matrix = m;
  PyObject *child_dict;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in mirror");
    return NULL;
  }
  node->children.push_back(child);
  node->setPyName(child->getPyName());
  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_number_mirror(value, m, 4);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
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
  Matrix4d m = Matrix4d::Identity();
  if (x != 0.0 || y != 0.0 || z != 0.0) {
    // skip using sqrt to normalize the vector since each element of matrix contributes it with two
    // multiplied terms instead just divide directly within each matrix element simplified calculation
    // leads to less float errors
    double a = x * x + y * y + z * z;

    m << 1 - 2 * x * x / a, -2 * y * x / a, -2 * z * x / a, 0, -2 * x * y / a, 1 - 2 * y * y / a,
      -2 * z * y / a, 0, -2 * x * z / a, -2 * y * z / a, 1 - 2 * z * z / a, 0, 0, 0, 0, 1;
  }
  return python_mirror_sub(obj, m);
}

PyObject *python_mirror(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "v", NULL};

  PyObject *obj = NULL;
  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &obj, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_oo_mirror(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};

  PyObject *val_v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &val_v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing mirror(object, vec3)");
    return NULL;
  }
  return python_mirror_core(obj, val_v);
}

PyObject *python_number_trans(PyObject *pynum, Vector3d transvec, int vecs)
{
  Matrix4d mat;
  if (!python_tomatrix(pynum, mat)) {
    for (int i = 0; i < 3; i++) mat(i, 3) += transvec[i];
    return python_frommatrix(mat);
  }

  Vector3d vec;
  if (!python_tovector(pynum, vec)) {
    return python_fromvector(vec + transvec);
  }

  return nullptr;
}

PyObject *python_translate_sub(PyObject *obj, Vector3d translatevec, int dragflags)
{
  PyObject *child_dict;
  PyObject *mat = python_number_trans(obj, translatevec, 4);
  if (mat != nullptr) return mat;

  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "translate");
  std::shared_ptr<AbstractNode> child;
  PyTypeObject *type = PyOpenSCADObjectType(obj);
  child = PyOpenSCADObjectToNodeMulti(obj, &child_dict);
  if (child == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in translate");
    return NULL;
  }
  node->setPyName(child->getPyName());
  node->dragflags = dragflags;
  node->matrix.translate(translatevec);

  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {  // TODO dies ueberall
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      PyObject *value1 = python_number_trans(value, translatevec, 4);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_translate_core(PyObject *obj, PyObject *v)
{
  return python_nb_sub_vec3(obj, v, 0);
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
  return python_translate_core(obj, v);
}

PyObject *python_oo_translate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"v", NULL};
  PyObject *v = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &v)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_translate_core(obj, v);
}

PyObject *python_multmatrix_sub(PyObject *pyobj, PyObject *pymat, int div)
{
  Matrix4d mat;
  if (!python_tomatrix(pymat, mat)) {
    double w = mat(3, 3);
    if (w != 1.0) mat = mat / w;
  } else {
    PyErr_SetString(PyExc_TypeError, "Matrix vector should be 4x4 array");
    return NULL;
  }
  if (div) {
    auto tmp = mat.inverse().eval();
    mat = tmp;
  }

  Matrix4d objmat;
  if (!python_tomatrix(pyobj, objmat)) {
    objmat = objmat * mat;
    return python_frommatrix(objmat);
  }

  Vector3d objvec;
  if (!python_tovector(pyobj, objvec)) {
    Vector4d objvec4(objvec[0], objvec[1], objvec[2], 1);
    objvec4 = mat * objvec4;
    return python_fromvector(objvec4.head<3>());
  }
  DECLARE_INSTANCE();
  auto node = std::make_shared<TransformNode>(instance, "multmatrix");
  std::shared_ptr<AbstractNode> child;
  PyObject *child_dict;
  PyTypeObject *type = PyOpenSCADObjectType(pyobj);
  child = PyOpenSCADObjectToNodeMulti(pyobj, &child_dict);
  if (!child) {
    PyErr_SetString(PyExc_TypeError, "Invalid type for Object in multmatrix");
    return NULL;
  }
  node->setPyName(child->getPyName());

  node->matrix = mat;
  node->children.push_back(child);
  PyObject *pyresult = PyOpenSCADObjectFromNode(type, node);
  if (child_dict != nullptr) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(child_dict, &pos, &key, &value)) {
      Matrix4d raw;
      if (python_tomatrix(value, raw)) return nullptr;
      PyObject *value1 = python_frommatrix(node->matrix * raw);
      if (value1 != nullptr) PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value1);
      else PyDict_SetItem(((PyOpenSCADObject *)pyresult)->dict, key, value);
    }
  }
  return pyresult;
}

PyObject *python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist, &obj, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 0);
}

PyObject *python_oo_multmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing multmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 0);
}

PyObject *python_divmatrix(PyObject *self, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"obj", "m", NULL};
  PyObject *obj = NULL;
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO!", kwlist, &obj, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 1);
}

PyObject *python_oo_divmatrix(PyObject *obj, PyObject *args, PyObject *kwargs)
{
  char *kwlist[] = {"m", NULL};
  PyObject *mat = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyList_Type, &mat)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing divmatrix(object, vec16)");
    return NULL;
  }
  return python_multmatrix_sub(obj, mat, 1);
}

PyObject *python_dir_sub_core(PyObject *obj, double arg, int mode)
{
  if (mode < 6) {
    Vector3d trans;
    switch (mode) {
    case 0: trans = Vector3d(arg, 0, 0); break;
    case 1: trans = Vector3d(-arg, 0, 0); break;
    case 2: trans = Vector3d(0, -arg, 0); break;
    case 3: trans = Vector3d(0, arg, 0); break;
    case 4: trans = Vector3d(0, 0, -arg); break;
    case 5: trans = Vector3d(0, 0, arg); break;
    }
    return python_translate_sub(obj, trans, 0);
  } else {
    Vector3d rot;
    switch (mode) {
    case 6: rot = Vector3d(arg, 0, 0); break;
    case 7: rot = Vector3d(0, arg, 0); break;
    case 8: rot = Vector3d(0, 0, arg); break;
    }
    return python_rotate_sub(obj, rot, NAN, nullptr, 0);
  }
}

PyObject *python_dir_sub(PyObject *self, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"obj", "v", NULL};
  PyObject *obj = NULL;
  double arg;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Od", kwlist, &obj, &arg)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_dir_sub_core(obj, arg, mode);
}

PyObject *python_oo_dir_sub(PyObject *obj, PyObject *args, PyObject *kwargs, int mode)
{
  char *kwlist[] = {"v", NULL};
  double arg;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist, &arg)) {
    PyErr_SetString(PyExc_TypeError, "Error during parsing translate(object,vec3)");
    return NULL;
  }
  return python_dir_sub_core(obj, arg, mode);
}

PyObject *python_right(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 0);
}

PyObject *python_oo_right(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 0);
}

PyObject *python_left(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 1);
}

PyObject *python_oo_left(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 1);
}

PyObject *python_front(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 2);
}

PyObject *python_oo_front(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 2);
}

PyObject *python_back(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 3);
}

PyObject *python_oo_back(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 3);
}

PyObject *python_down(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 4);
}

PyObject *python_oo_down(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 4);
}

PyObject *python_up(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 5);
}

PyObject *python_oo_up(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 5);
}

PyObject *python_rotx(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 6);
}

PyObject *python_oo_rotx(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 6);
}

PyObject *python_roty(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 7);
}

PyObject *python_oo_roty(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 7);
}

PyObject *python_rotz(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_dir_sub(self, args, kwargs, 8);
}

PyObject *python_oo_rotz(PyObject *self, PyObject *args, PyObject *kwargs)
{
  return python_oo_dir_sub(self, args, kwargs, 8);
}
