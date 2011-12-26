#include "CGALCache.h"
#include "printutils.h"
#include "CGAL_Nef_polyhedron.h"

CGALCache *CGALCache::inst = NULL;

void CGALCache::insert(const std::string &id, const CGAL_Nef_polyhedron &N)
{
	this->cache.insert(id, new CGAL_Nef_polyhedron(N), N.weight());
#ifdef DEBUG
	PRINTF("CGAL Cache insert: %s (%d verts)", id.substr(0, 40).c_str(), N.weight());
#endif
}

void CGALCache::print()
{
	PRINTF("CGAL Polyhedrons in cache: %d", this->cache.size());
	PRINTF("Vertices in cache: %d", this->cache.totalCost());
}
