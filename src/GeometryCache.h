#ifndef GEOMETRYCACHE_H_
#define GEOMETRYCACHE_H_

#include "cache.h"
#include "memory.h"
#include "Geometry.h"

class GeometryCache
{
public:	
	GeometryCache(size_t memorylimit = 100*1024*1024) : cache(memorylimit) {}

	static GeometryCache *instance() { if (!inst) inst = new GeometryCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	shared_ptr<class Geometry> get(const std::string &id) const { return this->cache[id]->ps; }
	void insert(const std::string &id, const shared_ptr<Geometry> &ps);
	size_t maxSize() const;
	void setMaxSize(size_t limit);
	void clear() { cache.clear(); }
	void print();

private:
	static GeometryCache *inst;

	struct cache_entry {
		shared_ptr<class Geometry> ps;
		std::string msg;
		cache_entry(const shared_ptr<Geometry> &ps);
		~cache_entry() { }
	};

	Cache<std::string, cache_entry> cache;
};

#endif
