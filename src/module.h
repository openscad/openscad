#ifndef MODULE_H_
#define MODULE_H_

#include <string>
#include <vector>
#include <list>
#include <boost/unordered_map.hpp>
#include "value.h"

class ModuleInstantiation
{
public:
	ModuleInstantiation(const std::string &name = "") 
	: ctx(NULL), 
		tag_root(false), tag_highlight(false), tag_background(false), modname(name) { }
	virtual ~ModuleInstantiation();

	std::string dump(const std::string &indent) const;
	class AbstractNode *evaluate(const class Context *ctx) const;
	std::vector<AbstractNode*> evaluateChildren(const Context *ctx = NULL) const;

	const std::string &name() const { return this->modname; }
	bool isBackground() const { return this->tag_background; }
	bool isHighlight() const { return this->tag_highlight; }
	bool isRoot() const { return this->tag_root; }

	std::vector<std::string> argnames;
	std::vector<Value> argvalues;
	std::vector<class Expression*> argexpr;
	std::vector<ModuleInstantiation*> children;
	const Context *ctx;

	bool tag_root;
	bool tag_highlight;
	bool tag_background;
protected:
	std::string modname;

	friend class Module;
};

class IfElseModuleInstantiation : public ModuleInstantiation {
public:
	IfElseModuleInstantiation() : ModuleInstantiation("if") { }
	virtual ~IfElseModuleInstantiation();
	std::vector<AbstractNode*> evaluateElseChildren(const Context *ctx = NULL) const;

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
	Module() : is_handling_dependencies(false) { }
	virtual ~Module();
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;

	void addChild(ModuleInstantiation *ch) { this->children.push_back(ch); }

	typedef boost::unordered_map<std::string, class Module*> ModuleContainer;
	ModuleContainer usedlibs;
	void registerInclude(const std::string &filename);
	typedef boost::unordered_map<std::string, time_t> IncludeContainer;
	IncludeContainer includes;
	bool is_handling_dependencies;
	bool handleDependencies();

	std::vector<std::string> assignments_var;
	std::vector<Expression*> assignments_expr;

	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef boost::unordered_map<std::string, AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;

	std::vector<ModuleInstantiation*> children;

	std::vector<std::string> argnames;
	std::vector<Expression*> argexpr;

protected:

private:
};

#endif
