#pragma once

#include "evaluationsession.h"
#include "valuemap.h"

class ContextFrame
{
public:
	ContextFrame(EvaluationSession* session);
	virtual ~ContextFrame() {}

	virtual boost::optional<const Value&> lookup_local_variable(const std::string &name) const;
	virtual boost::optional<CallableFunction> lookup_local_function(const std::string &name, const Location &loc) const;
	virtual boost::optional<InstantiableModule> lookup_local_module(const std::string &name, const Location &loc) const;

	void set_variable(const std::string &name, Value&& value);

	void apply_lexical_variables(const ContextFrame &other);
	void apply_config_variables(const ContextFrame &other);
	void apply_variables(const ContextFrame &other) {
		apply_lexical_variables(other);
		apply_config_variables(other);
	}
	void apply_variables(const ValueMap& variables);

	static bool is_config_variable(const std::string &name);

	EvaluationSession* session() const { return evaluation_session; }
	const std::string &documentRoot() const { return evaluation_session->documentRoot(); }

protected:
	ValueMap lexical_variables;
	ValueMap config_variables;
	EvaluationSession* evaluation_session;

public:
#ifdef DEBUG
	virtual std::string dump();
#endif
};
