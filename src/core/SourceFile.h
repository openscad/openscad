#pragma once

#include <ctime>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include "core/AST.h"
#include "core/IndicatorData.h"
#include "core/LocalScope.h"

namespace fs = std::filesystem;

class SourceFile : public ASTNode
{
public:
  SourceFile(fs::path path, fs::path filename);

  std::shared_ptr<AbstractNode> instantiate(
    const std::shared_ptr<const Context>& context,
    std::shared_ptr<const class FileContext> *resulting_file_context) const;
  void print(std::ostream& stream, const std::string& indent) const override;

  void setModulePath(const fs::path& path) { this->path = path; }
  const fs::path& modulePath() const { return this->path; }
  void registerUse(const fs::path& path, const Location& loc);
  void registerInclude(const fs::path& localpath, const fs::path& fullpath, const Location& loc);
  std::time_t includesChanged() const;
  std::time_t handleDependencies(bool is_root = true);
  bool hasIncludes() const { return !this->includes.empty(); }
  bool usesLibraries() const { return !this->usedlibs.empty(); }
  bool isHandlingDependencies() const { return this->is_handling_dependencies; }
  void clearHandlingDependencies() { this->is_handling_dependencies = false; }
  void setFilename(const fs::path& filename) { this->filename = filename; }
  const fs::path& getFilename() const { return this->filename; }
  const fs::path getFullpath() const;

  const std::shared_ptr<LocalScope> scope;
  std::vector<fs::path> usedlibs;

  std::vector<IndicatorData> indicatorData;

private:
  std::time_t include_modified(const fs::path& filename) const;

  std::unordered_map<fs::path, fs::path> includes;
  bool is_handling_dependencies{false};

  fs::path path;
  fs::path filename;
};
