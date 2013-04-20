#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "value.h"

class Context
{
public:
	Context(const Context *parent = NULL, const class Module *library = NULL);
	~Context();

	void args(const std::vector<std::string> &argnames, 
						const std::vector<class Expression*> &argexpr, 
						const std::vector<std::string> &call_argnames, 
						const std::vector<Value> &call_argvalues);

	bool check_for_unknown_args(
        const std::vector<std::string> &argnames,
        const std::vector<std::string> &opt_argnames,
  		const std::vector<std::string> &call_argnames);

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
	const boost::unordered_map<std::string, class AbstractFunction*> *functions_p;
	const boost::unordered_map<std::string, class AbstractModule*> *modules_p;
	typedef boost::unordered_map<std::string, class Module*> ModuleContainer;
	const ModuleContainer *usedlibs_p;
	const ModuleInstantiation *inst_p;

	static std::vector<const Context*> ctx_stack;

	mutable boost::unordered_map<std::string, int> recursioncount;

private:
	typedef boost::unordered_map<std::string, Value> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;
	std::string document_path;
};

// Constants

// $rm= radius mode constants
// It's kinda weird that these are double, but that fits best with the rest of the Value class.

// Default. Radius is to verticies of polygon which approximates a circle.
static const double OUTER_RADIUS = 1;
// Specified radius is tangent to the polygon segments approximating a circle.
static const double INNER_RADIUS = 2;
// Specified radius crosses segments at 1/4 and 3/4 of the arc approximated by each segment of the polygon.
static const double MIDPOINT_RADIUS = 3;


#endif
