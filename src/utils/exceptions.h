#pragma once

#include <stdexcept>
#include <utility>
#include <string>
#include <functional>

#include "core/AST.h"
#include "utils/defer_call.h"
#include "utils/printutils.h"
#include "boost/circular_buffer.hpp"

class EvaluationException : public std::runtime_error
{
public:
  EvaluationException(const std::string& what_arg)
    : std::runtime_error(what_arg), traceDepth(OpenSCAD::traceDepth), tail_msgs(OpenSCAD::traceDepth)
  {
  }

  template <typename... Args>
  void LOG(const message_group& msgGroup, Args&&...args)
  {
    if (traceDepth > 0) {
      ::LOG(msgGroup, std::forward<Args>(args)...);
    } else {
      tail_msgs.push_back(
        defer_call([](auto&&...args) { return make_message_obj(std::forward<decltype(args)>(args)...); },
                   msgGroup, std::forward<Args>(args)...));
    }
  }

  ~EvaluationException()
  {
    int frames_skipped = -(traceDepth + tail_msgs.size());
    if (frames_skipped > 0) {
      ::PRINT(Message(std::string{"  *** Excluding "} + std::to_string(frames_skipped) + " frames ***",
                      message_group::Trace));
    }

    while (!tail_msgs.empty()) {
      if (auto msg = tail_msgs.front()()) {
        ::PRINT(*msg);
      }
      tail_msgs.pop_front();
    }
  }

public:
  int traceDepth = 0;
  boost::circular_buffer<std::function<std::optional<Message>()>> tail_msgs;
};

class AssertionFailedException : public EvaluationException
{
public:
  AssertionFailedException(const std::string& what_arg, Location loc)
    : EvaluationException(what_arg), loc(std::move(loc))
  {
  }

public:
  Location loc;
};

class RecursionException : public EvaluationException
{
public:
  static RecursionException create(const std::string& recursiontype, const std::string& name,
                                   const Location& loc)
  {
    return RecursionException{STR("Recursion detected calling ", recursiontype, " '", name, "'"), loc};
  }

public:
  Location loc;

private:
  RecursionException(const std::string& what_arg, Location loc)
    : EvaluationException(what_arg), loc(std::move(loc))
  {
  }
};

class LoopCntException : public EvaluationException
{
public:
  static LoopCntException create(const std::string& type, const Location& loc)
  {
    return LoopCntException{STR(type, " loop counter exceeded limit"), loc};
  }

public:
  Location loc;

private:
  LoopCntException(const std::string& what_arg, Location loc)
    : EvaluationException(what_arg), loc(std::move(loc))
  {
  }
};

class VectorEchoStringException : public EvaluationException
{
public:
  static VectorEchoStringException create()
  {
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
