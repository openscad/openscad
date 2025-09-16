#pragma once
#include "core/node.h"

extern bool python_active;
extern bool python_trusted;
std::string python_version(void);

void initPython(const std::string& binDir, double time);
std::string evaluatePython(const std::string& code, bool dry_run = 0);
void finishPython();
void python_lock(void);
void python_unlock(void);

extern std::shared_ptr<AbstractNode> python_result_node;

int pythonRunArgs(int argc, char **argv);
int pythonCreateVenv(const std::string& path);
int pythonRunModule(const std::string& appPath, const std::string& module,
                    const std::vector<std::string>& args);
std::string venvBinDirFromSettings();
