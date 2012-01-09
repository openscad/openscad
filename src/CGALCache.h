#ifndef CGALCACHE_H_
#define CGALCACHE_H_

#include "cache.h"

/*!
*/
class CGALCache
{
public:	
	CGALCache(size_t limit = 100*1024*1024);

	static CGALCache *instance() { if (!inst) inst = new CGALCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	const class CGAL_Nef_polyhedron &get(const std::string &id) const;
	bool insert(const std::string &id, const CGAL_Nef_polyhedron &N);
	size_t maxSize() const;
	void setMaxSize(size_t limit);
	void clear();
	void print();

private:
	static CGALCache *inst;

	Cache<std::string, CGAL_Nef_polyhedron> cache;
};

#endif
