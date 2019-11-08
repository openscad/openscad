#pragma once

#include "cache.h"
#include "memory.h"
#include "Geometry.h"
#include <atomic>
#include <boost/smart_ptr/detail/spinlock.hpp>

class GeometryCache
{
public:	
	GeometryCache(size_t memorylimit = 100*1024*1024) : cache(memorylimit) {
		// TODO: why is this needed? Shouldn't it start out unlocked?
		cacheLock.unlock();
	}

	static GeometryCache *instance() { if (!inst) inst = new GeometryCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	shared_ptr<const class Geometry> get(const std::string &id) const;
	bool insert(const std::string &id, const shared_ptr<const Geometry> &geom);
	size_t maxSizeMB() const;
	void setMaxSizeMB(size_t limit);
	void clear() { cache.clear(); }
	void print();

	// a mutex (spinlock) to guard access to the cache
	boost::detail::spinlock cacheLock;

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
