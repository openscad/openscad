#include "geometry/cgal/CGALCache.h"

#include <cassert>
#include <memory>
#include <cstddef>
#include <string>

#include "utils/printutils.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

CGALCache *CGALCache::inst = nullptr;

CGALCache::CGALCache(size_t limit) : cache(limit)
{
}

std::shared_ptr<const Geometry> CGALCache::get(const std::string& id) const
{
  const auto& N = this->cache[id]->N;
#ifdef DEBUG
  LOG("CGAL Cache hit: %1$s (%2$d bytes)", id.substr(0, 40), N ? N->memsize() : 0);
#endif
  return N;
}

bool CGALCache::acceptsGeometry(const std::shared_ptr<const Geometry>& geom) {
  return
    std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom) ||
    std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)
#ifdef ENABLE_MANIFOLD
    || std::dynamic_pointer_cast<const ManifoldGeometry>(geom)
#endif
    ;
}

bool CGALCache::insert(const std::string& id, const std::shared_ptr<const Geometry>& N)
{
  assert(acceptsGeometry(N));
  auto inserted = this->cache.insert(id, new cache_entry(N), N ? N->memsize() : 0);
#ifdef DEBUG
  if (inserted) LOG("CGAL Cache insert: %1$s (%2$d bytes)", id.substr(0, 40), (N ? N->memsize() : 0));
  else LOG("CGAL Cache insert failed: %1$s (%2$d bytes)", id.substr(0, 40), (N ? N->memsize() : 0));
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

CGALCache::cache_entry::cache_entry(const std::shared_ptr<const Geometry>& N)
  : N(N)
{
  if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
