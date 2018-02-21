#pragma once

#include "cache.h"
#include "memory.h"

/*!
*/
class CGALCache
{
public:	
	CGALCache(size_t limit = 100*1024*1024);

	static CGALCache *instance() { if (!inst) inst = new CGALCache; return inst; }

	bool contains(const size_t id) const { return this->cache.contains(id); }
	shared_ptr<const class CGAL_Nef_polyhedron> get(const size_t id) const;
	bool insert(const size_t id, const shared_ptr<const CGAL_Nef_polyhedron> &N);
	size_t maxSize() const;
	void setMaxSize(size_t limit);
	void clear();
	void print();

private:
	static CGALCache *inst;

	struct cache_entry {
		shared_ptr<const CGAL_Nef_polyhedron> N;
		std::string msg;
		cache_entry(const shared_ptr<const CGAL_Nef_polyhedron> &N);
		~cache_entry() { }
	};

	Cache<size_t, cache_entry> cache;
};
