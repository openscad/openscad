#include "geometry/GeometryCache.h"
#include "utils/printutils.h"
#include "geometry/Geometry.h"

#include <memory>
#include <cstddef>
#include <string>

#ifdef ENABLE_CGAL
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#endif

GeometryCache *GeometryCache::inst = nullptr;

std::shared_ptr<const Geometry> GeometryCache::get(const std::string& id) const
{
  const auto& geom = this->cache[id]->geom;
#ifdef DEBUG
  PRINTDB("Geometry Cache hit: %s (%d bytes)", id.substr(0, 40) % (geom ? geom->memsize() : 0));
#endif
  return geom;
}

bool GeometryCache::insert(const std::string& id, const std::shared_ptr<const Geometry>& geom)
{
  auto inserted = this->cache.insert(id, new cache_entry(geom), geom ? geom->memsize() : 0);
#if defined(ENABLE_CGAL) && defined(DEBUG)
  assert(!dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get()));
  if (inserted) PRINTDB("Geometry Cache insert: %s (%d bytes)",
                        id.substr(0, 40) % (geom ? geom->memsize() : 0));
  else PRINTDB("Geometry Cache insert failed: %s (%d bytes)",
               id.substr(0, 40) % (geom ? geom->memsize() : 0));
#endif
  return inserted;
}

size_t GeometryCache::size() const
{
  return cache.size();
}

size_t GeometryCache::totalCost() const
{
  return cache.totalCost();
}

size_t GeometryCache::maxSizeMB() const
{
  return this->cache.maxCost() / (1024ul * 1024ul);
}

void GeometryCache::setMaxSizeMB(size_t limit)
{
  this->cache.setMaxCost(limit * 1024ul * 1024ul);
}

void GeometryCache::print()
{
  LOG("Geometries in cache: %1$d", this->cache.size());
  LOG("Geometry cache size in bytes: %1$d", this->cache.totalCost());
}

GeometryCache::cache_entry::cache_entry(const std::shared_ptr<const Geometry>& geom)
  : geom(geom)
{
  if (print_messages_stack.size() > 0) this->msg = print_messages_stack.back();
}
