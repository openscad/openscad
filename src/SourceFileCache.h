#pragma once

#include <string>
#include <ctime>
#include <unordered_map>

/*!
	Caches SourceFiles based on their filenames
*/
class SourceFileCache
{
public:
	static SourceFileCache *instance() { if (!inst) inst = new SourceFileCache; return inst; }

	std::time_t evaluate(const std::string &mainFile, const std::string &filename, class SourceFile *&sourceFile);
	class SourceFile *lookup(const std::string &filename);
	size_t size() { return this->entries.size(); }
	void clear();
	static void clear_markers();

private:
	SourceFileCache() {}

	static SourceFileCache *inst;

	struct cache_entry {
		class SourceFile *file{};
		class SourceFile *parsed_file{}; // the last version parsed for the include list
		std::string cache_id;
		std::time_t mtime{};          // time file last modified
		std::time_t includes_mtime{}; // time the includes last changed
	};
	std::unordered_map<std::string, cache_entry> entries;
};
