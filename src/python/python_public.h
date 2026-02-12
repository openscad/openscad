#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/node.h"
#include "core/function.h"
#include "core/node.h"
#include "geometry/Polygon2d.h"
#include <core/Selection.h>

extern bool python_active;
extern bool python_trusted;
extern bool python_runipython;
extern AssignmentList customizer_parameters;
extern AssignmentList customizer_parameters_finished;
void python_export_obj_att(std::ostream& output);
std::string python_version(void);

void initPython(const std::string& binDir, const std::string& scriptpath, double time);
std::string evaluatePython(const std::string& code, bool dry_run = false);
void finishPython();
void python_lock(void);
void python_unlock(void);
void ipython(void);

std::shared_ptr<AbstractNode> python_modulefunc(const ModuleInstantiation *op_module,
                                                const std::shared_ptr<const Context>& cxt,
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
