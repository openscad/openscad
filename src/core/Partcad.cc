#include <Python.h>
#include <random>
#include <array>
#include <algorithm>
#include <memory>
#include "core/PartCAD.h"
#include "utils/printutils.h"

static std::filesystem::path makeTempFilename()
{
  namespace fs = std::filesystem;

  static const std::array charMap = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, charMap.size() - 1);
  std::string name(12, '_');

  // Get the temporary directory path
  fs::path tempDir = fs::temp_directory_path();

  std::generate_n(std::begin(name), name.size(), [&] { return charMap[dis(gen)]; });

  // Create a random file name
  std::string randomFileName = "PartCAD_" + name + ".stl";

  // Create the temporary filename
  return tempDir / randomFileName;
}

static auto PyObjectDeleter = [](PyObject *pObject) { Py_XDECREF(pObject); };
using PyObjectUniquePtr = std::unique_ptr<PyObject, const decltype(PyObjectDeleter)&>;

static void logPartCADError()
{
  PyObjectUniquePtr pExcValue{nullptr, PyObjectDeleter};
  PyObjectUniquePtr pExcTraceback{nullptr, PyObjectDeleter};

  if (!PyErr_Occurred()) {
    return;
  }

  pExcValue.reset(PyErr_GetRaisedException());
  if (!pExcValue) {
    return;
  }

  {
    PyObjectUniquePtr pRepr{PyObject_Repr(pExcValue.get()), PyObjectDeleter};
    if (!pRepr) {
      return;
    }
    LOG(message_group::Error, "Exception: %1s", PyUnicode_AsUTF8(pRepr.get()));
  }

  pExcTraceback.reset(PyException_GetTraceback(pExcValue.get()));
  if (!pExcTraceback) {
    return;
  }

  {
    PyObjectUniquePtr pRepr{PyObject_Repr(pExcTraceback.get()), PyObjectDeleter};
    if (!pRepr) {
      return;
    }
    LOG(message_group::Error, "Traceback: %1s", PyUnicode_AsUTF8(pRepr.get()));
  }
}

class PythonInterpreter
{
public:
  static PythonInterpreter *instance()
  {
    static PythonInterpreter interpreter;
    return &interpreter;
  }

private:
  PythonInterpreter(const PythonInterpreter&) = delete;
  PythonInterpreter(PythonInterpreter&&) = delete;
  PythonInterpreter& operator=(const PythonInterpreter&) = delete;
  PythonInterpreter& operator=(PythonInterpreter&&) = delete;

  PythonInterpreter()
  {
    PyConfig config;
    PyStatus status = PyStatus_Ok();

    PyConfig_InitPythonConfig(&config);
    config.install_signal_handlers = 0;

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
      LOG(message_group::Error, "Could not initialize the Python interpreter");
      logPartCADError();
      throw std::exception();
    }
    PyConfig_Clear(&config);
  }

  ~PythonInterpreter()
  {
    // Let it be. It should be finalized in the
    // same thread it was initialized.
    // Py_Finalize();
  }
};

class PartCADModule
{
public:
  static PartCADModule *instance()
  {
    static PartCADModule partCADModule;
    return &partCADModule;
  }

  PyObject *get() { return pModule.get(); }

private:
  PartCADModule(const PartCADModule&) = delete;
  PartCADModule(PartCADModule&&) = delete;
  PartCADModule& operator=(const PartCADModule&) = delete;
  PartCADModule& operator=(PartCADModule&&) = delete;

  PartCADModule()
  {
    const char *modname = "partcad";
    PyObjectUniquePtr pName{nullptr, PyObjectDeleter};

    pName.reset(PyUnicode_FromString(modname));
    if (!pName) {
      LOG(message_group::Error, "Could not decode the Python module \"%1s\" into Python Unicode string",
          modname);
      logPartCADError();
      throw std::exception();
    }

    pModule.reset(PyImport_Import(pName.get()));
    pName.release();

    if (!pModule) {
      LOG(message_group::Error,
          "Could not load the Python module \"partcad\". Please, try installing it with \"pip install "
          "partcad-cli\"");
      logPartCADError();
      throw std::exception();
    }
  }

  ~PartCADModule() {}

  PyObjectUniquePtr pModule{nullptr, PyObjectDeleter};
};

std::string PartCAD::getPart(const std::string& partSpec, const std::filesystem::path& configPath)
{
  namespace fs = std::filesystem;
  PartCADModule *pModule = nullptr;

  std::string filename;
  PyObjectUniquePtr pInitFunc{nullptr, PyObjectDeleter};
  PyObjectUniquePtr pContextObj{nullptr, PyObjectDeleter};
  PyObjectUniquePtr pPartObj{nullptr, PyObjectDeleter};
  PyObjectUniquePtr pValue{nullptr, PyObjectDeleter};
  PyStatus status = PyStatus_Ok();

  try {
    PythonInterpreter::instance();
    pModule = PartCADModule::instance();
  } catch (std::exception&) {
    return "";
  }

  pInitFunc.reset(PyObject_GetAttrString(pModule->get(), "init"));
  if (!pInitFunc) {
    LOG(message_group::Error, "Could not find \"init\" function in Python module \"partcad\"");
    logPartCADError();
    return "";
  }

  if (!PyCallable_Check(pInitFunc.get())) {
    LOG(message_group::Error, "\"init\" is not a function in the Python module \"partcad\"");
    logPartCADError();
    return "";
  }

  pContextObj.reset(PyObject_CallFunction(pInitFunc.get(), "sN", configPath.c_str(), Py_False));
  pInitFunc.release();
  if (pContextObj == NULL) {
    LOG(message_group::Error, "Error calling \"init\" function in the Python module \"partcad\"");
    logPartCADError();
    return "";
  }

  if (pContextObj.get() == Py_None) {
    LOG(message_group::Error,
        "Error executing \"init\" function in the Python module \"partcad\" (failed to return a context "
        "object)");
    logPartCADError();
    return "";
  }

  pPartObj.reset(PyObject_CallMethod(pContextObj.get(), "get_part", "s", partSpec.c_str()));
  pContextObj.release();
  if (!pPartObj) {
    LOG(message_group::Error,
        "Error calling \"Context::get_part\" method in the Python module \"partcad\"");
    logPartCADError();
    return "";
  }

  if (pPartObj.get() == Py_None) {
    LOG(message_group::Error,
        "Error executing \"Context::get_part\" method in the Python module \"partcad\" (failed to "
        "return a part)");
    return "";
  }

  fs::path path = makeTempFilename();
  filename = path.native();
  pValue.reset(PyObject_CallMethod(pPartObj.get(), "render_stl", "NNs", Py_None, Py_None, path.c_str()));
  pPartObj.release();
  if (!pValue) {
    LOG(message_group::Error, "Could not save the PartCAD part to the STL file %1$s", path.c_str());
    logPartCADError();
    return "";
  }
  pValue.release();

  return filename;
}
