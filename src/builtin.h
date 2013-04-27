#ifndef BUILTIN_H_
#define BUILTIN_H_

#include <string>
#include <boost/unordered_map.hpp>
#include "module.h"

class Builtins
{
public:
	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	typedef boost::unordered_map<std::string, class AbstractModule*> ModuleContainer;

	static Builtins *instance(bool erase = false);
	static void init(const char *name, class AbstractModule *module);
	static void init(const char *name, class AbstractFunction *function);
	void initialize();
	std::string isDeprecated(const std::string &name);

	const FunctionContainer &functions() { return this->builtinfunctions; }
	const ModuleContainer &modules() { return this->builtinmodules; }

	const Module &getRootModule() { return this->rootmodule; }

private:
	Builtins();
	~Builtins();

	Module rootmodule;
	FunctionContainer builtinfunctions;
	ModuleContainer builtinmodules;

	boost::unordered_map<std::string, std::string> deprecations;
};

#endif
