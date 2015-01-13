#include "cgalutils.h"
#include "linalg.h"
#include "tesselator.h"

#include <boost/foreach.hpp>

static void *stdAlloc(void* userData, unsigned int size) {
	int* allocated = ( int*)userData;
	TESS_NOTUSED(userData);
	*allocated += (int)size;
	return malloc(size);
}

static void stdFree(void* userData, void* ptr) {
	TESS_NOTUSED(userData);
	free(ptr);
}

namespace CGALUtils {

bool tessellatePolygonWithHolesNew(const PolyholeK &polygons,
                                Polygons &triangles,
                                const K::Vector_3 *normal)
{
  // No polygon. FIXME: Will this ever happen or can we assert here?
  if (polygons.empty()) return false;

  TESSreal *normalvec = NULL;
  TESSreal passednormal[3];
  if (normal) {
    passednormal[0] = normal->x();
    passednormal[1] = normal->y();
    passednormal[2] = normal->z();
    normalvec = passednormal;
  }

  int allocated = 0;
  TESSalloc ma;
  TESStesselator* tess = 0;

  memset(&ma, 0, sizeof(ma));
  ma.memalloc = stdAlloc;
  ma.memfree = stdFree;
  ma.userData = (void*)&allocated;
  ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.
  
  tess = tessNewTess(&ma);
  if (!tess) return -1;

  std::vector<TESSreal> contour;
  std::vector<Vector3d> outputvertices;
  BOOST_FOREACH(const PolygonK &poly, polygons) {
    contour.clear();
    BOOST_FOREACH(const Vertex3K &v, poly) {
      outputvertices.push_back(Vector3d(v.x(), v.y(), v.z()));
      contour.push_back(v.x());
      contour.push_back(v.y());
      contour.push_back(v.z());
    }
    tessAddContour(tess, 3, &contour.front(), sizeof(TESSreal) * 3, poly.size());
  }

  if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_CONSTRAINED_DELAUNAY_TRIANGLES, 3, 3, normalvec)) return -1;
  //  printf("Memory used: %.1f kB\n", allocated/1024.0f);

  const TESSindex *vindices = tessGetVertexIndices(tess);
  const TESSindex *elements = tessGetElements(tess);
  int numelems = tessGetElementCount(tess);
  
  Polygon tri;
  for (int t=0;t<numelems;t++) {
    tri.resize(3);
    for (int i=0;i<3;i++) {
      tri[i] = outputvertices[vindices[elements[t*3 + i]]];
      //      printf("%d (%d) ", elements[t*3 + i], vindices[elements[t*3 + i]]);
    }
    //   printf("\n");
    triangles.push_back(tri);
  }

  tessDeleteTess(tess);

  return false;
}

}; // namespace CGALUtils

