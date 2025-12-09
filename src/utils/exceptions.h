#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "boost/circular_buffer.hpp"
#include "core/AST.h"
#include "core/Context.h"
#include "utils/defer_call.h"
#include "utils/printutils.h"
#include "utils/CallTraceStack.h"

class EvaluationException : public std::runtime_error
{
public:
  EvaluationException(const std::string& what_arg)
    : std::runtime_error(what_arg), traceDepth(OpenSCAD::traceDepth), tail_msgs(OpenSCAD::traceDepth)
  {
  }

  // Copy constructor: DO NOT copy tail_msgs to avoid double-processing and corruption.
  EvaluationException(const EvaluationException& other)
    : std::runtime_error(other), traceDepth(other.traceDepth), tail_msgs(other.traceDepth)
  {
  }

  // Move constructor: take ownership of tail_msgs
  EvaluationException(EvaluationException&& other) noexcept
    : std::runtime_error(std::move(other)), traceDepth(other.traceDepth), tail_msgs(std::move(other.tail_msgs))
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

  // Print the call trace from CallTraceStack
  // This should be called at the TOP-LEVEL catch site only
  void printCallTrace() const {
    const auto& stack = CallTraceStack::getStack();
    int depth = traceDepth;
    
    // Print from most recent (top of stack) down
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
      auto ctx = it->context.lock();
      std::string docRoot = ctx ? ctx->documentRoot() : "";
      
      if (depth > 0) {
        // Print immediately
        switch (it->type) {
          case CallTraceStack::Entry::Type::FunctionCall:
            ::LOG(message_group::Trace, it->location, docRoot, "called by '%1$s'", it->name);
            break;
          case CallTraceStack::Entry::Type::ModuleInstantiation:
            ::LOG(message_group::Trace, it->location, docRoot, "called by '%1$s'", it->name);
            break;
          case CallTraceStack::Entry::Type::UserModuleCall:
            if (it->getParameterString) {
              ::LOG(message_group::Trace, it->location, docRoot, "call of '%1$s(%2$s)'",
                    it->name, it->getParameterString());
            } else {
              ::LOG(message_group::Trace, it->location, docRoot, "call of '%1$s'", it->name);
            }
            break;
          case CallTraceStack::Entry::Type::Assignment:
            if (it->overwriteLocation.isNone()) {
              ::LOG(message_group::Trace, it->location, docRoot, "assignment to %1$s", 
                    quoteVar(it->name));
            } else {
              ::LOG(message_group::Trace, it->location, docRoot, 
                    "overwritten assignment to %1$s (this is where the assignment is evaluated)",
                    quoteVar(it->name));
              ::LOG(message_group::Trace, it->overwriteLocation, docRoot,
                    "overwriting assignment to %1$s", quoteVar(it->name));
            }
            break;
        }
        depth--;
      }
    }
    
    // Report excluded frames
    int frames_skipped = static_cast<int>(stack.size()) - traceDepth;
    if (frames_skipped > 0) {
      ::PRINT(Message(std::string{"  *** Excluding "} + std::to_string(frames_skipped) + " frames ***",
                      message_group::Trace));
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
