#include "geometry/cgal/CGALCache.h"

#include <cassert>
#include <memory>
#include <cstddef>
#include <string>

#include "geometry/Geometry.h"
#include "utils/printutils.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALNefGeometry.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

CGALCache *CGALCache::inst = nullptr;

CGALCache::CGALCache(size_t limit) : cache(limit)
{
}

std::shared_ptr<const Geometry> CGALCache::get(const std::string& id) const
{
  const auto& geom = this->cache[id]->N;
#ifdef DEBUG
  LOG("CGAL Cache hit: %1$s (%2$d bytes)", id.substr(0, 40), geom ? geom->memsize() : 0);
#endif
  return geom;
}

bool CGALCache::acceptsGeometry(const std::shared_ptr<const Geometry>& geom)
{
  return 0
#ifdef ENABLE_CGAL
         || std::dynamic_pointer_cast<const CGALNefGeometry>(geom) != nullptr
#endif
#ifdef ENABLE_MANIFOLD
         || std::dynamic_pointer_cast<const ManifoldGeometry>(geom) != nullptr
#endif
    ;
}

bool CGALCache::insert(const std::string& id, const std::shared_ptr<const Geometry>& geom)
{
  assert(acceptsGeometry(geom));
  auto inserted = this->cache.insert(id, new cache_entry(geom), geom->memsize());
#ifdef DEBUG
  LOG("CGAL Cache %1$s: %2$s (%3$d bytes)", inserted ? "inserted" : "insert failed", id.substr(0, 40),
      geom->memsize());
#endif
  return inserted;
}

size_t CGALCache::size() const
{
  return cache.size();
}

size_t CGALCache::totalCost() const
{
  return cache.totalCost();
}

size_t CGALCache::maxSizeMB() const
{
  return this->cache.maxCost() / (1024ul * 1024ul);
}

void CGALCache::setMaxSizeMB(size_t limit)
{
  this->cache.setMaxCost(limit * 1024ul * 1024ul);
}

void CGALCache::clear()
{
  cache.clear();
}

void CGALCache::print()
{
  LOG("CGAL Polyhedrons in cache: %1$d", this->cache.size());
  LOG("CGAL cache size in bytes: %1$d", this->cache.totalCost());
}

CGALCache::cache_entry::cache_entry(const std::shared_ptr<const Geometry>& N) : N(N)
{
  if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
