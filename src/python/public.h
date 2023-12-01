#pragma once
#include "node.h"
#include "src/core/function.h"

extern bool python_active;
extern bool python_trusted;

void initPython(double time);

void finishPython();

std::string evaluatePython(const std::string &code,
                           AssignmentList &assignments);

std::shared_ptr<AbstractNode>
python_modulefunc(const ModuleInstantiation *module,
                  const std::shared_ptr<const Context> &context,
                  int *modulefound);

Value python_functionfunc(const FunctionCall *call,
                          const std::shared_ptr<const Context> &context);

extern bool pythonMainModuleInitialized;
extern bool pythonRuntimeInitialized;
extern std::shared_ptr<AbstractNode> python_result_node;
