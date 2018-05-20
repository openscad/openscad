#pragma once

#include <stdexcept>
#include <sstream>
#include "expression.h"

class EvaluationException : public std::runtime_error {
public:
	EvaluationException(const std::string &what_arg) : std::runtime_error(what_arg) {}
	~EvaluationException() throw() {}
};

class AssertionFailedException : public EvaluationException {
public:
	AssertionFailedException(const std::string &what_arg, const Location &loc)  : EvaluationException(what_arg), loc(loc) {}
	~AssertionFailedException() throw() {}
	
public:
	Location loc;
};

class RecursionException: public EvaluationException {
public:
	static RecursionException create(const char *recursiontype, const std::string &name) {
		std::stringstream out;
		out << "ERROR: Recursion detected calling " << recursiontype << " '" << name << "'";
		return RecursionException(out.str());
	}
	~RecursionException() throw() {}

private:
	RecursionException(const std::string &what_arg) : EvaluationException(what_arg) {}
};
