#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <ctime>

#include "module.h"
#include "value.h"
#include "localscope.h"
#include "indicatordata.h"

class FileModule : public AbstractModule, public ASTNode
{
public:
	FileModule(const std::string &path, const std::string &filename);
	~FileModule();

	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	AbstractNode *instantiateWithFileContext(const std::shared_ptr<class FileContext>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const;

	void setModulePath(const std::string &path) { this->path = path; }
	const std::string &modulePath() const { return this->path; }
	void addExternalNode(const std::shared_ptr<ExternalNode> &node);
	void resolveUseNodes();
	void resolveIncludeNodes();
	time_t includesChanged() const;
	std::time_t handleDependencies(bool is_root = true);
	bool hasExternals() const { return !this->externalDict.empty(); }
	bool isHandlingDependencies() const { return this->is_handling_dependencies; }
	void resolveExternals();
	std::vector<shared_ptr<UseNode>> getUseNodes() const;

	void clearHandlingDependencies() { this->is_handling_dependencies = false; }
	void setFilename(const std::string &filename) { this->filename = filename; }
	const std::string &getFilename() const { return this->filename; }
	const std::string getFullpath() const;
	void resolveAssignments();

	LocalScope scope;
	std::unordered_map<std::string, shared_ptr<ExternalNode>> externalDict;
	std::vector<shared_ptr<ExternalNode>> externalList;

	std::vector<IndicatorData> indicatorData;
private:
	struct IncludeFile {
		std::string filename;
	};

	std::time_t includeModified(const IncludeNode &node) const;

	bool is_handling_dependencies;

	std::string path;
	std::string filename;
};
