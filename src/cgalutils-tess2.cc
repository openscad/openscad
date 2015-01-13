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

	if (polygons.size() == 1 && polygons[0].size() == 3) {
		// Input polygon has 3 points. shortcut tessellation.
		Polygon t;
		t.push_back(Vector3d(polygons[0][0].x(), polygons[0][0].y(), polygons[0][0].z()));
		t.push_back(Vector3d(polygons[0][1].x(), polygons[0][1].y(), polygons[0][1].z()));
		t.push_back(Vector3d(polygons[0][2].x(), polygons[0][2].y(), polygons[0][2].z()));
		triangles.push_back(t);
		return false;
	}

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
			TESSreal tessv[3] = {v.x(), v.y(), v.z()};
      outputvertices.push_back(Vector3d(tessv[0], tessv[1], tessv[2]));
      contour.push_back(tessv[0]);
      contour.push_back(tessv[1]);
      contour.push_back(tessv[2]);
    }
    tessAddContour(tess, 3, &contour.front(), sizeof(TESSreal) * 3, poly.size());
  }

  if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_CONSTRAINED_DELAUNAY_TRIANGLES, 3, 3, normalvec)) return -1;

  const TESSindex *vindices = tessGetVertexIndices(tess);
  const TESSindex *elements = tessGetElements(tess);
  int numelems = tessGetElementCount(tess);
  
  Polygon tri;
  for (int t=0;t<numelems;t++) {
    tri.resize(3);
    bool err = false;
    for (int i=0;i<3;i++) {
      int vidx = vindices[elements[t*3 + i]];
      if (vidx == TESS_UNDEF) err = true;
      else tri[i] = outputvertices[vidx];
    }
		// FIXME: We ignore self-intersecting triangles rather than detecting and handling this
    if (!err) triangles.push_back(tri);
		else PRINT("WARNING: Self-intersecting polygon encountered - ignoring");
  }

  tessDeleteTess(tess);

  return false;
}

}; // namespace CGALUtils

