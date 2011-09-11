#include "PolySetCache.h"
#include "printutils.h"
#include "PolySet.h"

PolySetCache *PolySetCache::inst = NULL;

void PolySetCache::insert(const std::string &id, const shared_ptr<PolySet> &ps)
{
	this->cache.insert(id, new cache_entry(ps), ps ? ps->polygons.size() : 0);
}

void PolySetCache::print()
{
	PRINTF("PolySets in cache: %d", this->cache.size());
	PRINTF("Polygons in cache: %d", this->cache.totalCost());
}

PolySetCache::cache_entry::cache_entry(const shared_ptr<PolySet> &ps) : ps(ps)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.last();
}
