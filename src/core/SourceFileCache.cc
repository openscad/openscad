#include "core/SourceFileCache.h"
#include "core/StatCache.h"
#include "core/SourceFile.h"
#include "utils/printutils.h"
#include "openscad.h"
#include <ctime>
#include <boost/format.hpp>

#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <algorithm>

/*!
   FIXME: Implement an LRU scheme to avoid having an ever-growing source file cache
   Only if long-running and continually `use<>`ing unique filenames.
   Will need to be returning shared_ptr<> if you do.
 */

SourceFileCache *SourceFileCache::inst = nullptr;

/*!
   Reprocess the given file and all its dependencies and reparse anything
   necessary. Updates the cache if necessary.
   The given filename must be absolute.

   Sets the given source file reference to the new file, or nullptr on any error (e.g. parse
   error or file not found).

   Returns the latest modification time of the file, its dependencies or includes.
 */
std::time_t SourceFileCache::process(const std::string& mainFile, const std::string& filename,
                                     SourceFile *& sourceFile)
{
  sourceFile = nullptr;
  auto entry = this->entries.find(filename);
  bool found{entry != this->entries.end()};
  SourceFile *file{found ? entry->second.file : nullptr};

  // Don't try to recursively process - if the file changes
  // during processing, that would be really bad.
  if (file && file->isHandlingDependencies()) return 0;

  // Create cache ID
  struct stat st;
  bool valid = (StatCache::stat(filename, st) == 0);

  // If file isn't there, just return and let the cache retain the old file
  if (!valid) return 0;

  // If the file is present, we'll always cache some result
  std::string cache_id = str(boost::format("%x.%x") % st.st_mtime % st.st_size);

  cache_entry& cacheEntry = this->entries[filename];
  // Initialize entry, if new
  if (!found) {
    cacheEntry.file = nullptr;
    cacheEntry.parsed_file = nullptr;
    cacheEntry.cache_id = cache_id;
    cacheEntry.includes_mtime = st.st_mtime;
  }
  cacheEntry.mtime = st.st_mtime;

  bool shouldParse = true;
  if (found) {
    // Files should only be reparsed if the cache ID changed
    if (cacheEntry.cache_id == cache_id) {
      shouldParse = false;
      // Reparse if includes changed
      if (cacheEntry.parsed_file) {
        std::time_t mtime = cacheEntry.parsed_file->includesChanged();
        if (mtime > cacheEntry.includes_mtime) {
          cacheEntry.includes_mtime = mtime;
          shouldParse = true;
        }
      }
    }
  }

#ifdef DEBUG
  // Causes too much debug output
  // if (!shouldParse) LOG(message_group::NONE,,"Using cached library: %1$s (%2$p)",filename,file);
#endif

  // If cache lookup failed (non-existing or old timestamp), parse file
  if (shouldParse) {
#ifdef DEBUG
    if (found) {
      PRINTDB("Recompiling cached library: %s (%s)", filename % cache_id);
    } else {
      PRINTDB("Compiling library '%s'.", filename);
    }
#endif

    std::string text;
    {
      std::ifstream ifs(filename.c_str());
      if (!ifs.is_open()) {
        LOG(message_group::Warning, "Can't open library file '%1$s'\n", filename);
        return 0;
      }
      text = STR(ifs.rdbuf(), "\n\x03\n", commandline_commands);
    }

    print_messages_push();

    delete cacheEntry.parsed_file;
    file =
      parse(cacheEntry.parsed_file, text, filename, mainFile, false) ? cacheEntry.parsed_file : nullptr;
    PRINTDB("parsed file: %s", filename);
    cacheEntry.file = file;
    cacheEntry.cache_id = cache_id;
    auto mod = file ? file : cacheEntry.parsed_file;
    if (!found && mod) cacheEntry.includes_mtime = mod->includesChanged();
    print_messages_pop();
  }

  sourceFile = file;
  // FIXME: Do we need to handle include-only cases?
  std::time_t deps_mtime = file ? file->handleDependencies(false) : 0;

  return std::max({deps_mtime, cacheEntry.mtime, cacheEntry.includes_mtime});
}

void SourceFileCache::clear() { this->entries.clear(); }

SourceFile *SourceFileCache::lookup(const std::string& filename)
{
  auto it = this->entries.find(filename);
  return it != this->entries.end() ? it->second.file : nullptr;
}

void SourceFileCache::clear_markers()
{
  for (const auto& entry : instance()->entries)
    if (auto lib = entry.second.file) lib->clearHandlingDependencies();
}
