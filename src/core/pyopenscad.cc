// Author: Sohler Guenther
// Date: 2023-01-01
// Purpose: Extend openscad with an python interpreter

#include <Python.h>
#include "pyopenscad.h"
#include "CsgOpNode.h"
#include "Value.h"
#include "Expression.h"
#include "PlatformUtils.h"

extern "C" {
	float __flt_rounds(float f) {
		return f;
	}
}	

// https://docs.python.org/3.10/extending/newtypes.html 

static PyObject *pythonInitDict=NULL;
static PyObject *pythonMainModule = NULL ;
bool python_active;
bool python_trusted;

void PyOpenSCADObject_dealloc(PyOpenSCADObject *self)
{
  Py_XDECREF(self->dict);
//  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *PyOpenSCADObject_alloc(PyTypeObject *cls, Py_ssize_t nitems)
{
  return PyType_GenericAlloc(cls, nitems);
}

static PyObject *PyOpenSCADObject_new(PyTypeObject *type, PyObject *args,  PyObject *kwds)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)  type->tp_alloc(type, 0);
  self->node = NULL;
  self->dict = PyDict_New();
  Py_XINCREF(self->dict);
  Py_XINCREF(self);
  return (PyObject *)self;
}

PyObject *PyOpenSCADObjectFromNode(PyTypeObject *type, std::shared_ptr<AbstractNode> node)
{
  PyOpenSCADObject *self;
  self = (PyOpenSCADObject *)  type->tp_alloc(type, 0);
  if (self != NULL) {
    self->node = node;
    self->dict = PyDict_New();
    Py_XINCREF(self->dict);
    Py_XINCREF(self);
    return (PyObject *)self;
  }
  return NULL;
}

int python_more_obj(std::vector<std::shared_ptr<AbstractNode>>& children, PyObject *more_obj) {
  int i, n;
  PyObject *obj;
  std::shared_ptr<AbstractNode> child;
  if (PyList_Check(more_obj)) {
    n = PyList_Size(more_obj);
    for (i = 0; i < n; i++) {
      obj = PyList_GetItem(more_obj, i);
      child = PyOpenSCADObjectToNode(obj);
      children.push_back(child);
    }
  } else if (Py_TYPE(more_obj) == &PyOpenSCADType) {
    child = PyOpenSCADObjectToNode(more_obj);
    children.push_back(child);
  } else return 1;
  return 0;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNode(PyObject *obj)
{
  std::shared_ptr<AbstractNode> result = ((PyOpenSCADObject *) obj)->node;
  Py_XDECREF(obj); 
  return result;
}

std::shared_ptr<AbstractNode> PyOpenSCADObjectToNodeMulti(PyObject *objs)
{
  std::shared_ptr<AbstractNode> result;
  if (Py_TYPE(objs) == &PyOpenSCADType) {
    result = ((PyOpenSCADObject *) objs)->node;
  } else if (PyList_Check(objs)) {

    DECLARE_INSTANCE
    auto node = std::make_shared<CsgOpNode>(instance, OpenSCADOperator::UNION);

    int n = PyList_Size(objs);
    for (int i = 0; i < n; i++) {
      PyObject *obj = PyList_GetItem(objs, i);
      std::shared_ptr<AbstractNode> child = PyOpenSCADObjectToNode(obj);
      node->children.push_back(child);
      Py_XDECREF(obj);
    }
    result=node;
  } else result=NULL;
  Py_XDECREF(objs);
  return result;
}

int python_numberval(PyObject *number, double *result)
{
  if (PyFloat_Check(number)) {
    *result = PyFloat_AsDouble(number);
    return 0;
  }
  if (PyLong_Check(number)) {
    *result = PyLong_AsLong(number);
    return 0;
  }
  return 1;
}

int python_vectorval(PyObject *vec, double *x, double *y, double *z)
{
  *x = 1;
  *y = 1;
  *z = 1;
  if (PyList_Check(vec)) {
    if (PyList_Size(vec) >= 1) {
      if (python_numberval(PyList_GetItem(vec, 0), x)) return 1;
    }
    if (PyList_Size(vec) >= 2) {
      if (python_numberval(PyList_GetItem(vec, 1), y)) return 1;
    }
    if (PyList_Check(vec) && PyList_Size(vec) >= 3) {
      if (python_numberval(PyList_GetItem(vec, 2), z)) return 1;
    }
    return 0;
  }
  if (!python_numberval(vec, x)) {
    *y = *x;
    *z = *x;
    return 0;
  }
  return 1;
}

static int PyOpenSCADInit(PyOpenSCADObject *self, PyObject *arfs, PyObject *kwds)
{
  return 0;
}

static PyMethodDef PyOpenSCADFunctions[] = {
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

  {"linear_extrude", (PyCFunction) python_linear_extrude, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},

  {"union", (PyCFunction) python_union, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"hull", (PyCFunction) python_hull, METH_VARARGS | METH_KEYWORDS, "Hull Object."},
  {"minkowski", (PyCFunction) python_minkowski, METH_VARARGS | METH_KEYWORDS, "Minkowski Object."},
  {"fill", (PyCFunction) python_fill, METH_VARARGS | METH_KEYWORDS, "Fill Object."},
  {"resize", (PyCFunction) python_resize, METH_VARARGS | METH_KEYWORDS, "Resize Object."},
  {"render", (PyCFunction) python_render, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"group", (PyCFunction) python_group, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},

  {"projection", (PyCFunction) python_projection, METH_VARARGS | METH_KEYWORDS, "Projection Object."},
  {"surface", (PyCFunction) python_surface, METH_VARARGS | METH_KEYWORDS, "Surface Object."},
  {"osimport", (PyCFunction) python_import, METH_VARARGS | METH_KEYWORDS, "Import Object."},
  {"color", (PyCFunction) python_color, METH_VARARGS | METH_KEYWORDS, "Import Object."},

  {"output", (PyCFunction) python_output, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"version", (PyCFunction) python_version, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {"version_num", (PyCFunction) python_version_num, METH_VARARGS | METH_KEYWORDS, "Output openscad Version."},
  {NULL, NULL, 0, NULL}
};
// TODO missing
//  dxf_dim
//  dxf_cross
//  intersection_for
//  dxf_linear_extrude
//  dxf_rotate_extrude
//  fontmetrics

PyObject *python_oo_args(PyObject *self, PyObject *args)
{
  int i;
  PyObject *item;
  int n = PyTuple_Size(args);
  PyObject *new_args = PyTuple_New(n + 1);
//	Py_INCREF(new_args); // dont decref either,  dont know why its not working

  Py_INCREF(self);
  int ret = PyTuple_SetItem(new_args, 0, self);
  item = PyTuple_GetItem(new_args, 0);

  for (i = 0; i < PyTuple_Size(args); i++) {
    item = PyTuple_GetItem(args, i);
    Py_INCREF(item);
    PyTuple_SetItem(new_args, i + 1, item);
  }
  return new_args;
}

void get_fnas(double& fn, double& fa, double& fs) {
  PyObject *mainModule = PyImport_AddModule("__main__");
  if (mainModule == NULL) return;
  PyObject *varFn = PyObject_GetAttrString(mainModule, "fn");
  PyObject *varFa = PyObject_GetAttrString(mainModule, "fa");
  PyObject *varFs = PyObject_GetAttrString(mainModule, "fs");
  if (varFn != NULL) fn = PyFloat_AsDouble(varFn);
  if (varFa != NULL) fa = PyFloat_AsDouble(varFa);
  if (varFs != NULL) fs = PyFloat_AsDouble(varFs);
}

static PyMethodDef PyOpenSCADMethods[] = {
  {"translate", (PyCFunction) python_translate_oo, METH_VARARGS | METH_KEYWORDS, "Move  Object."},
  {"rotate", (PyCFunction) python_rotate_oo, METH_VARARGS | METH_KEYWORDS, "Rotate Object."},
  {"scale", (PyCFunction) python_scale_oo, METH_VARARGS | METH_KEYWORDS, "Scale Object."},
  {"union", (PyCFunction) python_union_oo, METH_VARARGS | METH_KEYWORDS, "Union Object."},
  {"difference", (PyCFunction) python_difference_oo, METH_VARARGS | METH_KEYWORDS, "Difference Object."},
  {"intersection", (PyCFunction) python_intersection_oo, METH_VARARGS | METH_KEYWORDS, "Intersection Object."},
  {"mirror", (PyCFunction) python_mirror_oo, METH_VARARGS | METH_KEYWORDS, "Mirror Object."},
  {"multmatrix", (PyCFunction) python_multmatrix_oo, METH_VARARGS | METH_KEYWORDS, "Multmatrix Object."},
  {"linear_extrude", (PyCFunction) python_linear_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Linear_extrude Object."},
  {"rotate_extrude", (PyCFunction) python_rotate_extrude_oo, METH_VARARGS | METH_KEYWORDS, "Rotate_extrude Object."},
  {"offset", (PyCFunction) python_offset_oo, METH_VARARGS | METH_KEYWORDS, "Offset Object."},
  {"roof", (PyCFunction) python_roof_oo, METH_VARARGS | METH_KEYWORDS, "Roof Object."},
  {"output", (PyCFunction) python_output_oo, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {"color", (PyCFunction) python_color_oo, METH_VARARGS | METH_KEYWORDS, "Output the result."},
  {NULL, NULL, 0, NULL}
};

PyNumberMethods PyOpenSCADNumbers =
{
  &python_nb_add,
  &python_nb_substract,
  &python_nb_multiply
};

PyMappingMethods PyOpenSCADMapping =
{
  0,
  python__getitem__,
  python__setitem__
};

PyTypeObject PyOpenSCADType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "PyOpenSCAD",             			/* tp_name */
    sizeof(PyOpenSCADObject), 			/* tp_basicsize */
    0,                         			/* tp_itemsize */
    (destructor) PyOpenSCADObject_dealloc,	/* tp_dealloc */
    0,                         			/* vectorcall_offset */
    0,                         			/* tp_getattr */
    0,                         			/* tp_setattr */
    0,                         			/* tp_as_async */
    0,                         			/* tp_repr */
    &PyOpenSCADNumbers,        			/* tp_as_number */
    0,                         			/* tp_as_sequence */
    &PyOpenSCADMapping,        			/* tp_as_mapping */
    0,                         			/* tp_hash  */
    0,                         			/* tp_call */
    0,                         			/* tp_str */
    0,                         			/* tp_getattro */
    0,                         			/* tp_setattro */
    0,                         			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
    "PyOpenSCAD Object",          		/* tp_doc */
    0,                         			/* tp_traverse */
    0,                         			/* tp_clear */
    0,                         			/* tp_richcompare */
    0,                         			/* tp_weaklistoffset */
    0,                         			/* tp_iter */
    0,                         			/* tp_iternext */
    PyOpenSCADMethods,             		/* tp_methods */
    0,             				/* tp_members */
    0,                         			/* tp_getset */
    0,                         			/* tp_base */
    0,                         			/* tp_dict */
    0,                         			/* tp_descr_get */
    0,                         			/* tp_descr_set */
    0,                         			/* tp_dictoffset */
    (initproc) PyOpenSCADInit,      		/* tp_init */
    PyOpenSCADObject_alloc,    			/* tp_alloc */
    PyOpenSCADObject_new,                	/* tp_new */
};


static PyModuleDef OpenSCADModule = {
  PyModuleDef_HEAD_INIT,
  "openscad",
  "Example module that creates an extension type.",
  -1,
  PyOpenSCADFunctions,
  NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_PyOpenSCAD(void)
{
  PyObject *m;

  if (PyType_Ready(&PyOpenSCADType) < 0) return NULL;
  m = PyModule_Create(&OpenSCADModule);
  if (m == NULL) return NULL;

  Py_INCREF(&PyOpenSCADType);
  PyModule_AddObject(m, "openscad", (PyObject *)&PyOpenSCADType);
  return m;
}

std::shared_ptr<AbstractNode> python_result_node = NULL;

static PyObject *PyInit_openscad(void)
{
  return PyModule_Create(&OpenSCADModule);
}

PyObject *python_callfunction(const std::string &name, const std::vector<std::shared_ptr<Assignment> > &op_args, const char *&errorstr)
{
	PyObject *pFunc = NULL;
	if(!pythonMainModule){
		return NULL;
	}
	PyObject *maindict = PyModule_GetDict(pythonMainModule);

	// search the function in all modules
	PyObject *key, *value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(maindict, &pos, &key, &value)) {
		PyObject *module = PyObject_GetAttrString(pythonMainModule, PyUnicode_AsUTF8(key));
		if(module == NULL) continue;
		PyObject *moduledict = PyModule_GetDict(module);
		if(moduledict == NULL) continue;
        	pFunc = PyDict_GetItemString(moduledict, name.c_str());
		if(pFunc == NULL) continue;
		break;
	}
	if (!pFunc) {
		return NULL;
	}
	if (!PyCallable_Check(pFunc)) {
		return NULL;
	}
	
	// TODO childs,
	PyObject *args = PyTuple_New(op_args.size());
	for(int i=0;i<op_args.size();i++)
	{
		Assignment *op_arg=op_args[i].get();
		shared_ptr<Expression> expr=op_arg->getExpr();
		Value val = expr.get()->evaluate(NULL);
		switch(val.type())
		{
			case Value::Type::NUMBER:
				PyTuple_SetItem(args, i, PyFloat_FromDouble(val.toDouble()));
				break;
			case Value::Type::STRING:
				PyTuple_SetItem(args, i, PyUnicode_FromString(val.toString().c_str()));
				break;
//TODO  more types RANGE, VECTOR, OBEJCT, FUNCTION
			default:
				printf("other\n");
				PyTuple_SetItem(args, i, PyLong_FromLong(-1));
				break;
		}
	}
	PyObject* funcresult = PyObject_CallObject(pFunc, args);

	if(funcresult == NULL) {
		PyObject *pyExcType;
		PyObject *pyExcValue;
		PyObject *pyExcTraceback;
		PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
		PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);

		PyObject* str_exc_value = PyObject_Repr(pyExcValue);
		PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "~");
		errorstr =  PyBytes_AS_STRING(pyExcValueStr);
		Py_XDECREF(pyExcType);
		Py_XDECREF(pyExcValue);
		Py_XDECREF(pyExcTraceback);
		return NULL;
	}
	return funcresult;
}

std::shared_ptr<AbstractNode> python_modulefunc(const ModuleInstantiation *op_module)
{
	std::shared_ptr<AbstractNode> result=NULL;
	const char *errorstr = NULL;
	do {
		PyObject *funcresult = python_callfunction(op_module->name(),op_module->arguments, errorstr);
		if (errorstr != NULL){
			PyErr_SetString(PyExc_TypeError, errorstr);
			return NULL;
		}
		if(funcresult == NULL) return NULL;

		if(funcresult->ob_type == &PyOpenSCADType) result=PyOpenSCADObjectToNode(funcresult);
		else {
			PyErr_SetString(PyExc_TypeError, "Python function result is  not a solid\n");
			break;
		}
	} while(0);
	return result;
}
Value python_convertresult(PyObject *arg)
{
	if(arg == NULL) return Value::undefined.clone();
	if(PyList_Check(arg)) {
		VectorType vec(NULL);
		for(int i=0;i<PyList_Size(arg);i++) {
			PyObject *item=PyList_GetItem(arg,i);
			vec.emplace_back(python_convertresult(item));
		}
		return std::move(vec);
	} else if(PyFloat_Check(arg)) { return { PyFloat_AsDouble(arg) }; }
	else if(PyUnicode_Check(arg)) {
		PyObject* repr = PyObject_Repr(arg);
		PyObject* strobj = PyUnicode_AsEncodedString(repr, "utf-8", "~");
		const char *chars =  PyBytes_AS_STRING(strobj);
		return { std::string(chars) } ;
	} else {
		PyErr_SetString(PyExc_TypeError, "Unsupported function result\n");
		return Value::undefined.clone();
	}
}
Value python_functionfunc(const FunctionCall *call )
{
	const char *errorstr = NULL;
	PyObject *funcresult = python_callfunction(call->name, call->arguments, errorstr);
	if (errorstr != NULL)
	{
		PyErr_SetString(PyExc_TypeError, errorstr);
		return Value::undefined.clone();
	}
	if(funcresult == NULL) return Value::undefined.clone();

	return  python_convertresult(funcresult);
}

extern PyObject *PyInit_libfive(void);

PyMODINIT_FUNC PyInit_PyLibFive(void);

std::string evaluatePython(const std::string & code, double time)
{
  std::string error;
  python_result_node = NULL;
  PyObject *pyExcType = NULL;
  PyObject *pyExcValue = NULL;
  PyObject *pyExcTraceback = NULL;
  const char *python_init_code="\
import sys\n\
class OutputCatcher:\n\
   def __init__(self):\n\
      self.data = ''\n\
   def write(self, stuff):\n\
      self.data = self.data + stuff\n\
   def flush(self):\n\
      pass\n\
catcher_out = OutputCatcher()\n\
catcher_err = OutputCatcher()\n\
stdout_bak=sys.stdout\n\
stderr_bak=sys.stderr\n\
sys.stdout = catcher_out\n\
sys.stderr = catcher_err\n\
";
  const char *python_exit_code="\
sys.stdout = stdout_bak\n\
sys.stderr = stderr_bak\n\
";

    if(pythonInitDict) {
      if (Py_FinalizeEx() < 0) {
        exit(120);
      }
      pythonInitDict=NULL;
    }
    if(!pythonInitDict) {
	    char run_str[80];
	    PyImport_AppendInittab("openscad", &PyInit_openscad);
	    PyConfig config;
            PyConfig_InitPythonConfig(&config);
	    wchar_t libdir[256];
	    swprintf(libdir, 256, L"%s/../lib/pylib/",PlatformUtils::applicationPath().c_str());
	    PyConfig_SetString(&config, &config.pythonpath_env, libdir);
	    Py_Initialize();
            //Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);

            Py_InitializeFromConfig(&config);
	    pythonMainModule =  PyImport_AddModule("__main__");
	    pythonInitDict = PyModule_GetDict(pythonMainModule);
	    PyInit_PyOpenSCAD();
	    sprintf(run_str,"from openscad import *\nfa=12.0\nfn=0.0\nfs=2.0\nt=%g",time);
	    PyRun_String(run_str, Py_file_input, pythonInitDict, pythonInitDict);
    }
    PyRun_SimpleString(python_init_code);
    PyObject *result = PyRun_String(code.c_str(), Py_file_input, pythonInitDict, pythonInitDict);
    if(result  == NULL) PyErr_Print();
    PyRun_SimpleString(python_exit_code);
    for(int i=0;i<2;i++)
    {
      PyObject* catcher = PyObject_GetAttrString(pythonMainModule, i==1?"catcher_err":"catcher_out");
      PyObject* command_output = PyObject_GetAttrString(catcher, "data");
      PyObject* command_output_value = PyUnicode_AsEncodedString(command_output, "utf-8", "~");
      const char *command_output_bytes =  PyBytes_AS_STRING(command_output_value);
      if(command_output_bytes == NULL || *command_output_bytes == '\0') continue;
      if(i ==1) error += command_output_bytes;
      else LOG(command_output_bytes);
    }

    PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback);
//    PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);
    PyObject* str_exc_value = PyObject_Repr(pyExcValue);
    PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "~");
    if(str_exc_value != NULL) Py_XDECREF(str_exc_value);
    const char *strExcValue =  PyBytes_AS_STRING(pyExcValueStr);
    if(pyExcValueStr != NULL) Py_XDECREF(pyExcValueStr);
    if(strExcValue != NULL && strcmp(strExcValue,"<NULL>")) error += strExcValue;
    if(pyExcTraceback != NULL) {
      PyTracebackObject *tb_o = (PyTracebackObject *)pyExcTraceback;
      int line_num = tb_o->tb_lineno;
      error += " in line ";
      error += std::to_string(line_num);
      Py_XDECREF(pyExcTraceback);
    }

    if(pyExcType != NULL) Py_XDECREF(pyExcType);
    if(pyExcValue != NULL) Py_XDECREF(pyExcValue);
    return error;
}

