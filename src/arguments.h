#pragma once

#include <ostream>
#include <vector>
#include <boost/optional.hpp>

#include "Assignment.h"
#include "context.h"

struct Argument {
	boost::optional<std::string> name;
	Value value;
	
	Argument(boost::optional<std::string> name, Value value): name(name), value(std::move(value)) {}
	Argument(Argument&& other) = default;
	Argument& operator=(Argument&& other) = default;
	
	const Value* operator->() const { return &value; }
	Value* operator->() { return &value; }
};

class Arguments : public std::vector<Argument>
{
	public:
	Arguments(const AssignmentList& argument_expressions, const std::shared_ptr<const Context>& context);
	Arguments(Arguments&& other) = default;
	Arguments& operator=(Arguments&& other) = default;
	
	EvaluationSession* session() const { return evaluation_session; }
	const std::string &documentRoot() const { return evaluation_session->documentRoot(); }
	
	private:
	EvaluationSession* evaluation_session;
};

std::ostream &operator<<(std::ostream &stream, const Argument& argument);
std::ostream &operator<<(std::ostream &stream, const Arguments& arguments);
