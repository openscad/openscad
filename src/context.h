#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "value.h"

using boost::unordered_map;

class Context
{
public:
	const Context *parent;
	typedef unordered_map<std::string, Value> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;
	const unordered_map<std::string, class AbstractFunction*> *functions_p;
	const unordered_map<std::string, class AbstractModule*> *modules_p;
	typedef unordered_map<std::string, class Module*> ModuleContainer;
	const ModuleContainer *usedlibs_p;
	const class ModuleInstantiation *inst_p;
	std::string document_path;

	static std::vector<const Context*> ctx_stack;

	Context(const Context *parent = NULL);
	~Context();

	void args(const std::vector<std::string> &argnames, const std::vector<class Expression*> &argexpr, const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues);

	void set_variable(const std::string &name, Value value);
	Value lookup_variable(const std::string &name, bool silent = false) const;

	void set_constant(const std::string &name, Value value);

	std::string get_absolute_path(const std::string &filename) const;

	Value evaluate_function(const std::string &name, const std::vector<std::string> &argnames, const std::vector<Value> &argvalues) const;
	class AbstractNode *evaluate_module(const ModuleInstantiation *inst) const;
};

#endif
