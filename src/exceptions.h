#pragma once

#include <stdexcept>
#include <sstream>
#include "expression.h"
#include "printutils.h"

class EvaluationException : public std::runtime_error {
public:
	EvaluationException(const std::string &what_arg) : std::runtime_error(what_arg) {}
	~EvaluationException() throw() {}
public:
	int traceDepth=12;
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
	static RecursionException create(const std::string &recursiontype, const std::string &name, const Location &loc) {
		return RecursionException{STR("ERROR: Recursion detected calling " << recursiontype << " '" << name << "'"), loc};
	}
	~RecursionException() throw() {}

public:
	Location loc;

private:
	RecursionException(const std::string &what_arg, const Location &loc) : EvaluationException(what_arg), loc(loc) {}
};


class LoopCntException: public EvaluationException {
public:
	static LoopCntException create(const std::string &type, const Location &loc) {
		return LoopCntException{STR("ERROR: " << type << " loop counter exceeded limit"), loc};
	}
	~LoopCntException() throw() {}

public:
	Location loc;

private:
	LoopCntException(const std::string &what_arg, const Location &loc) : EvaluationException(what_arg), loc(loc) {}
};

class HardWarningException : public EvaluationException {
public:
	HardWarningException(const std::string &what_arg) : EvaluationException(what_arg) {}
	~HardWarningException() throw() {}
};
