
PyObject *python_frommatrix(const Matrix4d& mat);
PyObject *python_from2dvarpointlist(const std::vector<Vector3d>& ptlist);
PyObject *python_from3dpointlist(const std::vector<Vector3d>& ptlist);
PyObject *python_from2dint(const std::vector<std::vector<size_t>>& intlist);
PyObject *python_from2dlong(const std::vector<IndexedFace>& intlist);
