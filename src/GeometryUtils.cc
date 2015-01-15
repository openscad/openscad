#include "GeometryUtils.h"
#include "tesselator.h"
#include "printutils.h"
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

/*!
	Tessellates input contours into a triangle mesh.

	The input contours may consist of positive (CCW) and negative (CW)
	contours. These define holes, and possibly islands.

	The output will be written as indices into the input vertex vector.

	This function should be robust wrt. malformed input.

	It will only use existing vertices and is guaranteed use all
	existing vertices, i.e. it will maintain connectivity if the input
	polygon is part of a polygon mesh.

	Returns true on error, false on success.
*/
bool GeometryUtils::tessellatePolygonWithHoles(const IndexedPolygons &polygons, 
																							 std::vector<IndexedTriangle> &triangles,
																							 const Vector3f *normal)
{
  // No polygon. FIXME: Will this ever happen or can we assert here?
  if (polygons.faces.empty()) return false;

	if (polygons.faces.size() == 1 && polygons.faces[0].size() == 3) {
		// Input polygon has 3 points. shortcut tessellation.
		triangles.push_back(IndexedTriangle(polygons.faces[0][0], polygons.faces[0][1], polygons.faces[0][2]));
		return false;
	}

  TESSreal *normalvec = NULL;
  TESSreal passednormal[3];
  if (normal) {
    passednormal[0] = (*normal)[0];
		passednormal[1] = (*normal)[1];
		passednormal[2] = (*normal)[2];
    normalvec = passednormal;
  }

  TESSalloc ma;
  TESStesselator* tess = 0;

  memset(&ma, 0, sizeof(ma));
  ma.memalloc = stdAlloc;
  ma.memfree = stdFree;
  ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.
  
  if (!(tess = tessNewTess(&ma))) return true;

	int numContours = 0;
  std::vector<TESSreal> contour;
  BOOST_FOREACH(const IndexedFace &face, polygons.faces) {
		const Vector3f *verts = &polygons.vertices.front();
    contour.clear();
    BOOST_FOREACH(int idx, face) {
			const Vector3f &v = verts[idx];
      contour.push_back(v[0]);
      contour.push_back(v[1]);
      contour.push_back(v[2]);
    }
		assert(face.size() >= 3);
		PRINTDB("Contour: %d\n", face.size());
    tessAddContour(tess, 3, &contour.front(), sizeof(TESSreal) * 3, face.size());
		numContours++;
  }

  if (!tessTesselate(tess, TESS_WINDING_ODD, TESS_CONSTRAINED_DELAUNAY_TRIANGLES, 3, 3, normalvec)) return -1;

  const TESSindex *vindices = tessGetVertexIndices(tess);
  const TESSindex *elements = tessGetElements(tess);
  int numelems = tessGetElementCount(tess);
  
	/*
		At this point, we have a delaunay triangle mesh.

		However, as libtess2 might merge vertices, as well as insert new
		vertices for intersecting edges, we need to detect these and
		insert dummy triangles to maintain external connectivity.

		FIXME: This currently only works for polygons without holes.
	 */
	if (polygons.faces.size() == 1) { // Only works for polygons without holes

		/*
			Algorithm:
			A) Collect all triangles using _only_ existing vertices -> triangles
			B) Locate all unused vertices
			C) For each unused vertex, create a triangle connecting it to the existing mesh
		*/
		int numInputVerts = polygons.faces[0].size();
		std::vector<int> vflags(numInputVerts); // Init 0
		
		IndexedTriangle tri;
		for (int t=0;t<numelems;t++) {
			bool err = false;
			for (int i=0;i<3;i++) {
				int vidx = vindices[elements[t*3 + i]];
				if (vidx == TESS_UNDEF) err = true;
				else tri[i] = vidx; // A)
			}
			PRINTDB("%d (%d) %d (%d) %d (%d)",
							elements[t*3 + 0] % vindices[elements[t*3 + 0]] %
							elements[t*3 + 1] % vindices[elements[t*3 + 1]] %
							elements[t*3 + 2] % vindices[elements[t*3 + 2]]);
			// FIXME: We ignore self-intersecting triangles rather than detecting and handling this
			if (!err) {
				triangles.push_back(tri);
				vflags[tri[0]] = 1; // B)
				vflags[tri[1]] = 1;
				vflags[tri[2]] = 1;
			}
		}

		for (int i=0;i<numInputVerts;i++) {
			if (vflags[i] == 0) { // vertex missing in output: C)
				int startv = (i+numInputVerts-1)%numInputVerts;
				int j;
				for (j = i; j < numInputVerts && vflags[j] == 0; j++) {
					// Create triangle fan from vertex i-1 to the first existing vertex
					PRINTDB("(%d) (%d) (%d)\n", startv % j % (j+1)%numInputVerts);
					tri[0] = startv;
					tri[1] = j;
					tri[2] = (j+1)%numInputVerts;
					triangles.push_back(tri);
				}
				i = j;
			}
		}
	}
	else {
		IndexedTriangle tri;
		for (int t=0;t<numelems;t++) {
			bool err = false;
			for (int i=0;i<3;i++) {
				int vidx = vindices[elements[t*3 + i]];
				if (vidx == TESS_UNDEF) err = true;
				else tri[i] = vidx;
			}
			PRINTDB("%d (%d) %d (%d) %d (%d)",
							elements[t*3 + 0] % vindices[elements[t*3 + 0]] %
							elements[t*3 + 1] % vindices[elements[t*3 + 1]] %
							elements[t*3 + 2] % vindices[elements[t*3 + 2]]);
			// FIXME: We ignore self-intersecting triangles rather than detecting and handling this
			if (!err) {
				triangles.push_back(tri);
			}
			else PRINT("WARNING: Self-intersecting polygon encountered - ignoring");
		}
	}

  tessDeleteTess(tess);

  return false;
}
