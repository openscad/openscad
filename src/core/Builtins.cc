#include "core/Builtins.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/AST.h"
#include "core/Expression.h"
#include "core/function.h"
#include "core/module.h"

std::unordered_map<std::string, std::vector<std::string>> Builtins::keywordList;

Builtins& Builtins::instance()
{
  static Builtins builtins;
  return builtins;
}

void Builtins::init(const std::string& name, AbstractModule *module)
{
#ifndef ENABLE_EXPERIMENTAL
  if (module->is_experimental()) return;
#endif
  instance().modules.emplace(name, module);
}

void Builtins::init(const std::string& name, AbstractModule *module,
                    const std::vector<std::string>& calltipList)
{
#ifndef ENABLE_EXPERIMENTAL
  if (module->is_experimental()) return;
#endif
  instance().modules.emplace(name, module);
  keywordList.insert({name, calltipList});
}

void Builtins::init(const std::string& name, BuiltinFunction *function,
                    const std::vector<std::string>& calltipList)
{
#ifndef ENABLE_EXPERIMENTAL
  if (function->is_experimental()) return;
#endif
  instance().functions.emplace(name, function);
  keywordList.insert({name, calltipList});
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
extern void register_builtin_linear_extrude();
extern void register_builtin_rotate_extrude();
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
  initKeywordList();

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
  register_builtin_linear_extrude();
  register_builtin_rotate_extrude();
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
  register_builtin_roof();
#endif
  register_builtin_text();
}

std::string Builtins::isDeprecated(const std::string& name) const
{
  auto it = deprecations.find(name);
  if (it != deprecations.end()) {
    return it->second;
  }
  return {};
}

Builtins::Builtins()
{
  this->assignments.emplace_back(new Assignment("$fn", std::make_shared<Literal>(0.0)));
  // $fe doesn't need initializing because `undef` is treated identical to 0.0,
  // but it could be used for feature detection when complete,
  // but does have the problem that these are initialized only once in `openscad_main`,
  // so enabling the feature wouldn't initialize the variable until the program is restarted.
  this->assignments.emplace_back(new Assignment("$fs", std::make_shared<Literal>(2.0)));
  this->assignments.emplace_back(new Assignment("$fa", std::make_shared<Literal>(12.0)));
  this->assignments.emplace_back(new Assignment("$t", std::make_shared<Literal>(0.0)));
  this->assignments.emplace_back(
    new Assignment("$preview", std::make_shared<Literal>()));  // undef as should always be overwritten.
  auto zeroVector = std::make_shared<Vector>(Location::NONE);
  zeroVector->emplace_back(new Literal(0.0));
  zeroVector->emplace_back(new Literal(0.0));
  zeroVector->emplace_back(new Literal(0.0));
  this->assignments.emplace_back(new Assignment("$vpt", zeroVector));
  this->assignments.emplace_back(new Assignment("$vpr", zeroVector));
  this->assignments.emplace_back(new Assignment("$vpd", std::make_shared<Literal>(500.0)));
  this->assignments.emplace_back(new Assignment("$vpf", std::make_shared<Literal>(22.5)));
}

void Builtins::initKeywordList()
{
  keywordList.insert({"else", {}});
  keywordList.insert({"each", {}});
  keywordList.insert({"module", {}});
  keywordList.insert({"function", {}});
  keywordList.insert({"true", {}});
  keywordList.insert({"false", {}});
  keywordList.insert({"undef", {}});
  keywordList.insert({"use", {}});
  keywordList.insert({"include", {}});
}
