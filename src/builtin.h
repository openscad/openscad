#pragma once

#include <string>
#include <unordered_map>
#include "module.h"
#include "localscope.h"
#include "Assignment.h"
#include "keyword.h"

class Builtins
{
public:
	using FunctionContainer = std::unordered_map<std::string, class AbstractFunction*>;
	using ModuleContainer = std::unordered_map<std::string, class AbstractModule*>;

	static Builtins *instance(bool erase = false);
	static void init(const std::string &name, class AbstractModule *module);
	static void init(const std::string &name, class AbstractFunction *function);
	static void init(const std::string &name, class AbstractModule *module, std::list<std::string> &calltipList);
	static void init(const std::string &name, class AbstractFunction *function, std::list<std::string> &calltipList);
	static void init(const Keyword *keyword, class AbstractModule *module);
	static void init(const Keyword *keyword, class AbstractFunction *function);
	void initialize();
	std::string isDeprecated(const std::string &name) const;

	const AssignmentList &getAssignments() const { return this->assignments; }
	const FunctionContainer &getFunctions() const { return this->functions; }
	const ModuleContainer &getModules() const { return modules; }

	static std::unordered_map<std::string, std::list<std::string>> keywordList;
	
private:
	Builtins();
	~Builtins() = default;

	AssignmentList assignments;
	FunctionContainer functions;
	ModuleContainer modules;

	std::unordered_map<std::string, std::string> deprecations;
};
