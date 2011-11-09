#ifndef POLYSETCACHE_H_
#define POLYSETCACHE_H_

#include "cache.h"
#include "memory.h"

class PolySetCache
{
public:	
	PolySetCache(size_t polygonlimit = 100000) : cache(polygonlimit) {}

	static PolySetCache *instance() { if (!inst) inst = new PolySetCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	shared_ptr<class PolySet> get(const std::string &id) const { return this->cache[id]->ps; }
	void insert(const std::string &id, const shared_ptr<PolySet> &ps);
	void clear() { cache.clear(); }
	void print();

private:
	static PolySetCache *inst;

	struct cache_entry {
		shared_ptr<class PolySet> ps;
		std::string msg;
		cache_entry(const shared_ptr<PolySet> &ps);
		~cache_entry() { }
	};

	Cache<std::string, cache_entry> cache;
};

#endif
