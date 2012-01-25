#include "CGALCache.h"
#include "printutils.h"
#include "CGAL_Nef_polyhedron.h"

CGALCache *CGALCache::inst = NULL;

CGALCache::CGALCache(size_t limit) : cache(limit)
{
}

const CGAL_Nef_polyhedron &CGALCache::get(const std::string &id) const
{
	const CGAL_Nef_polyhedron &N = *this->cache[id];
#ifdef DEBUG
	PRINTB("CGAL Cache hit: %s (%d bytes)", id.substr(0, 40) % N.weight());
#endif
	return N;
}

bool CGALCache::insert(const std::string &id, const CGAL_Nef_polyhedron &N)
{
	bool inserted = this->cache.insert(id, new CGAL_Nef_polyhedron(N), N.weight());
#ifdef DEBUG
	if (inserted) PRINTB("CGAL Cache insert: %s (%d bytes)", id.substr(0, 40) % N.weight());
	else PRINTB("CGAL Cache insert failed: %s (%d bytes)", id.substr(0, 40) % N.weight());
#endif
	return inserted;
}

size_t CGALCache::maxSize() const
{
	return this->cache.maxCost();
}

void CGALCache::setMaxSize(size_t limit)
{
	this->cache.setMaxCost(limit);
}

void CGALCache::clear()
{
	cache.clear();
}

void CGALCache::print()
{
	PRINTB("CGAL Polyhedrons in cache: %d", this->cache.size());
	PRINTB("CGAL cache size in bytes: %d", this->cache.totalCost());
}
