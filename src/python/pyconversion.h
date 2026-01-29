
PyObject *python_frommatrix(const Matrix4d& mat);
int python_tomatrix(PyObject *pyt, Matrix4d& mat);
int python_tovector(PyObject *pyt, Vector3d& vec);
std::vector<Vector3d> python_to2dvarpointlist(PyObject *pypoints);
std::vector<std::vector<size_t>> python_to2dintlist(PyObject *pypaths);
PyObject *python_fromvector(const Vector3d vec);
PyObject *python_from2dvarpointlist(const std::vector<Vector3d>& ptlist);
PyObject *python_from3dpointlist(const std::vector<Vector3d>& ptlist);
PyObject *python_from2dint(const std::vector<std::vector<size_t>>& intlist);
PyObject *python_from2dlong(const std::vector<IndexedFace>& intlist);
int python_numberval(PyObject *number, double *result, int *flags, int flagor);
std::vector<int> python_intlistval(PyObject *list);
int python_vectorval(PyObject *vec, int minval, int maxval, double *x, double *y, double *z, double *w,
                     int *flags);
