#pragma once

#include <stdexcept>
#include <sstream>
#include <utility>
#include "AST.h"
#include "printutils.h"

class EvaluationException : public std::runtime_error
{
public:
  EvaluationException(std::string what_arg) : std::runtime_error(std::move(what_arg)), traceDepth(OpenSCAD::traceDepth) {}
  ~EvaluationException() noexcept = default;
public:
  int traceDepth = 0;
};

class AssertionFailedException : public EvaluationException
{
public:
  AssertionFailedException(std::string what_arg, Location loc) : EvaluationException(std::move(what_arg)), loc(std::move(loc)) {}
  ~AssertionFailedException() noexcept = default;

public:
  Location loc;
};

class RecursionException : public EvaluationException
{
public:
  static RecursionException create(std::string recursiontype, std::string name, const Location& loc) {
    return RecursionException{STR("Recursion detected calling ", std::move(recursiontype), " '", std::move(name), "'"), loc};
  }
  ~RecursionException() noexcept = default;

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
  ~LoopCntException() noexcept = default;

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
  ~VectorEchoStringException() noexcept = default;

private:
  VectorEchoStringException(const std::string& what_arg) : EvaluationException(what_arg) {}
};

class HardWarningException : public EvaluationException
{
public:
  HardWarningException(const std::string& what_arg) : EvaluationException(what_arg) {}
  ~HardWarningException() noexcept = default;
};
