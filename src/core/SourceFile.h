#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctime>
#include <vector>

#include "core/AST.h"
#include "core/LocalScope.h"
#include "core/IndicatorData.h"

class SourceFile : public ASTNode, public std::enable_shared_from_this<SourceFile>
{
  struct make_shared_enabler {
  };

public:
  SourceFile(const make_shared_enabler&, std::string path, std::string filename);
  SourceFile() = delete;

  template <typename... T>
  static std::shared_ptr<SourceFile> create(T&&...t)
  {
    return std::make_shared<SourceFile>(make_shared_enabler{}, std::forward<T>(t)...);
  }

  std::shared_ptr<AbstractNode> instantiate(
    const std::shared_ptr<const Context>& context,
    std::shared_ptr<const class FileContext> *resulting_file_context) const;
  void print(std::ostream& stream, const std::string& indent) const override;

  void setModulePath(const std::string& path) { this->path = path; }
  const std::string& modulePath() const { return this->path; }
  void registerUse(const std::string& path, const Location& loc);
  void registerInclude(const std::string& localpath, const std::string& fullpath, const Location& loc);
  std::time_t includesChanged() const;
  std::time_t handleDependencies(bool is_root = true);
  bool hasIncludes() const { return !this->includes.empty(); }
  bool usesLibraries() const { return !this->usedlibs.empty(); }
  bool isHandlingDependencies() const { return this->is_handling_dependencies; }
  void clearHandlingDependencies() { this->is_handling_dependencies = false; }
  void setFilename(const std::string& filename) { this->filename = filename; }
  const std::string& getFilename() const { return this->filename; }
  const std::string getFullpath() const;

  /**
   * @brief Get namespace scope, creating it if needed.
   */
  std::shared_ptr<LocalScope> registerNamespace(const char *name);
  /**
   * @brief If namespace exists, get its scope.
   */
  std::shared_ptr<LocalScope> getNamespaceScope(const std::string name) const;
  const std::vector<std::string>& getNamespaceNamesOrdered() { return this->namespaceNamesOrdered; }

  const std::shared_ptr<LocalScope> scope;
  std::vector<std::string> usedlibs;

  std::vector<IndicatorData> indicatorData;

private:
  std::time_t include_modified(const std::string& filename) const;

  std::unordered_map<std::string, std::string> includes;
  bool is_handling_dependencies{false};
  std::unordered_map<std::string, std::shared_ptr<LocalScope>> namespaceScopes;
  std::vector<std::string> namespaceNamesOrdered;

  std::string path;
  std::string filename;
};
