/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
#include "compiler_specific.h"
#include "context.h"
#include "evalcontext.h"
#include "expression.h"
#include "function.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "builtin.h"
#include "printutils.h"
#include <boost/filesystem.hpp>
#include "boost-utils.h"
namespace fs = boost::filesystem;

// $children is not a config_variable. config_variables have dynamic scope, 
// meaning they are passed down the call chain implicitly.
// $children is simply misnamed and shouldn't have included the '$'.
static bool is_config_variable(const std::string &name)
{
	return name[0] == '$' && name != "$children";
}

Context::Context(EvaluationSession* session):
	parent(nullptr),
	evaluation_session(session)
{}

Context::Context(const std::shared_ptr<Context> parent):
	parent(parent),
	evaluation_session(parent->evaluation_session)
{}

Context::~Context()
{}

/*!
	Initialize context from a module parameter list and a evaluation context
	which may pass variables which will be preferred over default values.
*/
void Context::setVariables(const std::shared_ptr<EvalContext> &evalctx, const AssignmentList &parameters, const AssignmentList &optional_parameters, bool usermodule)
{
	// Set any default values
	for (const auto &parameter : parameters) {
		// FIXME should we just not set value if parameter.expr is false?
		set_variable(parameter->getName(), parameter->getExpr() ? parameter->getExpr()->evaluate(this->parent) : Value::undefined.clone());
	}
	
	if (evalctx) {
		auto assignments = evalctx->resolveArguments(parameters, optional_parameters, usermodule && !OpenSCAD::parameterCheck);
		for (const auto &ass : assignments) {
			this->set_variable(ass.first, ass.second->evaluate(evalctx));
		}
	}
}

// sink for value takes &&
void Context::set_variable(const std::string &name, Value&& value)
{
	if (is_config_variable(name)) {
		this->config_variables.insert_or_assign(name, std::move(value));
	} else {
		this->variables.insert_or_assign(name, std::move(value));
	}
}

void Context::apply_variables(const ValueMap& variables)
{
	for (const auto& variable : variables) {
		set_variable(variable.first, variable.second.clone());
	}
}

void Context::apply_variables(const std::shared_ptr<Context> &other)
{
	this->variables.applyFrom(other->variables);
}

void Context::apply_config_variables(const std::shared_ptr<Context> &other)
{
	this->config_variables.applyFrom(other->config_variables);
}

const Value& Context::lookup_variable(const std::string &name, bool silent, const Location &loc) const
{
	ValueMap::const_iterator result;
	if (is_config_variable(name)) {
		const auto& stack = session()->getStack();
		for (int i = stack.size()-1; i >= 0; i--) {
			const auto &confvars = stack.at(i)->config_variables;
			if ((result = confvars.find(name)) != confvars.end()) {
				return result->second;
			}
		}
		if (!silent) {
			LOG(message_group::Warning,loc,this->documentRoot(),"Ignoring unknown variable '%1$s'",name);
		}
		return Value::undefined;
	}
	if ((result = this->variables.find(name)) != this->variables.end()) {
		return result->second;
	}
	if (this->parent) {
		return this->parent->lookup_variable(name, silent, loc);
	}
	if (!silent) {
		LOG(message_group::Warning,loc,this->documentRoot(),"Ignoring unknown variable '%1$s'",name);
	}
	return Value::undefined;
}


double Context::lookup_variable_with_default(const std::string &variable, const double &def, const Location &loc) const
{
	const Value& v = this->lookup_variable(variable, true, loc);
	return (v.type() == Value::Type::NUMBER) ? v.toDouble() : def;
}

const std::string& Context::lookup_variable_with_default(const std::string &variable, const std::string &def, const Location &loc) const
{
	const Value& v = this->lookup_variable(variable, true, loc);
	return (v.type() == Value::Type::STRING) ? v.toStrUtf8Wrapper().toString() : def;
}

Value Context::lookup_local_config_variable(const std::string &name) const
{
	if (is_config_variable(name)) {
  	ValueMap::const_iterator result;
		if ((result = config_variables.find(name)) != config_variables.end()) {
			return result->second.clone();
		}
	}
	return Value::undefined.clone();
}

bool Context::has_local_variable(const std::string &name) const
{
	if (is_config_variable(name)) {
		return config_variables.find(name) != config_variables.end();
	}
	return variables.find(name) != variables.end();
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the methods using it evaluate_function() and instantiate_module()
 * are called often when recursive functions or modules are evaluated.
 * noinline prevents compiler optimization, as we here specifically
 * optimize for stack usage during normal operating, not runtime during
 * error handling.
 *
 * @param what what is ignored
 * @param name name of the ignored object
 * @param loc location of the function/module call
 * @param docPath document path of the root file, used to calculate the relative path
 */
static void NOINLINE print_ignore_warning(const char *what, const char *name, const Location &loc, const char *docPath){
	LOG(message_group::Warning,loc,docPath,"Ignoring unknown %1$s '%2$s'",what,name);
}

boost::optional<CallableFunction> Context::lookup_local_function(const std::string &name, const Location &loc) const
{
	if (is_config_variable(name)) {
		ValueMap::const_iterator result = config_variables.find(name);
		if (result != config_variables.end()) {
			if (result->second.type() == Value::Type::FUNCTION) {
				return CallableFunction{&result->second};
			}
		}
	} else {
		ValueMap::const_iterator result = variables.find(name);
		if (result != variables.end()) {
			if (result->second.type() == Value::Type::FUNCTION) {
				return CallableFunction{&result->second};
			}
		}
	}
	return boost::none;
}

boost::optional<CallableFunction> Context::lookup_function(const std::string &name, const Location &loc) const
{
	if (is_config_variable(name)) {
		const auto& stack = session()->getStack();
		for (int i = stack.size()-1; i >= 0; i--) {
			auto result = stack.at(i)->lookup_local_function(name, loc);
			if (result) {
				return result;
			}
		}
	} else {
		std::shared_ptr<Context> context = get_shared_ptr();
		while (context) {
			auto result = context->lookup_local_function(name, loc);
			if (result) {
				return result;
			}
			context = context->parent;
		}
	}
	return boost::none;
}

boost::optional<InstantiableModule> Context::lookup_local_module(const std::string &name, const Location &loc) const
{
	return boost::none;
}

boost::optional<InstantiableModule> Context::lookup_module(const std::string &name, const Location &loc) const
{
	if (is_config_variable(name)) {
		const auto& stack = session()->getStack();
		for (int i = stack.size()-1; i >= 0; i--) {
			auto result = stack.at(i)->lookup_local_module(name, loc);
			if (result) {
				return result;
			}
		}
	} else {
		std::shared_ptr<Context> context = get_shared_ptr();
		while (context) {
			auto result = context->lookup_local_module(name, loc);
			if (result) {
				return result;
			}
			context = context->parent;
		}
	}
	LOG(message_group::Warning,loc,this->documentRoot(),"Ignoring unknown module '%1$s'",name);
	return boost::none;
}

#ifdef DEBUG
std::string Context::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	std::ostringstream s;
	if (inst) {
		s << boost::format("ModuleContext %p (%p) for %s inst (%p)\n") % this % this->parent % inst->name() % inst;
	}
	else {
		s << boost::format("Context: %p (%p)\n") % this % this->parent;
	}
	if (mod) {
		const UserModule *m = dynamic_cast<const UserModule*>(mod);
		if (m) {
			s << "  module parameters:";
			for(const auto &parameter: m->parameters) {
				s << boost::format("    %s = %s\n") % parameter->getName() % variables.get(parameter->getName());
			}
		}
	}
	s << "  vars:\n";
	for(const auto &v : variables) {
		s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
	}
	for(const auto &v : config_variables) {
		s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
	}
	return s.str();
}
#endif

