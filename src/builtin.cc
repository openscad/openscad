#include "builtin.h"
#include "function.h"
#include "module.h"
#include "expression.h"

std::unordered_map<std::string, const std::vector<std::string>> Builtins::keywordList;

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

void Builtins::init(const std::string &name, class AbstractModule *module, const std::vector<std::string> &calltipList)
{
#ifndef ENABLE_EXPERIMENTAL
	if (module->is_experimental()) return;
#endif
	Builtins::instance()->modules.emplace(name, module);
	Builtins::keywordList.insert({name, calltipList});
}

void Builtins::init(const std::string &name, class AbstractFunction *function, const std::vector<std::string> &calltipList)
{
#ifndef ENABLE_EXPERIMENTAL
	if (function->is_experimental()) return;
#endif
	Builtins::instance()->functions.emplace(name, function);
	Builtins::keywordList.insert({name, calltipList});
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
	Builtins::initKeywordList();

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
	this->assignments.emplace_back(new Assignment("$fn", make_shared<Literal>(0.0)) );
	this->assignments.emplace_back(new Assignment("$fs", make_shared<Literal>(2.0)) );
	this->assignments.emplace_back(new Assignment("$fa", make_shared<Literal>(12.0)) );
	this->assignments.emplace_back(new Assignment("$t", make_shared<Literal>(0.0)) );
	this->assignments.emplace_back(new Assignment("$preview", make_shared<Literal>(Value::undefined.clone())) ); //undef as should always be overwritten.
	this->assignments.emplace_back(new Assignment("$vpt", make_shared<Literal>(VectorType(0.0, 0.0, 0.0))) );
	this->assignments.emplace_back(new Assignment("$vpr", make_shared<Literal>(VectorType(0.0, 0.0, 0.0))) );
	this->assignments.emplace_back(new Assignment("$vpd", make_shared<Literal>(500)) );
	this->assignments.emplace_back(new Assignment("$vpf", make_shared<Literal>(22.5)) );
}

void Builtins::initKeywordList()
{
	Builtins::keywordList.insert({"else", {}});
	Builtins::keywordList.insert({"each", {}});
	Builtins::keywordList.insert({"module", {}});
	Builtins::keywordList.insert({"function", {}});
	Builtins::keywordList.insert({"true", {}});
	Builtins::keywordList.insert({"false", {}});
	Builtins::keywordList.insert({"undef", {}});
	Builtins::keywordList.insert({"use", {}});
	Builtins::keywordList.insert({"include", {}});
}
