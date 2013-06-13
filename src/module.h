#ifndef MODULE_H_
#define MODULE_H_

#include <string>
#include <vector>
#include <list>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <time.h>
#include <sys/stat.h>

#include "value.h"
#include "typedefs.h"
#include "localscope.h"

class ModuleInstantiation
{
public:
	ModuleInstantiation(const std::string &name = "")
		: tag_root(false), tag_highlight(false), tag_background(false), recursioncount(0), modname(name) { }
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
	mutable int recursioncount;
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
public:
	virtual ~AbstractModule();
	virtual class AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const class EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Module : public AbstractModule
{
public:
	Module() { }
	virtual ~Module();

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;

	AssignmentList definition_arguments;

	LocalScope scope;
};

// FIXME: A FileModule doesn't have definition arguments, so we shouldn't really
// inherit from a Module
class FileModule : public Module
{
public:
	FileModule() : is_handling_dependencies(false) {}
	virtual ~FileModule() {}

	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }
	void registerInclude(const std::string &localpath, const std::string &fullpath);
	bool includesChanged() const;
	bool handleDependencies();
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx = NULL) const;
	bool hasIncludes() const { return !this->includes.empty(); }
	bool usesLibraries() const { return !this->usedlibs.empty(); }
	bool isHandlingDependencies() const { return this->is_handling_dependencies; }

	typedef boost::unordered_set<std::string> ModuleContainer;
	ModuleContainer usedlibs;
private:
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

#endif
