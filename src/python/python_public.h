#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/node.h"
#include "core/function.h"
#include "geometry/Polygon2d.h"
#include <core/Selection.h>
#include <core/RenderVariables.h>

extern bool python_active;
extern bool python_trusted;
extern bool python_runipython;
extern bool python_runrepl;
extern std::vector<std::string> python_replargs;
extern AssignmentList customizer_parameters;
extern AssignmentList customizer_parameters_finished;
extern std::shared_ptr<RenderVariables> renderVarsSet;
void python_export_obj_att(std::ostream& output);
std::string python_version(void);

void initPython(const std::string& binDir, const std::string& scriptpath, const RenderVariables *r);
std::string evaluatePython(const std::string& code, bool dry_run = false);
void finishPython();
void python_lock(void);
void python_unlock(void);
// Launch the real IPython interactive shell. `args` is forwarded as the
// IPython argv (so `pythonscad --ipython script.py arg1` runs `script.py`
// inside IPython with `arg1` available). The user namespace starts
// empty; the user is responsible for any imports they need (`from
// pythonscad import *`, `from openscad import *`, or `import
// pythonscad`). If IPython cannot be imported (most commonly because
// it is not installed), prints a diagnostic to stderr and falls back
// to repl(). Returns an exit code suitable for the caller to propagate
// to the OS: 0 on a clean shell exit, non-zero if the embedded
// interpreter could not be initialised.
int ipython(const std::vector<std::string>& args);
// Open the basic embedded CPython REPL on stdin. Used as the explicit
// `--repl` entry point and as the fallback when IPython is unavailable.
// The user namespace starts empty; the user is responsible for any
// imports they need. Returns an exit code suitable for the caller to
// propagate (0 on clean exit, non-zero on init failure).
int repl(void);

std::shared_ptr<AbstractNode> python_modulefunc(
  const std::shared_ptr<const ModuleInstantiation>& op_module, const std::shared_ptr<const Context>& cxt,
  std::string& error);

Value python_functionfunc(const FunctionCall *call, const std::shared_ptr<const Context>& cxt,
                          int& error);
double python_doublefunc(void *v_cbfunc, double arg);
Outline2d python_getprofile(void *v_cbfunc, int fn, double arg);

extern bool pythonMainModuleInitialized;
extern bool pythonRuntimeInitialized;
extern bool pythonDryRun;
extern std::shared_ptr<AbstractNode> genlang_result_node;
extern std::vector<SelectedObject> python_result_handle;
extern std::string commandline_commands;

int pythonRunArgs(int argc, char **argv);
int pythonCreateVenv(const std::string& path);
int pythonRunModule(const std::string& appPath, const std::string& module,
                    const std::vector<std::string>& args);
std::string venvBinDirFromSettings();
