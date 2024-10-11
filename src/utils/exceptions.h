#pragma once

#include <stdexcept>
#include <utility>
#include <string>
#include "core/AST.h"
#include "utils/printutils.h"

class EvaluationException : public std::runtime_error
{
public:
  EvaluationException(const std::string& what_arg) : std::runtime_error(what_arg), traceDepth(OpenSCAD::traceDepth) {}

public:
  int traceDepth = 0;
};

class AssertionFailedException : public EvaluationException
{
public:
  AssertionFailedException(const std::string& what_arg, Location loc) : EvaluationException(what_arg), loc(std::move(loc)) {}

public:
  Location loc;
};

class RecursionException : public EvaluationException
{
public:
  static RecursionException create(const std::string& recursiontype, const std::string& name, const Location& loc) {
    return RecursionException{STR("Recursion detected calling ", recursiontype, " '", name, "'"), loc};
  }

public:
  Location loc;

private:
  RecursionException(const std::string& what_arg, Location loc) : EvaluationException(what_arg), loc(std::move(loc)) {}
};

class LoopCntException : public EvaluationException
{
public:
  static LoopCntException create(const std::string& type, const Location& loc) {
    return LoopCntException{STR(type, " loop counter exceeded limit"), loc};
  }

public:
  Location loc;

private:
  LoopCntException(const std::string& what_arg, Location loc) : EvaluationException(what_arg), loc(std::move(loc)) {}
};

class VectorEchoStringException : public EvaluationException
{
public:
  static VectorEchoStringException create() {
    return VectorEchoStringException{"Stack exhausted while trying to convert a vector to EchoString"};
  }

private:
  VectorEchoStringException(const std::string& what_arg) : EvaluationException(what_arg) {}
};

class HardWarningException : public EvaluationException
{
public:
  HardWarningException(const std::string& what_arg) : EvaluationException(what_arg) {}
};
