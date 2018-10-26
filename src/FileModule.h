#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <ctime>

#include "module.h"
#include "value.h"
#include "localscope.h"

class FileModule : public AbstractModule, public ASTNode
{
public:
	FileModule(const std::string &path, const std::string &filename);
	~FileModule();

	AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx = nullptr) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	AbstractNode *instantiateWithFileContext(class FileContext *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;

	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }
	void registerUse(const std::string path);
	void registerInclude(const std::string &localpath, const std::string &fullpath);
	std::time_t includesChanged() const;
	std::time_t handleDependencies();
	bool hasIncludes() const { return !this->includes.empty(); }
	bool usesLibraries() const { return !this->usedlibs.empty(); }
	bool isHandlingDependencies() const { return this->is_handling_dependencies; }
	void setFilename(const std::string &filename) { this->filename = filename; }
	const std::string &getFilename() const { return this->filename; }
	const std::string getFullpath() const;
	LocalScope scope;
	typedef std::unordered_set<std::string> ModuleContainer;
	ModuleContainer usedlibs;

private:
	struct IncludeFile {
		std::string filename;
	};

	std::time_t include_modified(const IncludeFile &inc) const;

	typedef std::unordered_map<std::string, struct IncludeFile> IncludeContainer;
	IncludeContainer includes;
	bool is_handling_dependencies;

	std::string path;
	std::string filename;
};
