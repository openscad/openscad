#include "GeometryCache.h"
#include "printutils.h"
#include "Geometry.h"
#ifdef DEBUG
  #ifndef ENABLE_CGAL
  #define ENABLE_CGAL
  #endif
  #include "CGAL_Nef_polyhedron.h"
#endif

GeometryCache *GeometryCache::inst = nullptr;

shared_ptr<const Geometry> GeometryCache::get(const size_t id) const
{
	const auto &geom = this->cache[id]->geom;
#ifdef DEBUG
	PRINTDB("Geometry Cache hit: %zx (%d bytes)", id % (geom ? geom->memsize() : 0));
#endif
	return geom;
}

bool GeometryCache::insert(const size_t id, const shared_ptr<const Geometry> &geom)
{
	auto inserted = this->cache.insert(id, new cache_entry(geom), geom ? geom->memsize() : 0);
#ifdef DEBUG
	assert(!dynamic_cast<const CGAL_Nef_polyhedron*>(geom.get()));
	if (inserted) PRINTDB("Geometry Cache insert: %zx (%d bytes)", 
                         id % (geom ? geom->memsize() : 0));
	else PRINTDB("Geometry Cache insert failed: %zx (%d bytes)",
                id % (geom ? geom->memsize() : 0));
#endif
	return inserted;
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
	PRINTB("Geometries in cache: %d", this->cache.size());
	PRINTB("Geometry cache size in bytes: %d", this->cache.totalCost());
}

GeometryCache::cache_entry::cache_entry(const shared_ptr<const Geometry> &geom)
	: geom(geom)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
