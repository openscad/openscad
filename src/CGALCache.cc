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
	LOG(message_group::None,Location::NONE,"","CGAL Cache hit: %1$s (%2$d bytes)",id.substr(0, 40),N ? N->memsize() : 0);
#endif
	return N;
}

bool CGALCache::insert(const std::string &id, const shared_ptr<const CGAL_Nef_polyhedron> &N)
{
	auto inserted = this->cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
#ifdef DEBUG
	if (inserted) LOG(message_group::None,Location::NONE,"","CGAL Cache insert: %1$s (%2$d bytes)",id.substr(0, 40), (N ? N->memsize() : 0));
	else LOG(message_group::None,Location::NONE,"","CGAL Cache insert failed: %1$s (%2$d bytes)",id.substr(0, 40), (N ? N->memsize() : 0));
#endif
	return inserted;
}

size_t CGALCache::maxSizeMB() const
{
	return this->cache.maxCost()/(1024*1024);
}

void CGALCache::setMaxSizeMB(size_t limit)
{
	this->cache.setMaxCost(limit*1024*1024);
}

void CGALCache::clear()
{
	cache.clear();
}

void CGALCache::print()
{
	LOG(message_group::None,Location::NONE,"","CGAL Polyhedrons in cache: %1$d",this->cache.size());
	LOG(message_group::None,Location::NONE,"","CGAL cache size in bytes: %1$d",this->cache.totalCost());
}

CGALCache::cache_entry::cache_entry(const shared_ptr<const CGAL_Nef_polyhedron> &N)
	: N(N)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
