#pragma once

#include "cache.h"
#include "memory.h"
#include "Geometry.h"

class GeometryCache
{
public:
	GeometryCache(size_t memorylimit = 100 *1024 *1024) : cache(memorylimit) {}

	static GeometryCache *instance() { if (!inst) inst = new GeometryCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	shared_ptr<const class Geometry> get(const std::string &id) const;
	bool insert(const std::string &id, const shared_ptr<const Geometry> &geom);
	size_t maxSize() const;
	void setMaxSize(size_t limit);
	void clear() { cache.clear(); }
	void print();

private:
	static GeometryCache *inst;

	struct cache_entry {
		shared_ptr<const class Geometry> geom;
		std::string msg;
		cache_entry(const shared_ptr<const Geometry> &geom);
		~cache_entry() { }
	};

	Cache<std::string, cache_entry> cache;
};
