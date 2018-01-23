#include "builtin.h"
#include "function.h"
#include "module.h"
#include "expression.h"

Builtins *Builtins::instance(bool erase)
{
	static Builtins *s_builtins = new Builtins;
	if (erase) {
		delete s_builtins;
		s_builtins = nullptr;
	}
	return s_builtins;
}

void Builtins::init(const char *name, class AbstractModule *module)
{
#ifndef ENABLE_EXPERIMENTAL
	if (module->is_experimental()) {
		return;
	}
#endif
	Builtins::instance()->modules[name] = module;
}

void Builtins::init(const char *name, class AbstractFunction *function)
{
#ifndef ENABLE_EXPERIMENTAL
	if (function->is_experimental()) {
		return;
	}
#endif
	Builtins::instance()->functions[name] = function;
}

extern void register_builtin_functions();
extern void register_builtin_group();
extern void register_builtin_csgops();
extern void register_builtin_transform();
extern void register_builtin_color();
extern void register_builtin_primitives();
extern void register_builtin_surface();
extern void register_builtin_control();
extern void register_builtin_render();
extern void register_builtin_import();
extern void register_builtin_projection();
extern void register_builtin_cgaladv();
extern void register_builtin_offset();
extern void register_builtin_dxf_linear_extrude();
extern void register_builtin_dxf_rotate_extrude();
extern void register_builtin_text();
extern void initialize_builtin_dxf_dim();

/*!
	Registers all builtin functions.
	Call once for the whole app.
*/
void Builtins::initialize()
{
	register_builtin_functions();
	initialize_builtin_dxf_dim();

	register_builtin_group();
	register_builtin_csgops();
	register_builtin_transform();
	register_builtin_color();
	register_builtin_primitives();
	register_builtin_surface();
	register_builtin_control();
	register_builtin_render();
	register_builtin_import();
	register_builtin_projection();
	register_builtin_cgaladv();
	register_builtin_offset();
	register_builtin_dxf_linear_extrude();
	register_builtin_dxf_rotate_extrude();
	register_builtin_text();

	this->deprecations["dxf_linear_extrude"] = "linear_extrude()";
	this->deprecations["dxf_rotate_extrude"] = "rotate_extrude()";
	this->deprecations["import_stl"] = "import()";
	this->deprecations["import_dxf"] = "import()";
	this->deprecations["import_off"] = "import()";
	this->deprecations["assign"] = "a regular assignment";
}

std::string Builtins::isDeprecated(const std::string &name)
{
	if (this->deprecations.find(name) != this->deprecations.end()) {
		return this->deprecations[name];
	}
	return std::string();
}

Builtins::Builtins()
{
	this->assignments.push_back(Assignment("$fn", shared_ptr<Expression>(new Literal(ValuePtr(0.0)))));
	this->assignments.push_back(Assignment("$fs", shared_ptr<Expression>(new Literal(ValuePtr(2.0)))));
	this->assignments.push_back(Assignment("$fa", shared_ptr<Expression>(new Literal(ValuePtr(12.0)))));
	this->assignments.push_back(Assignment("$t", shared_ptr<Expression>(new Literal(ValuePtr(0.0)))));
	this->assignments.push_back(Assignment("$preview", shared_ptr<Expression>(new Literal(ValuePtr())))); //undef as should always be overwritten.

	Value::VectorType zero3;
	zero3.push_back(ValuePtr(0.0));
	zero3.push_back(ValuePtr(0.0));
	zero3.push_back(ValuePtr(0.0));
	ValuePtr zero3val(zero3);
	this->assignments.push_back(Assignment("$vpt", shared_ptr<Expression>(new Literal(zero3val))));
	this->assignments.push_back(Assignment("$vpr", shared_ptr<Expression>(new Literal(zero3val))));
	this->assignments.push_back(Assignment("$vpd", shared_ptr<Expression>(new Literal(ValuePtr(500)))));
}

Builtins::~Builtins()
{
}
