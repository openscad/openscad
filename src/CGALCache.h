#ifndef CGALCACHE_H_
#define CGALCACHE_H_

#include "myqhash.h"
#include <QCache>

/*!
*/
class CGALCache
{
public:	
	CGALCache(size_t limit = 100000) : cache(limit) {}

	static CGALCache *instance() { if (!inst) inst = new CGALCache; return inst; }

	bool contains(const std::string &id) const { return this->cache.contains(id); }
	const class CGAL_Nef_polyhedron &get(const std::string &id) const { return *this->cache[id]; }
	void insert(const std::string &id, const CGAL_Nef_polyhedron &N);
	void clear() { cache.clear(); }
	void print();

private:
	static CGALCache *inst;

	QCache<std::string, CGAL_Nef_polyhedron> cache;
};

#endif
