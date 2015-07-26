#include "CSGIF_Cache.h"
#include "printutils.h"

#include "CSGIF.h"

CSGIF_Cache *CSGIF_Cache::inst = NULL;

CSGIF_Cache::CSGIF_Cache(size_t limit) : cache(limit)
{
}

shared_ptr<const CSGIF_polyhedron> CSGIF_Cache::get(const std::string &id) const
{
	const shared_ptr<const CSGIF_polyhedron> &N = this->cache[id]->N;
#ifdef DEBUG
	PRINTB("CSG Cache hit: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
#endif
	return N;
}

bool CSGIF_Cache::insert(const std::string &id, const shared_ptr<const CSGIF_polyhedron> &N)
{
	bool inserted = this->cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
#ifdef DEBUG
	if (inserted) PRINTB("CSG Cache insert: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
	else PRINTB("CSG Cache insert failed: %s (%d bytes)", id.substr(0, 40) % (N ? N->memsize() : 0));
#endif
	return inserted;
}

size_t CSGIF_Cache::maxSize() const
{
	return this->cache.maxCost();
}

void CSGIF_Cache::setMaxSize(size_t limit)
{
	this->cache.setMaxCost(limit);
}

void CSGIF_Cache::clear()
{
	cache.clear();
}

void CSGIF_Cache::print()
{
	PRINTB("CSG Polyhedrons in cache: %d", this->cache.size());
	PRINTB("CSG cache size in bytes: %d", this->cache.totalCost());
}

CSGIF_Cache::cache_entry::cache_entry(const shared_ptr<const CSGIF_polyhedron> &N)
	: N(N)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
