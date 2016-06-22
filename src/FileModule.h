#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <time.h>

#include "module.h"
#include "value.h"
#include "localscope.h"

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
