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

Context::Context(EvaluationSession* session):
	ContextFrame(session),
	parent(nullptr)
{}

Context::Context(const std::shared_ptr<Context>& parent):
	ContextFrame(parent->evaluation_session),
	parent(parent)
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

const Value& Context::lookup_variable(const std::string &name, bool silent, const Location &loc) const
{
	if (is_config_variable(name)) {
		return session()->lookup_special_variable(name, silent, loc);
	}
	for (const Context* context = this; context != nullptr; context = context->getParent().get()) {
		boost::optional<const Value&> value = context->lookup_local_variable(name);
		if (value) {
			return *value;
		}
	}
	if (!silent) {
		LOG(message_group::Warning,loc,documentRoot(),"Ignoring unknown variable '%1$s'",name);
	}
	return Value::undefined;
}

boost::optional<CallableFunction> Context::lookup_function(const std::string &name, const Location &loc) const
{
	if (is_config_variable(name)) {
		return session()->lookup_special_function(name, loc);
	}
	for (const Context* context = this; context != nullptr; context = context->getParent().get()) {
		boost::optional<CallableFunction> result = context->lookup_local_function(name, loc);
		if (result) {
			return result;
		}
	}
	LOG(message_group::Warning,loc,documentRoot(),"Ignoring unknown function '%1$s'",name);
	return boost::none;
}

boost::optional<InstantiableModule> Context::lookup_module(const std::string &name, const Location &loc) const
{
	if (is_config_variable(name)) {
		return session()->lookup_special_module(name, loc);
	}
	for (const Context* context = this; context != nullptr; context = context->getParent().get()) {
		boost::optional<InstantiableModule> result = context->lookup_local_module(name, loc);
		if (result) {
			return result;
		}
	}
	LOG(message_group::Warning,loc,this->documentRoot(),"Ignoring unknown module '%1$s'",name);
	return boost::none;
}

double Context::lookup_variable_with_default(const std::string &variable, const double &def) const
{
	const Value& v = this->lookup_variable(variable, true);
	return (v.type() == Value::Type::NUMBER) ? v.toDouble() : def;
}

const std::string& Context::lookup_variable_with_default(const std::string &variable, const std::string &def) const
{
	const Value& v = this->lookup_variable(variable, true);
	return (v.type() == Value::Type::STRING) ? v.toStrUtf8Wrapper().toString() : def;
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
				s << boost::format("    %s = %s\n") % parameter->getName() % lookup_variable(parameter->getName());
			}
		}
	}
	s << ContextFrame::dump();
	return s.str();
}
#endif

