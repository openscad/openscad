#pragma once

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <time.h>
#include <sys/stat.h>

#include "value.h"
#include "typedefs.h"
#include "localscope.h"
#include "feature.h"

class ModuleInstantiation
{
public:
	ModuleInstantiation(const std::string &name = "")
		: tag_root(false), tag_highlight(false), tag_background(false), modname(name) { }
	virtual ~ModuleInstantiation();

	virtual std::string dump(const std::string &indent) const;
	class AbstractNode *evaluate(const class Context *ctx) const;
	std::vector<AbstractNode*> instantiateChildren(const Context *evalctx) const;

	void setPath(const std::string &path) { this->modpath = path; }
	const std::string &path() const { return this->modpath; }
	std::string getAbsolutePath(const std::string &filename) const;

	const std::string &name() const { return this->modname; }
	bool isBackground() const { return this->tag_background; }
	bool isHighlight() const { return this->tag_highlight; }
	bool isRoot() const { return this->tag_root; }

	AssignmentList arguments;
	LocalScope scope;

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
	std::vector<AbstractNode*> instantiateElseChildren(const Context *evalctx) const;
	virtual std::string dump(const std::string &indent) const;

	LocalScope else_scope;
};

class AbstractModule
{
private:
        const Feature *feature;
public:
        AbstractModule() : feature(NULL) {}
        AbstractModule(const Feature& feature) : feature(&feature) {}
	virtual ~AbstractModule();
        virtual bool is_experimental() const { return feature != NULL; }
        virtual bool is_enabled() const { return (feature == NULL) || feature->is_enabled(); }
	virtual class AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, class EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
        virtual double lookup_double_variable_with_default(Context &c, std::string variable, double def) const;
        virtual std::string lookup_string_variable_with_default(Context &c, std::string variable, std::string def) const;
};

class Module : public AbstractModule
{
public:
	Module() { }
	Module(const Feature& feature) : AbstractModule(feature) { }
	virtual ~Module();

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
	static const std::string& stack_element(int n) { return module_stack[n]; };
	static int stack_size() { return module_stack.size(); };

	AssignmentList definition_arguments;

	LocalScope scope;

private:
	static std::deque<std::string> module_stack;
};

// FIXME: A FileModule doesn't have definition arguments, so we shouldn't really
// inherit from a Module
class FileModule : public Module
{
public:
	FileModule() : context(NULL), is_handling_dependencies(false) {}
	virtual ~FileModule();

	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }
        void registerUse(const std::string path);
	void registerInclude(const std::string &localpath, const std::string &fullpath);
	bool includesChanged() const;
	bool handleDependencies();
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = NULL);
	bool hasIncludes() const { return !this->includes.empty(); }
	bool usesLibraries() const { return !this->usedlibs.empty(); }
	bool isHandlingDependencies() const { return this->is_handling_dependencies; }
        ValuePtr lookup_variable(const std::string &name) const;

	typedef boost::unordered_set<std::string> ModuleContainer;
	ModuleContainer usedlibs;
private:
        /** Reference to retain the context that was used in the last evaluation */
        class FileContext *context;
	struct IncludeFile {
		std::string filename;
		bool valid;
		time_t mtime;
	};

	bool include_modified(const IncludeFile &inc) const;

	typedef boost::unordered_map<std::string, struct IncludeFile> IncludeContainer;
	IncludeContainer includes;
	bool is_handling_dependencies;
	std::string path;
};
