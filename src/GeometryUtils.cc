#include "GeometryUtils.h"
#include "tesselator.h"
#include "printutils.h"
#include "Reindexer.h"
#include "grid.h"
#include <boost/foreach.hpp>

static void *stdAlloc(void* userData, unsigned int size) {
	TESS_NOTUSED(userData);
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

	It will only use existing vertices and is guaranteed to maintain
	connectivity if the input polygon is part of a polygon mesh.

	One requirement: The input vertices must be distinct
	(i.e. duplicated must resolve to the same index).

	Returns true on error, false on success.
*/
bool GeometryUtils::tessellatePolygonWithHoles(const IndexedPolygons &polygons, 
																							 std::vector<IndexedTriangle> &triangles,
																							 const Vector3f *normal)
{
  // No polygon. FIXME: Will this ever happen or can we assert here?
  if (polygons.faces.empty()) return false;

	// Remove consecutive equal vertices, as well as null ears
	std::vector<IndexedFace> faces = polygons.faces;
  BOOST_FOREACH(IndexedFace &face, faces) {
		int i=0;
		while (i < face.size()) {
			if (face[i] == face[(i+1)%face.size()]) { // Two consecutively equal indices
				face.erase(face.begin()+i);
			}
			else if (face[i] == face[(i+2)%face.size()]) { // Null ear
				face.erase(face.begin() + (i+1)%face.size());
			}
			else {
				i++;
			}
		}
	}
	// First polygon has < 3 points - no output
	if (faces[0].size() < 3) return false;
	// Remove collapsed holes
	for (int i=1;i<faces.size();i++) {
		if (faces[i].size() < 3) {
			faces.erase(faces.begin() + i);
			i--;
		}
	}

	if (faces.size() == 1 && faces[0].size() == 3) {
		// Input polygon has 3 points. shortcut tessellation.
		triangles.push_back(IndexedTriangle(faces[0][0], faces[0][1], faces[0][2]));
		return false;
	}

	const Vector3f *verts = &polygons.vertices.front();
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
	// Since libtess2's indices is based on the running number of points added, we need to map back
	// to our indices. allindices does the mapping.
	std::vector<int> allindices;
  BOOST_FOREACH(const IndexedFace &face, faces) {
    contour.clear();
    BOOST_FOREACH(int idx, face) {
			const Vector3f &v = verts[idx];
      contour.push_back(v[0]);
      contour.push_back(v[1]);
      contour.push_back(v[2]);
			allindices.push_back(idx);
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
	if (faces.size() == 1) { // Only works for polygons without holes

		/*
			Algorithm:
			A) Collect all triangles using _only_ existing vertices -> triangles
			B) Locate all unused vertices
			C) For each unused vertex, create a triangle connecting it to the existing mesh
		*/
		int inputSize = allindices.size(); // inputSize is number of points added to libtess2
		std::vector<int> vflags(inputSize); // Inits with 0's
		
		IndexedTriangle tri;
		for (int t=0;t<numelems;t++) {
			bool err = false;
			for (int i=0;i<3;i++) {
				int vidx = vindices[elements[t*3 + i]];
				if (vidx == TESS_UNDEF) err = true;
				else tri[i] = vidx; // A)
			}
			PRINTDB("%d (%d) %d (%d) %d (%d)",
							elements[t*3 + 0] % allindices[vindices[elements[t*3 + 0]]] %
							elements[t*3 + 1] % allindices[vindices[elements[t*3 + 1]]] %
							elements[t*3 + 2] % allindices[vindices[elements[t*3 + 2]]]);
			// FIXME: We ignore self-intersecting triangles rather than detecting and handling this
			if (!err) {
				vflags[tri[0]]++; // B)
				vflags[tri[1]]++;
				vflags[tri[2]]++;
				triangles.push_back(IndexedTriangle(allindices[tri[0]], allindices[tri[1]], allindices[tri[2]]));
			}
		}

		for (int i=0;i<inputSize;i++) {
			if (!vflags[i]) { // vertex missing in output: C)
				int starti = (i+inputSize-1)%inputSize;
				int j;
				for (j = i; j < inputSize && !vflags[j]; j++) {
					// Create triangle fan from vertex i-1 to the first existing vertex
					PRINTDB("(%d) (%d) (%d)\n", allindices[starti] % allindices[j] % allindices[((j+1)%inputSize)]);
					tri[0] = allindices[starti];
					tri[1] = allindices[j];
					tri[2] = allindices[(j+1)%inputSize];
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
				else tri[i] = allindices[vidx];
			}
			PRINTDB("%d (%d) %d (%d) %d (%d)",
							elements[t*3 + 0] % allindices[vindices[elements[t*3 + 0]]] %
							elements[t*3 + 1] % allindices[vindices[elements[t*3 + 1]]] %
							elements[t*3 + 2] % allindices[vindices[elements[t*3 + 2]]]);
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

/*!
	Tessellates a single contour. Non-indexed version.
*/
bool GeometryUtils::tessellatePolygon(const Polygon &polygon, Polygons &triangles,
																			const Vector3f *normal)
{
	bool err = false;
	Reindexer<Vector3f> uniqueVertices;
	IndexedPolygons indexedpolygons;
	indexedpolygons.faces.push_back(IndexedFace());
	IndexedFace &currface = indexedpolygons.faces.back();
	BOOST_FOREACH (const Vector3d &v, polygon) {
		int idx = uniqueVertices.lookup(v.cast<float>());
		if (currface.empty() || idx != currface.back()) currface.push_back(idx);
	}
	if (currface.front() == currface.back()) currface.pop_back();
	if (currface.size() >= 3) { // Cull empty triangles
		uniqueVertices.copy(std::back_inserter(indexedpolygons.vertices));
		std::vector<IndexedTriangle> indexedtriangles;
		err = tessellatePolygonWithHoles(indexedpolygons, indexedtriangles, normal);
		Vector3f *verts = &indexedpolygons.vertices.front();
		BOOST_FOREACH(const IndexedTriangle &t, indexedtriangles) {
			triangles.push_back(Polygon());
			Polygon &p = triangles.back();
			p.push_back(verts[t[0]].cast<double>());
			p.push_back(verts[t[1]].cast<double>());
			p.push_back(verts[t[2]].cast<double>());
		}
	}
	return err;
}
