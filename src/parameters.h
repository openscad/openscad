#pragma once

#include <string>
#include <vector>

#include "Assignment.h"
#include "contextframe.h"

/*
 * The parameters of a builtin function or module do not form a true Context;
 * it is a value map, but it doesn't have a parent context or child contexts,
 * no function literals can capture it, and it has simpler memory management.
 * But special variables passed as parameters ARE accessible on the execution
 * stack. Thus, a Parameters is a ContextFrame, held by a ContextFrameHandle.
 */
class Parameters
{
private:
	Parameters(ContextFrame&& frame);

public:
	/*
	 * Matches arguments with parameters.
	 * Does not support default arguments.
	 * Required parameters are set to undefined if absent;
	 * Optional parameters are not set at all.
	 */
	static Parameters parse(
		const std::shared_ptr<EvalContext>& evalctx,
		const std::vector<std::string>& required_parameters,
		const std::vector<std::string>& optional_parameters = {}
	);
	/*
	 * Matches arguments with parameters.
	 * Supports default arguments, and requires a context in which to interpret them.
	 * Absent parameters without defaults are set to undefined.
	 */
	static Parameters parse(
		const std::shared_ptr<EvalContext>& evalctx,
		const AssignmentList& required_parameters,
		const std::shared_ptr<Context>& defining_context
	);
	
	boost::optional<const Value&> lookup(const std::string& name) const;
	
	const Value& get(const std::string& name) const;
	double get(const std::string& name, double default_value) const;
	const std::string& get(const std::string& name, const std::string& default_value) const;
	
	bool contains(const std::string& name) const { return bool(lookup(name)); }
	const Value& operator[](const std::string& name) const { return get(name); }
	
	ContextFrame to_context_frame() &&;

private:
	ContextFrame frame;
	ContextFrameHandle handle;
};
