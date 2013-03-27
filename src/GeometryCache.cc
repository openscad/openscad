#include "GeometryCache.h"
#include "printutils.h"
#include "geometry.h"

GeometryCache *GeometryCache::inst = NULL;

void GeometryCache::insert(const std::string &id, const shared_ptr<Geometry> &ps)
{
	this->cache.insert(id, new cache_entry(ps), ps ? ps->memsize() : 0);
}

size_t GeometryCache::maxSize() const
{
	return this->cache.maxCost();
}

void GeometryCache::setMaxSize(size_t limit)
{
	this->cache.setMaxCost(limit);
}

void GeometryCache::print()
{
	PRINTB("Geometrys in cache: %d", this->cache.size());
	PRINTB("Geometry cache size in bytes: %d", this->cache.totalCost());
}

GeometryCache::cache_entry::cache_entry(const shared_ptr<Geometry> &ps) : ps(ps)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
