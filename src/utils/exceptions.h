#pragma once

#include <stdexcept>
#include <sstream>
#include "AST.h"
#include "printutils.h"

class EvaluationException : public std::runtime_error
{
public:
  EvaluationException(const std::string& what_arg) : std::runtime_error(what_arg) {
    this->traceDepth = OpenSCAD::traceDepth;
  }
  ~EvaluationException() throw() {}
public:
  int traceDepth = 0;
};

class AssertionFailedException : public EvaluationException
{
public:
  AssertionFailedException(const std::string& what_arg, const Location& loc) : EvaluationException(what_arg), loc(loc) {}
  ~AssertionFailedException() throw() {}

public:
  Location loc;
};

class RecursionException : public EvaluationException
{
public:
  static RecursionException create(const std::string& recursiontype, const std::string& name, const Location& loc) {
    return RecursionException{STR("Recursion detected calling ", recursiontype, " '", name, "'"), loc};
  }
  ~RecursionException() throw() {}

public:
  Location loc;

private:
  RecursionException(const std::string& what_arg, const Location& loc) : EvaluationException(what_arg), loc(loc) {}
};

class LoopCntException : public EvaluationException
{
public:
  static LoopCntException create(const std::string& type, const Location& loc) {
    return LoopCntException{STR(type, " loop counter exceeded limit"), loc};
  }
  ~LoopCntException() throw() {}

public:
  Location loc;

private:
  LoopCntException(const std::string& what_arg, const Location& loc) : EvaluationException(what_arg), loc(loc) {}
};

class VectorEchoStringException : public EvaluationException
{
public:
  static VectorEchoStringException create() {
    return VectorEchoStringException{"Stack exhausted while trying to convert a vector to EchoString"};
  }
  ~VectorEchoStringException() throw() {}

private:
  VectorEchoStringException(const std::string& what_arg) : EvaluationException(what_arg) {}
};

class HardWarningException : public EvaluationException
{
public:
  HardWarningException(const std::string& what_arg) : EvaluationException(what_arg) {}
  ~HardWarningException() throw() {}
};
