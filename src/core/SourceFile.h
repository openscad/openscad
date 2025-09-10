#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
  SourceFile(const make_shared_enabler&, std::string path, std::string filename,
             std::shared_ptr<LocalScope> new_scope);
  SourceFile() = delete;

  template <typename... T>
  static std::shared_ptr<SourceFile> create(T&&...t)
  {
    return std::make_shared<SourceFile>(make_shared_enabler{}, std::forward<T>(t)...);
  }

  std::shared_ptr<AbstractNode> instantiate(
    const std::shared_ptr<const Context>& parent,
    std::shared_ptr<const class FileContext> *resulting_file_context) const;
  void print(std::ostream& stream, const std::string& indent) const override;

  void setModulePath(const std::string& path) { this->path = path; }
  const std::string& modulePath() const { return this->path; }
  void registerUse(const std::string& path, const Location& loc);
  void registerNamespaceUse(const std::string& ns_name, const std::string& path, const Location& loc);
  void registerInclude(const std::string& localpath, const std::string& fullpath, const Location& loc);
  std::time_t includesChanged() const;
  std::time_t handleDependencies(bool is_root = true);
  bool hasIncludes() const { return !includes.empty(); }
  bool usesLibraries() const { return !namespaceUsedLibs.empty(); }
  bool isHandlingDependencies() const { return is_handling_dependencies; }
  void clearHandlingDependencies() { this->is_handling_dependencies = false; }
  void setFilename(const std::string& filename) { this->filename = filename; }
  const std::string& getFilename() const { return filename; }
  const std::string getFullpath() const;

  /**
   * @brief Get namespace scope, creating it if needed.
   */
  std::shared_ptr<LocalScope> registerNamespace(const char *name);
  /**
   * @brief If namespace exists, get its scope.
   */
  std::shared_ptr<LocalScope> getNamespaceScope(const std::string name) const;

  /**
   * @brief Return vector of namespace names in the order they were first encountered.
   *
   * We wish to evaluate the namespaces in the order they are first declared.
   */
  const std::vector<std::string>& getNamespaceNamesOrdered() const
  {
    return this->namespaceNamesOrdered;
  }

  /**
   * @brief Return vector of `use<>`d library filenames in reverse order of last use<>
   *
   * Since we want the last use<> to take precedence, iterate from the back.
   * This only gives `use<>` inside the given namespace.
   */
  const std::vector<std::string>& getNamespaceUsedLibrariesReverseOrdered(const std::string& name) const;

  const AssignmentList& getAssignments() const { return scope->getAssignments(); }

  /**
   * Used for GUI feedback of some sort
   */
  std::vector<IndicatorData> indicatorData;

private:
  std::time_t processUseVector(std::vector<std::string>& vec);
  std::time_t include_modified(const std::string& filename) const;

  std::unordered_map<std::string, std::string> includes;
  bool is_handling_dependencies{false};

  std::unordered_map<std::string, std::vector<std::string>> namespaceUsedLibs;
  std::unordered_map<std::string, std::shared_ptr<LocalScope>> namespaceScopes;
  std::vector<std::string> namespaceNamesOrdered;

  static const std::vector<std::string>
    emptyStringVector;  // Just exists so getNamespaceUsedLibrariesReverseOrdered can be const and return
                        // reference.

  std::string path;
  std::string filename;

  std::shared_ptr<LocalScope> scope;

  friend class FileContext;  // TODO: coryrc - only for access to scope->lookup
};
