#pragma once

#include <string>
#include <ctime>
#include <unordered_map>

/*!
	Caches FileModules based on their filenames
*/
class ModuleCache
{
public:
	static ModuleCache *instance() { if (!inst) inst = new ModuleCache; return inst; }

	std::time_t evaluate(const std::string &mainFile, const std::string &filename, class FileModule *&module);
	class FileModule *lookup(const std::string &filename);
	bool isCached(const std::string &filename);
	size_t size() { return this->entries.size(); }
	void clear();

private:
	ModuleCache() {}
	~ModuleCache() {}

	static ModuleCache *inst;

	struct cache_entry {
		class FileModule *module{};
		class FileModule *parsed_module{}; // the last version parsed for the include list
		std::string cache_id;
		std::time_t mtime{};          // time file last modified
		std::time_t includes_mtime{}; // time the includes last changed
	};
	std::unordered_map<std::string, cache_entry> entries;
};
