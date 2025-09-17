#pragma once

#include <string>
#include <ctime>
#include <unordered_map>

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

  std::time_t process(const std::string& mainFile, const std::string& filename,
                      SourceFile *& sourceFile);
  SourceFile *lookup(const std::string& filename);
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
  std::unordered_map<std::string, cache_entry> entries;
};
