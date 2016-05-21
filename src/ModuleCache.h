#pragma once

#include <string>
#include <unordered_map>

/*!
	Caches FileModules based on their filenames
*/
class ModuleCache
{
public:
	static ModuleCache *instance() { if (!inst) inst = new ModuleCache; return inst; }
	bool evaluate(const std::string &filename, class FileModule *&module);
	class FileModule *lookup(const std::string &filename);
	bool isCached(const std::string &filename);
	size_t size() { return this->entries.size(); }
	void clear();

private:
	ModuleCache() {}
	~ModuleCache() {}

	static ModuleCache *inst;

	struct cache_entry {
		class FileModule *module;
		std::string cache_id;
	};
	std::unordered_map<std::string, cache_entry> entries;
};
