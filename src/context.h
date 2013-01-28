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
	Context(const Context *parent = NULL, const class Module *library = NULL);
	~Context();

	void args(const std::vector<std::string> &argnames, 
						const std::vector<class Expression*> &argexpr, 
						const std::vector<std::string> &call_argnames, 
						const std::vector<Value> &call_argvalues);

	void set_variable(const std::string &name, const Value &value);
	void set_constant(const std::string &name, const Value &value);

	Value lookup_variable(const std::string &name, bool silent = false) const;
	Value evaluate_function(const std::string &name, 
													const std::vector<std::string> &argnames, 
													const std::vector<Value> &argvalues) const;
	class AbstractNode *evaluate_module(const class ModuleInstantiation &inst) const;

	void setDocumentPath(const std::string &path) { this->document_path = path; }
	std::string getAbsolutePath(const std::string &filename) const;

public:
	const Context *parent;
	const unordered_map<std::string, class AbstractFunction*> *functions_p;
	const unordered_map<std::string, class AbstractModule*> *modules_p;
	typedef unordered_map<std::string, class Module*> ModuleContainer;
	const ModuleContainer *usedlibs_p;
	const ModuleInstantiation *inst_p;

	static std::vector<const Context*> ctx_stack;

	mutable unordered_map<std::string, int> recursioncount;

private:
	typedef unordered_map<std::string, Value> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;
	std::string document_path;
};

#endif
