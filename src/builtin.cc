#include "builtin.h"
#include "function.h"
#include "module.h"
#include "expression.h"

Builtins *Builtins::instance(bool erase)
{
	static Builtins *builtins = new Builtins;
	if (erase) {
		delete builtins;
		builtins = nullptr;
	}
	return builtins;
}

void Builtins::init(const std::string &name, class AbstractModule *module)
{
#ifndef ENABLE_EXPERIMENTAL
	if (module->is_experimental()) return;
#endif
	Builtins::instance()->modules.emplace(name, module);
}

void Builtins::init(const std::string &name, class AbstractFunction *function)
{
#ifndef ENABLE_EXPERIMENTAL
	if (function->is_experimental()) return;
#endif
	Builtins::instance()->functions.emplace(name, function);
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

	this->deprecations.emplace("dxf_linear_extrude", "linear_extrude()");
	this->deprecations.emplace("dxf_rotate_extrude", "rotate_extrude()");
	this->deprecations.emplace("import_stl", "import()");
	this->deprecations.emplace("import_dxf", "import()");
	this->deprecations.emplace("import_off", "import()");
	this->deprecations.emplace("assign", "a regular assignment");
}

std::string Builtins::isDeprecated(const std::string &name) const
{
	if (this->deprecations.find(name) != this->deprecations.end()) {
		return this->deprecations.at(name);
	}
	return {};
}

Builtins::Builtins()
{
	this->assignments.emplace_back("$fn", make_shared<Literal>(0.0));
	this->assignments.emplace_back("$fs", make_shared<Literal>(2.0));
	this->assignments.emplace_back("$fa", make_shared<Literal>(12.0));
	this->assignments.emplace_back("$t", make_shared<Literal>(0.0));
	this->assignments.emplace_back("$preview", make_shared<Literal>(ValuePtr::undefined)); //undef as should always be overwritten.

	Value::VectorType zero3{0.0, 0.0, 0.0};
	this->assignments.emplace_back("$vpt", make_shared<Literal>(zero3));
	this->assignments.emplace_back("$vpr", make_shared<Literal>(zero3));
	this->assignments.emplace_back("$vpd", make_shared<Literal>(500));
}
