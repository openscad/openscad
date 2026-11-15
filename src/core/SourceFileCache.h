#pragma once

#include <ctime>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

class SourceFile;

/*!
   Caches parsed SourceFiles based on their filenames and mtimes.
 */
class SourceFileCache
{
public:
  static SourceFileCache *instance()
  {
    if (!inst) inst = new SourceFileCache;
    return inst;
  }

  std::time_t process(const fs::path& mainFile, const fs::path& filename, SourceFile *& sourceFile);
  SourceFile *lookup(const fs::path& filename);
  size_t size() const { return this->entries.size(); }
  void clear();
  static void clear_markers();

private:
  SourceFileCache() = default;

  static SourceFileCache *inst;

  struct cache_entry {
    SourceFile *file{};  // This is only valid if parsing succeeded?
    SourceFile
      *parsed_file{};  // the last version parsed for the include list, can exist even if parsing failed
    std::string cache_id;
    std::time_t mtime{};           // time file last modified
    std::time_t includes_mtime{};  // time the includes last changed
  };
  std::unordered_map<fs::path, cache_entry> entries;
};
