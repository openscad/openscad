#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "value.h"
#include "typedefs.h"

class Context
{
public:
	Context(const Context *parent = NULL);
	virtual ~Context();

	virtual Value evaluate_function(const std::string &name, const class EvalContext *evalctx) const;
	virtual class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, const EvalContext *evalctx) const;

	void setVariables(const AssignmentList &args,
										const class EvalContext *evalctx = NULL);

	void set_variable(const std::string &name, const Value &value);
	void set_constant(const std::string &name, const Value &value);

	Value lookup_variable(const std::string &name, bool silent = false) const;

	void setDocumentPath(const std::string &path) { this->document_path = path; }
	const std::string &documentPath() const { return this->document_path; }
	std::string getAbsolutePath(const std::string &filename) const;

public:
	const Context *parent;

	static std::vector<const Context*> ctx_stack;

protected:
	typedef boost::unordered_map<std::string, Value> ValueMap;
	ValueMap constants;
	ValueMap variables;
	ValueMap config_variables;

	std::string document_path; // FIXME: This is a remnant only needed by dxfdim

#ifdef DEBUG
public:
	virtual void dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
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
