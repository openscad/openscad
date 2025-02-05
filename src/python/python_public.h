#pragma once
#include "node.h"
#include "src/core/function.h"
#include "src/geometry/Polygon2d.h"
#include <Selection.h>

extern bool python_active;
extern bool python_trusted;
extern bool python_runipython;
extern AssignmentList customizer_parameters;
extern AssignmentList customizer_parameters_finished;
void python_export_obj_att(std::ostream& output);

void initPython(double time);

void finishPython();
void python_lock(void);
void python_unlock(void);
void ipython(void);

std::string evaluatePython(const std::string &code, bool dry_run=0);

std::shared_ptr<AbstractNode>
python_modulefunc(const ModuleInstantiation *module,
                  const std::shared_ptr<const Context> &context,
                  std::string &error);

Value python_functionfunc(const FunctionCall *call,
                          const std::shared_ptr<const Context> &context, int &error);
double python_doublefunc(void *cbfunc, double arg);
Outline2d python_getprofile(void *cbfunc, int fn, double arg);

extern bool pythonMainModuleInitialized;
extern bool pythonRuntimeInitialized;
extern bool pythonDryRun;
extern std::shared_ptr<AbstractNode> python_result_node;
extern std::vector<SelectedObject> python_result_handle;
