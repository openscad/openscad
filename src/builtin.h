#pragma once

#include <string>
#include <boost/unordered_map.hpp>
#include "module.h"
#include "localscope.h"

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

	const LocalScope &getGlobalScope() { return this->globalscope; }

private:
	Builtins();
	~Builtins();

	LocalScope globalscope;

	boost::unordered_map<std::string, std::string> deprecations;
};
