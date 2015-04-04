#include "GeometryUtils.h"
#include "tesselator.h"
#include "printutils.h"
#include "Reindexer.h"
#include "grid.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

static void *stdAlloc(void* userData, unsigned int size) {
	TESS_NOTUSED(userData);
	return malloc(size);
}

static void stdFree(void* userData, void* ptr) {
	TESS_NOTUSED(userData);
	free(ptr);
}

typedef std::pair<int,int> IndexedEdge;

/*!
	Helper class for keeping track of edges in a mesh.
	Can probably be replaced with a proper HalfEdge mesh later on
*/
class EdgeDict {
public:
// Counts occurrences of edges
	typedef boost::unordered_map<IndexedEdge, int> IndexedEdgeDict;

	EdgeDict() { }

	void add(const IndexedFace &face) {
		for (size_t i=0;i<face.size();i++) {
			IndexedEdge e(face[(i+1)%face.size()], face[i]);
			if (this->count(e) > 0) this->remove(e);
			else this->add(e.second, e.first);
		}
	}

	void remove(const IndexedTriangle &t) {
		for (int i=0;i<3;i++) {
			IndexedEdge e(t[i], t[(i+1)%3]);
			// If the edge exist, remove it
			if (this->count(e) > 0) this->remove(e);
			else this->add(e.second, e.first);
		}
	}

	void add(const IndexedTriangle &t) {
		for (int i=0;i<3;i++) {
			IndexedEdge e(t[(i+1)%3], t[i]);
			// If an opposite edge exists, they cancel out
			if (this->count(e) > 0) this->remove(e);
			else this->add(e.second, e.first);
		}
	}

	void add(int start, int end) {
		this->add(IndexedEdge(start,end));
	}

	void add(const IndexedEdge &e) {
		this->edges[e]++;
		PRINTDB("add: (%d,%d)", e.first % e.second);
	}

	void remove(int start, int end) {
		this->remove(IndexedEdge(start,end));
	}

	void remove(const IndexedEdge &e) {
		this->edges[e]--;
		if (this->edges[e] == 0) this->edges.erase(e);
		PRINTDB("remove: (%d,%d)", e.first % e.second);
	}

	int count(int start, int end) {
		return this->count(IndexedEdge(start, end));
	}

	int count(const IndexedEdge &e) {
		IndexedEdgeDict::const_iterator it = this->edges.find(e);
		if (it != edges.end()) return it->second;
		return 0;
	}

	bool empty() const { return this->edges.empty(); }

	size_t size() const { return this->edges.size(); }

	void print() const {
		BOOST_FOREACH(const IndexedEdgeDict::value_type &v, this->edges) {
			const IndexedEdge &e = v.first;
			PRINTDB("     (%d,%d)%s", e.first % e.second % ((v.second > 1) ? boost::lexical_cast<std::string>(v.second).c_str() : ""));
		}
	}

	void remove_from_v2e(int vidx, int next, int prev) {
		std::list<int> &l = v2e[vidx];
		std::list<int>::iterator it = std::find(l.begin(), l.end(), next);
		if (it != l.end()) l.erase(it);
		if (l.empty()) v2e.erase(vidx);

		std::list<int> &l2 = v2e_reverse[vidx];
		it = std::find(l2.begin(), l2.end(), prev);
		if (it != l2.end()) l2.erase(it);
		if (l2.empty()) v2e_reverse.erase(vidx);
	}

	void extractTriangle(int vidx, int next, std::vector<IndexedTriangle> &triangles) {
		assert(v2e_reverse.find(vidx) != v2e_reverse.end());
		assert(!v2e_reverse[vidx].empty());
		int prev = v2e_reverse[vidx].front();
		
		IndexedTriangle t(prev, vidx, next);
		PRINTDB("Clipping ear: %d %d %d", t[0] % t[1] % t[2]);
		triangles.push_back(t);
		// Remove the generated triangle from the original.
		// Add new boundary edges to the edge dict
		this->remove(t);

            // If next->prev doesn't exists, add prev->next
            std::list<int>::iterator v2eit = std::find(v2e[next].begin(), v2e[next].end(), prev);
            if (v2eit == v2e[next].end()) {
                v2e[prev].push_back(next);
                v2e_reverse[next].push_back(prev);
            }
            remove_from_v2e(vidx, next, prev);
            remove_from_v2e(prev, vidx, next);
            remove_from_v2e(next, prev, vidx);
		
		
	}

	// Triangulate remaining loops and add to triangles
	void triangulateLoops(std::vector<IndexedTriangle> &triangles) {
		// First, look for self-intersections in edges
		v2e.clear();
		v2e_reverse.clear();
		BOOST_FOREACH(const IndexedEdgeDict::value_type &v, this->edges) {
			const IndexedEdge &e = v.first;
			for (int i=0;i<v.second;i++) {
				v2e[e.first].push_back(e.second);
				v2e_reverse[e.second].push_back(e.first);
			}
		}

		while (!v2e.empty()) {
			boost::unordered_map<int, std::list<int> >::iterator it;
			for (it = v2e.begin();it != v2e.end();it++) {
				if (it->second.size() == 1) { // First single vertex
					int vidx = it->first;
					int next = it->second.front();
					extractTriangle(vidx, next, triangles);
					break;
				}
			}
			// Only duplicate vertices left
			if (it == v2e.end() && !v2e.empty()) {
				int vidx = v2e.begin()->first;
				int next = v2e.begin()->second.front();
				extractTriangle(vidx, next, triangles);
			}
		}
	}

	IndexedEdgeDict edges;
	boost::unordered_map<int, std::list<int> > v2e;
	boost::unordered_map<int, std::list<int> > v2e_reverse;
};


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
bool GeometryUtils::tessellatePolygonWithHoles(const Vector3f *vertices,
																							 const std::vector<IndexedFace> &faces, 
																							 std::vector<IndexedTriangle> &triangles,
																							 const Vector3f *normal)
{
	// Algorithm outline:
  // o Remove consecutive equal vertices and null ears (i.e. 23,24,23)
	// o Ignore polygons with < 3 vertices
	// o Pass-through polygons with 3 vertices
	// o Pass polygon to libtess2
	// o Postprocess to clean up misbehaviors in libtess2

  // No polygon. FIXME: Will this ever happen or can we assert here?
  if (faces.empty()) return false;

	// Remove consecutive equal vertices, as well as null ears
	std::vector<IndexedFace> cleanfaces = faces;
  BOOST_FOREACH(IndexedFace &face, cleanfaces) {
		size_t i=0;
		while (face.size() >= 3 && i < face.size()) {
			if (face[i] == face[(i+1)%face.size()]) { // Two consecutively equal indices
				face.erase(face.begin()+i);
			}
			else if (face[(i+face.size()-1)%face.size()] == face[(i+1)%face.size()]) { // Null ear
				if (i == 0) face.erase(face.begin() + i, face.begin() + i + 2);
				else face.erase(face.begin() + i - 1, face.begin() + i + 1);
				i--;
			}
			else {
				// Filter away inf and nan vertices as they cause libtess2 to crash
				const Vector3f &v = vertices[face[i]];
				int k;
				for (k=0;k<3;k++) {
					if (boost::math::isnan(v[k]) || boost::math::isinf(v[k])) {
						face.erase(face.begin()+i);
						break;
					}
				}
				if (k == 3) i++;
			}
		}
	}
	// First polygon has < 3 points - no output
	if (cleanfaces[0].size() < 3) return false;
	// Remove collapsed holes
	for (size_t i=1;i<cleanfaces.size();i++) {
		if (cleanfaces[i].size() < 3) {
			cleanfaces.erase(cleanfaces.begin() + i);
			i--;
		}
	}

	if (cleanfaces.size() == 1 && cleanfaces[0].size() == 3) {
		// Input polygon has 3 points. shortcut tessellation.
		//PRINTDB("  tri: %d %d %d", cleanfaces[0][0] % cleanfaces[0][1] % cleanfaces[0][2]);
		triangles.push_back(IndexedTriangle(cleanfaces[0][0], cleanfaces[0][1], cleanfaces[0][2]));
		return false;
	}

	// Build edge dict.
  // This contains all edges in the original polygon.
	// To maintain connectivity, all these edges must exist in the output.
	EdgeDict edges;
	BOOST_FOREACH(IndexedFace &face, cleanfaces) {
		edges.add(face);
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
	// Since libtess2's indices is based on the running number of points added, we need to map back
	// to our indices. allindices does the mapping.
	std::vector<int> allindices;
  BOOST_FOREACH(const IndexedFace &face, cleanfaces) {
    contour.clear();
    BOOST_FOREACH(int idx, face) {
			const Vector3f &v = vertices[idx];
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

		In addition, libtess2 may generate flipped triangles, i.e. triangles
		where the edge direction is reversed compared to the input polygon.
		This will also destroy connectivity and we need to flip those back.
	 */
		/*
			Algorithm:
			A) Collect all triangles using _only_ existing vertices -> triangles
			B) Locate all unused vertices
			C) For each unused vertex, create a triangle connecting it to the existing mesh
		*/
		int inputSize = allindices.size(); // inputSize is number of points added to libtess2
		std::vector<int> vflags(inputSize); // Inits with 0's
		
		IndexedTriangle tri;
		IndexedTriangle mappedtri;
		for (int t=0;t<numelems;t++) {
			bool err = false;
			mappedtri.fill(-1);
			for (int i=0;i<3;i++) {
				int vidx = vindices[elements[t*3 + i]];
				if (vidx == TESS_UNDEF) {
					err = true;
				}
				else {
					tri[i] = vidx; // A)
					mappedtri[i] = allindices[vidx];
				}
			}
			PRINTDB("%d (%d) %d (%d) %d (%d)",
							elements[t*3 + 0] % mappedtri[0] %
							elements[t*3 + 1] % mappedtri[1] %
							elements[t*3 + 2] % mappedtri[2]);
			// FIXME: We ignore self-intersecting triangles rather than detecting and handling this
			if (!err) {
				vflags[tri[0]]++; // B)
				vflags[tri[1]]++;
				vflags[tri[2]]++;

				// For each edge in mappedtri, locate the opposite edge in the original polygon.
				// If an opposite edge was found, we need to flip.
				// In addition, remove each edge from the dict to be able to later find
				// missing edges.
				// Note: In some degenerate cases, we create triangles with mixed edge directions.
				// In this case, don't reverse, but attempt to carry on
				bool reverse = false;
				for (int i=0;i<3;i++) {
					const IndexedEdge e(mappedtri[i], mappedtri[(i+1)%3]);
					if (edges.count(e) > 0) {
						reverse = false;
						break;
					}
					else if (edges.count(e.second, e.first) > 0) {
						reverse = true;
					}
				}
				if (reverse) {
					mappedtri.reverseInPlace();
					PRINTDB("  reversed: %d %d %d",
									mappedtri[0] % mappedtri[1] % mappedtri[2]);
				}

				// Remove the generated triangle from the original.
				// Add new boundary edges to the edge dict
				edges.remove(mappedtri);
				triangles.push_back(mappedtri);
			}
		}

		if (!edges.empty()) {
			PRINTDB("   %d edges remaining after main triangulation", edges.size());
			edges.print();
			
			// Collect loops from remaining edges and triangulate loops manually
			edges.triangulateLoops(triangles);

			if (!edges.empty()) {
				PRINTDB("   %d edges remaining after loop triangulation", edges.size());
				edges.print();
			}
		}
#if 0
		for (int i=0;i<inputSize;i++) {
			if (!vflags[i]) { // vertex missing in output: C)
				int starti = (i+inputSize-1)%inputSize;
				int j;
				PRINTD("   Fanning left-out vertices");
				for (j = i; j < inputSize && !vflags[j]; j++) {
					// Create triangle fan from vertex i-1 to the first existing vertex
					PRINTDB("   (%d) (%d) (%d)\n", allindices[starti] % allindices[j] % allindices[((j+1)%inputSize)]);
					tri[0] = allindices[starti];
					tri[1] = allindices[j];
					tri[2] = allindices[(j+1)%inputSize];
					vflags[tri[0]]++;
					vflags[tri[1]]++;
					vflags[tri[2]]++;
					triangles.push_back(tri);
				}
				i = j;
			}
		}
#endif

  tessDeleteTess(tess);

  return false;
}

/*!
	Tessellates a single contour. Non-indexed version.
	Appends resulting triangles to triangles.
*/
bool GeometryUtils::tessellatePolygon(const Polygon &polygon, Polygons &triangles,
																			const Vector3f *normal)
{
	bool err = false;
	Reindexer<Vector3f> uniqueVertices;
	std::vector<IndexedFace> indexedfaces;
	indexedfaces.push_back(IndexedFace());
	IndexedFace &currface = indexedfaces.back();
	BOOST_FOREACH (const Vector3d &v, polygon) {
		int idx = uniqueVertices.lookup(v.cast<float>());
		if (currface.empty() || idx != currface.back()) currface.push_back(idx);
	}
	if (currface.front() == currface.back()) currface.pop_back();
	if (currface.size() >= 3) { // Cull empty triangles
		const Vector3f *verts = uniqueVertices.getArray();
		std::vector<IndexedTriangle> indexedtriangles;
		err = tessellatePolygonWithHoles(verts, indexedfaces, indexedtriangles, normal);
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

int GeometryUtils::findUnconnectedEdges(const std::vector<std::vector<IndexedFace> > &polygons)
{
	EdgeDict edges;
	BOOST_FOREACH(const std::vector<IndexedFace> &faces, polygons) {
		BOOST_FOREACH(const IndexedFace &face, faces) {
			edges.add(face);
		}
	}
#if 1 // for debugging
	if (!edges.empty()) {
		PRINTD("Unconnected:");
		edges.print();
	}
#endif
	return edges.size();
}

int GeometryUtils::findUnconnectedEdges(const std::vector<IndexedTriangle> &triangles)
{
	EdgeDict edges;
	BOOST_FOREACH(const IndexedTriangle &t, triangles) {
		edges.add(t);
	}
#if 1 // for debugging
	if (!edges.empty()) {
		PRINTD("Unconnected:");
		edges.print();
	}
#endif

	return edges.size();
}
