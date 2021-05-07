#pragma once

#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "function.h"
#include "module.h"
#include "value.h"

class ContextFrame;

class EvaluationSession
{
public:
	EvaluationSession(const std::string& documentRoot):
		document_root(documentRoot)
	{}
	
	void push_frame(ContextFrame* frame);
	void pop_frame();
	
	boost::optional<const Value&> try_lookup_special_variable(const std::string &name) const;
	const Value& lookup_special_variable(const std::string &name, const Location &loc) const;
	boost::optional<CallableFunction> lookup_special_function(const std::string &name, const Location &loc) const;
	boost::optional<InstantiableModule> lookup_special_module(const std::string &name, const Location &loc) const;
	
	const std::string& documentRoot() const { return document_root; }

private:
	std::string document_root;
	std::vector<ContextFrame*> stack;
};
