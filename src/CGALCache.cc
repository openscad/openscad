#include "CGALCache.h"
#include "printutils.h"
#include "CGAL_Nef_polyhedron.h"

CGALCache *CGALCache::inst = nullptr;

CGALCache::CGALCache(size_t limit) : cache(limit)
{
}

shared_ptr<const CGAL_Nef_polyhedron> CGALCache::get(const std::string &id) const
{
	const auto &N = this->cache[id]->N;
#ifdef DEBUG
	PRINTB("CGAL Cache hit: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
#endif
	return N;
}

bool CGALCache::insert(const std::string &id, const shared_ptr<const CGAL_Nef_polyhedron> &N)
{
	auto inserted = this->cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
#ifdef DEBUG
	if (inserted) PRINTB("CGAL Cache insert: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
	else PRINTB("CGAL Cache insert failed: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
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

CGALCache::cache_entry::cache_entry(const shared_ptr<const CGAL_Nef_polyhedron> &N)
	: N(N)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
