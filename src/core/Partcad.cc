#include <Python.h>
#include <random>
#include <filesystem>
#include <mutex>
#include "core/Partcad.h"
#include "utils/printutils.h"

static std::filesystem::path makeTempFilename() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(0, 999999);
    namespace fs = std::filesystem;

    // Get the temporary directory path
    fs::path tempDir = fs::temp_directory_path();

    // Create a random file name
    std::string randomFileName = "partcad_" + std::to_string(dis(gen)) + ".stl";

    // Create the temporary filename
    return tempDir / randomFileName;
}

std::string Partcad::getPart(const std::string& partSpec) {
    static PyObject *pModule = NULL;
    static std::once_flag callOncePythonInterpreterInit;
    static std::once_flag callOncePartcadModInit;
    
    const char *modname = "partcad";
    std::string filename;
    PyObject *pGetPartFunc = NULL, *pValue = NULL, *pPartObj = NULL;
    PyStatus status = PyStatus_Ok();

    auto pythonInterpreterInit = [&]{
        PyConfig config;

        PyConfig_InitPythonConfig(&config);
        config.install_signal_handlers = 0;

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            LOG(message_group::Error, "error initializing python interpreter");
            PyErr_Print();
            throw std::exception();
        }
        PyConfig_Clear(&config);
    };

    auto partcadModInit = [&]{
        PyObject *pName = NULL;
    
        pName = PyUnicode_DecodeFSDefault(modname);
        if (pName == NULL) {
            LOG(message_group::Error, "error decoding module %1s filesystem name", modname);
            PyErr_Print();
            throw std::exception();
        }

        pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule == NULL) {
            LOG(message_group::Error, "Failed to load partcad module");
            PyErr_Print();
            throw std::exception();
        }
    };

    try {
        std::call_once(callOncePythonInterpreterInit, pythonInterpreterInit);
        std::call_once(callOncePartcadModInit, partcadModInit);
    } catch (std::exception&) {
        return "";
    }
  
    pGetPartFunc = PyObject_GetAttrString(pModule, "get_part");
    if (pGetPartFunc == NULL) {
        LOG(message_group::Error, "get_part function not found in partcad module");
        PyErr_Print();
        return "";

    }
    if (!PyCallable_Check(pGetPartFunc)) {
        LOG(message_group::Error, "get_part is not a function in partcad module");
        Py_DECREF(pGetPartFunc);
        return ""; 
    }
    
    pPartObj = PyObject_CallFunction(pGetPartFunc, "s", partSpec.c_str());
    if (pPartObj == NULL) {
        LOG(message_group::Error, "call error to get_part function in partcad module");
        PyErr_Print();
        Py_DECREF(pGetPartFunc);
        return ""; 
    }

    if (pPartObj == Py_None) {
        LOG(message_group::Error, "call to get_part function in partcad module failed to return a part");
        Py_DECREF(pGetPartFunc);
        Py_DECREF(pPartObj);
        return "";
    }

    std::filesystem::path path = makeTempFilename();
    filename = path.native();
    pValue = PyObject_CallMethod(pPartObj, "render_stl", "NNs", Py_None, Py_None, path.c_str());
    if (pValue == NULL) {
        LOG(message_group::Error, "Cannot save partcad part to STL file %1$s", path.c_str());
        PyErr_Print();
        Py_DECREF(pGetPartFunc);
        Py_DECREF(pPartObj);
        return "";
    }
    
    Py_DECREF(pGetPartFunc);
    Py_DECREF(pPartObj);
    
    return filename;
}
