#ifndef MODULE_H_
#define MODULE_H_

#include <string>
#include <vector>
#include <list>
#include <boost/unordered_map.hpp>
#include "value.h"
#include "typedefs.h"

class ModuleInstantiation
{
public:
	ModuleInstantiation(const std::string &name = "")
	: tag_root(false), tag_highlight(false), tag_background(false), modname(name) { }
	virtual ~ModuleInstantiation();

	std::string dump(const std::string &indent) const;
	class AbstractNode *evaluate_instance(const class Context *ctx) const;
	std::vector<AbstractNode*> evaluateChildren(const Context *evalctx) const;

	void setPath(const std::string &path) { this->modpath = path; }
	const std::string &path() const { return this->modpath; }
	std::string getAbsolutePath(const std::string &filename) const;

	const std::string &name() const { return this->modname; }
	bool isBackground() const { return this->tag_background; }
	bool isHighlight() const { return this->tag_highlight; }
	bool isRoot() const { return this->tag_root; }

	AssignmentList arguments;
	std::vector<ModuleInstantiation*> children;

	bool tag_root;
	bool tag_highlight;
	bool tag_background;
protected:
	std::string modname;
	std::string modpath;

	friend class Module;
};

class IfElseModuleInstantiation : public ModuleInstantiation {
public:
	IfElseModuleInstantiation() : ModuleInstantiation("if") { }
	virtual ~IfElseModuleInstantiation();
	std::vector<AbstractNode*> evaluateElseChildren(const Context *evalctx) const;

	std::vector<ModuleInstantiation*> else_children;
};

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual class AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst, const class EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Module : public AbstractModule
{
public:
	Module() : is_handling_dependencies(false) { }
	virtual ~Module();

	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }

	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;

	void addChild(ModuleInstantiation *ch) { this->children.push_back(ch); }

	typedef boost::unordered_map<std::string, class Module*> ModuleContainer;
	ModuleContainer usedlibs;
	void registerInclude(const std::string &filename);
	typedef boost::unordered_map<std::string, time_t> IncludeContainer;
	IncludeContainer includes;
	bool is_handling_dependencies;
	bool handleDependencies();

	std::list<std::string> assignments_var;
	typedef boost::unordered_map<std::string, Expression*> AssignmentMap;
	AssignmentMap assignments;

	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef boost::unordered_map<std::string, AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;

	std::vector<ModuleInstantiation*> children;

	std::vector<Assignment> definition_arguments;

protected:

private:
	std::string path;
};

#endif
