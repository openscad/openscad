#pragma once

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <time.h>
#include <sys/stat.h>

#include "AST.h"
#include "value.h"
#include "localscope.h"
#include "feature.h"

class AbstractModule : public ASTNode
{
private:
        const Feature *feature;
public:
        AbstractModule() : feature(NULL) {}
        AbstractModule(const Feature& feature) : feature(&feature) {}
	virtual ~AbstractModule();
        virtual bool is_experimental() const { return feature != NULL; }
        virtual bool is_enabled() const { return (feature == NULL) || feature->is_enabled(); }
	virtual class AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, class EvalContext *evalctx = NULL) const = 0;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
        virtual double lookup_double_variable_with_default(Context &c, std::string variable, double def) const;
        virtual std::string lookup_string_variable_with_default(Context &c, std::string variable, std::string def) const;
};

class GroupModule : public AbstractModule
{
public:
	GroupModule() { }
	virtual ~GroupModule() { }
	virtual class AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, class EvalContext *evalctx = NULL) const;
};

class UserModule : public AbstractModule
{
public:
	UserModule() { }
	UserModule(const Feature& feature) : AbstractModule(feature) { }
	virtual ~UserModule() {}

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
	static const std::string& stack_element(int n) { return module_stack[n]; };
	static int stack_size() { return module_stack.size(); };

	AssignmentList definition_arguments;

	LocalScope scope;

private:
	static std::deque<std::string> module_stack;
};

class FileModule : public AbstractModule
{
public:
	FileModule() : context(nullptr), is_handling_dependencies(false) {}
	virtual ~FileModule();

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = NULL) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }
        void registerUse(const std::string path);
	void registerInclude(const std::string &localpath, const std::string &fullpath);
	bool includesChanged() const;
	bool handleDependencies();
	bool hasIncludes() const { return !this->includes.empty(); }
	bool usesLibraries() const { return !this->usedlibs.empty(); }
	bool isHandlingDependencies() const { return this->is_handling_dependencies; }
	ValuePtr lookup_variable(const std::string &name) const;

	LocalScope scope;
	typedef std::unordered_set<std::string> ModuleContainer;
	ModuleContainer usedlibs;
private:
	// Reference to retain the context that was used in the last evaluation
	mutable class FileContext *context;
	struct IncludeFile {
		std::string filename;
		bool valid;
		time_t mtime;
	};

	bool include_modified(const IncludeFile &inc) const;

	typedef std::unordered_map<std::string, struct IncludeFile> IncludeContainer;
	IncludeContainer includes;
	bool is_handling_dependencies;
	std::string path;
};
