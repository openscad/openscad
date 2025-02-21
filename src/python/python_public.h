#pragma once
#include "src/core/node.h"

extern bool python_active;
extern bool python_trusted;
std::string python_version(void);

void initPython(double time);
std::string evaluatePython(const std::string &code, bool dry_run=0);
void finishPython();

extern std::shared_ptr<AbstractNode> python_result_node;
