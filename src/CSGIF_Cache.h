#pragma once

#include "cache.h"
#include "memory.h"

/*!
*/
class CSGIF_Cache
{
public:	
	CSGIF_Cache(size_t limit = 100*1024*1024);

	static CSGIF_Cache *instance() { if (!inst) inst = new CSGIF_Cache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	shared_ptr<const class CSGIF_polyhedron> get(const std::string &id) const;
	bool insert(const std::string &id, const shared_ptr<const CSGIF_polyhedron> &N);
	size_t maxSize() const;
	void setMaxSize(size_t limit);
	void clear();
	void print();

private:
	static CSGIF_Cache *inst;

	struct cache_entry {
		shared_ptr<const CSGIF_polyhedron> N;
		std::string msg;
		cache_entry(const shared_ptr<const CSGIF_polyhedron> &N);
		~cache_entry() { }
	};

	Cache<std::string, cache_entry> cache;
};
