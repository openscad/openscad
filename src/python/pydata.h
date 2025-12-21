#include <Python.h>
#include <memory>
#ifdef ENABLE_LIBFIVE
#include <libfive.h>
#endif

#include <src/core/module.h>
#include <boost/optional.hpp>

#pragma GCC diagnostic ignored "-Wwrite-strings"

#define DATA_TYPE_UNKNOWN -1
#define DATA_TYPE_LIBFIVE 0
#define DATA_TYPE_SCADMODULE 1
#define DATA_TYPE_MARKEDVALUE 2
#define DATA_TYPE_SCADFUNCTION 3

typedef struct {
  PyObject_HEAD void *data;
  int data_type;
  /* Type-specific fields go here. */
} PyDataObject;

PyMODINIT_FUNC PyInit_PyData(void);

extern PyTypeObject PyDataType;

#ifdef ENABLE_LIBFIVE
PyObject *PyDataObjectFromTree(PyTypeObject *type, const std::vector<libfive::Tree *>& tree);
std::vector<libfive::Tree *> PyDataObjectToTree(PyObject *object);
#endif

PyObject *PyDataObjectFromModule(PyTypeObject *type, std::string modulepath, std::string modulename);
void PyDataObjectToModule(PyObject *obj, std::string& modulepath, std::string& modulename);

PyObject *PyDataObjectFromFunction(PyTypeObject *type, std::string functionpath,
                                   std::string functionname);
void PyDataObjectToFunction(PyObject *obj, std::string& functionpath, std::string& functionname);

PyObject *PyDataObjectFromValue(PyTypeObject *type, double value);
double PyDataObjectToValue(PyObject *obj);
