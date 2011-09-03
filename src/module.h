#ifndef MODULE_H_
#define MODULE_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include "value.h"

class ModuleInstantiation
{
public:
	std::string label;
	std::string modname;
	std::vector<std::string> argnames;
	std::vector<class Expression*> argexpr;
	std::vector<Value> argvalues;
	std::vector<ModuleInstantiation*> children;

	bool tag_root;
	bool tag_highlight;
	bool tag_background;
	const class Context *ctx;

	ModuleInstantiation() : tag_root(false), tag_highlight(false), tag_background(false), ctx(NULL) { }
	virtual ~ModuleInstantiation();

	std::string dump(const std::string &indent) const;
	class AbstractNode *evaluate(const Context *ctx) const;
};

class IfElseModuleInstantiation : public ModuleInstantiation {
public:
	virtual ~IfElseModuleInstantiation();

	std::vector<ModuleInstantiation*> else_children;
};

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual class AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Module : public AbstractModule
{
public:
	typedef boost::unordered_map<std::string, class Module*> ModuleContainer;
	ModuleContainer usedlibs;

	struct libs_cache_ent {
		Module *mod;
		std::string cache_id, msg;
	};
	static boost::unordered_map<std::string, libs_cache_ent> libs_cache;
	static Module *compile_library(std::string filename);

	std::vector<std::string> argnames;
	std::vector<Expression*> argexpr;

	std::vector<std::string> assignments_var;
	std::vector<Expression*> assignments_expr;

	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef boost::unordered_map<std::string, AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;

	std::vector<ModuleInstantiation*> children;

	Module() { }
	virtual ~Module();

	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

#endif
