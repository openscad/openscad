#pragma once

#include <string>
#include <unordered_map>
#include "module.h"
#include "localscope.h"
#include "Assignment.h"

class Builtins
{
public:
	typedef std::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	typedef std::unordered_map<std::string, class AbstractModule*> ModuleContainer;

	static Builtins *instance(bool erase = false);
	static void init(const char *name, class AbstractModule *module);
	static void init(const char *name, class AbstractFunction *function);
	void initialize();
	std::string isDeprecated(const std::string &name);

	const AssignmentList &getAssignments() { return this->assignments; }
	const FunctionContainer &getFunctions() { return this->functions; }
	const ModuleContainer &getModules() { return modules; }
	
private:
	Builtins();
	~Builtins();

	AssignmentList assignments;
	FunctionContainer functions;
	ModuleContainer modules;

	std::unordered_map<std::string, std::string> deprecations;
};
