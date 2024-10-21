#include "core/Builtins.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "core/function.h"
#include "core/module.h"
#include "core/Expression.h"

std::unordered_map<std::string, const std::vector<std::string>> Builtins::keywordList;

Builtins *Builtins::instance(bool erase)
{
  static auto *builtins = new Builtins;
  if (erase) {
    delete builtins;
    builtins = nullptr;
  }
  return builtins;
}

void Builtins::init(const std::string& name, class AbstractModule *module)
{
#ifndef ENABLE_EXPERIMENTAL
  if (module->is_experimental()) return;
#endif
  Builtins::instance()->modules.emplace(name, module);
}

void Builtins::init(const std::string& name, AbstractModule *module, const std::vector<std::string>& calltipList)
{
#ifndef ENABLE_EXPERIMENTAL
  if (module->is_experimental()) return;
#endif
  Builtins::instance()->modules.emplace(name, module);
  Builtins::keywordList.insert({name, calltipList});
}

void Builtins::init(const std::string& name, BuiltinFunction *function, const std::vector<std::string>& calltipList)
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
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
extern void register_builtin_roof();
#endif
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
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  register_builtin_roof();
#endif
  register_builtin_text();

  this->deprecations.emplace("dxf_linear_extrude", "linear_extrude()");
  this->deprecations.emplace("dxf_rotate_extrude", "rotate_extrude()");
  this->deprecations.emplace("import_stl", "import()");
  this->deprecations.emplace("import_dxf", "import()");
  this->deprecations.emplace("import_off", "import()");
  this->deprecations.emplace("assign", "a regular assignment");
}

std::string Builtins::isDeprecated(const std::string& name) const
{
  if (this->deprecations.find(name) != this->deprecations.end()) {
    return this->deprecations.at(name);
  }
  return {};
}

Builtins::Builtins()
{
  this->assignments.emplace_back(new Assignment("$fn", std::make_shared<Literal>(0.0)) );
  this->assignments.emplace_back(new Assignment("$fs", std::make_shared<Literal>(2.0)) );
  this->assignments.emplace_back(new Assignment("$fa", std::make_shared<Literal>(12.0)) );
  this->assignments.emplace_back(new Assignment("$t", std::make_shared<Literal>(0.0)) );
  this->assignments.emplace_back(new Assignment("$preview", std::make_shared<Literal>()) ); //undef as should always be overwritten.
  auto zeroVector = std::make_shared<Vector>(Location::NONE);
  zeroVector->emplace_back(new Literal(0.0));
  zeroVector->emplace_back(new Literal(0.0));
  zeroVector->emplace_back(new Literal(0.0));
  this->assignments.emplace_back(new Assignment("$vpt", zeroVector) );
  this->assignments.emplace_back(new Assignment("$vpr", zeroVector) );
  this->assignments.emplace_back(new Assignment("$vpd", std::make_shared<Literal>(500.0)) );
  this->assignments.emplace_back(new Assignment("$vpf", std::make_shared<Literal>(22.5)) );
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
