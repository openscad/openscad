#include "PolySetCache.h"
#include "printutils.h"
#include "polyset.h"

PolySetCache *PolySetCache::inst = NULL;

void PolySetCache::insert(const std::string &id, const shared_ptr<PolySet> &ps)
{
	this->cache.insert(id, new cache_entry(ps), ps ? ps->memsize() : 0);
}

size_t PolySetCache::maxSize() const
{
	return this->cache.maxCost();
}

void PolySetCache::setMaxSize(size_t limit)
{
	this->cache.setMaxCost(limit);
}

void PolySetCache::print()
{
	PRINTB("PolySets in cache: %d", this->cache.size());
	PRINTB("PolySet cache size in bytes: %d", this->cache.totalCost());
}

PolySetCache::cache_entry::cache_entry(const shared_ptr<PolySet> &ps) : ps(ps)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
