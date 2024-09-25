#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "core/Assignment.h"
#include "core/Context.h"

struct Argument {
  boost::optional<std::string> name;
  Value value;

  Argument(boost::optional<std::string> name, Value value) : name(std::move(name)), value(std::move(value)) {
  }
  Argument(Argument&& other) = default;
  Argument& operator=(Argument&& other) = default;
  Argument(const Argument& other) = delete;
  Argument& operator=(const Argument& other) = delete;
  ~Argument() = default;

  const Value *operator->() const { return &value; }
  Value *operator->() { return &value; }
};

class Arguments : public std::vector<Argument>
{
public:
  Arguments(const AssignmentList& argument_expressions, const std::shared_ptr<const Context>& context);
  Arguments(Arguments&& other) = default;
  Arguments& operator=(Arguments&& other) = default;
  Arguments(const Arguments& other) = delete;
  Arguments& operator=(const Arguments& other) = delete;
  ~Arguments() = default;

private:
  Arguments(EvaluationSession *session) : evaluation_session(session) {}

public:
  [[nodiscard]] Arguments clone() const;

  [[nodiscard]] EvaluationSession *session() const { return evaluation_session; }
  [[nodiscard]] const std::string& documentRoot() const { return evaluation_session->documentRoot(); }

private:
  EvaluationSession *evaluation_session;
};

std::ostream& operator<<(std::ostream& stream, const Argument& argument);
std::ostream& operator<<(std::ostream& stream, const Arguments& arguments);
