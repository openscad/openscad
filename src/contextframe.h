#pragma once

#include "evaluationsession.h"
#include "valuemap.h"

class ContextFrame
{
public:
	ContextFrame(EvaluationSession* session);
	virtual ~ContextFrame() {}

	ContextFrame(ContextFrame&& other) = default;

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
	
	void apply_lexical_variables(ContextFrame&& other);
	void apply_config_variables(ContextFrame&& other);
	void apply_variables(ContextFrame&& other);
	

	static bool is_config_variable(const std::string &name);

	EvaluationSession* session() const { return evaluation_session; }
	const std::string &documentRoot() const { return evaluation_session->documentRoot(); }

protected:
	ValueMap lexical_variables;
	ValueMap config_variables;
	EvaluationSession* evaluation_session;

public:
#ifdef DEBUG
	virtual std::string dumpFrame();
#endif
};

/*
 * A ContextFrameHandle stores a reference to a ContextFrame, and keeps it on
 * the special variable stack for the lifetime of the handle.
 */
class ContextFrameHandle
{
public:
	ContextFrameHandle(ContextFrame* frame):
		session(frame->session())
	{
		session->push_frame(frame);
	}
	~ContextFrameHandle()
	{
		release();
	}
	
	ContextFrameHandle(const ContextFrameHandle&) = delete;
	ContextFrameHandle& operator=(const ContextFrameHandle&) = delete;
	ContextFrameHandle& operator=(ContextFrameHandle&&) = delete;
	
	ContextFrameHandle(ContextFrameHandle&& other):
		session(other.session)
	{
		other.session = nullptr;
	}
	
	// Valid only if handle is on the top of the stack.
	ContextFrameHandle& operator=(ContextFrame* frame)
	{
		release();
		session = frame->session();
		session->push_frame(frame);
		return *this;
	}
	
	// Valid only if handle is on the top of the stack.
	void release()
	{
		if (session) {
			session->pop_frame();
			session = nullptr;
		}
	}
	

protected:
	EvaluationSession* session;
};
