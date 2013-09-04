#include <string>
#include <boost/unordered_map.hpp>

/*!
	Caches FileModules based on their filenames
*/
class ModuleCache
{
public:
	static ModuleCache *instance() { if (!inst) inst = new ModuleCache; return inst; }
	class FileModule *evaluate(const std::string &filename);
	class FileModule *lookup(const std::string &filename);
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
	boost::unordered_map<std::string, cache_entry> entries;
};
