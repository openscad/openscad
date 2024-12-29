#include "geometry/GeometryEvaluator.h"
#include "core/Tree.h"
#include "geometry/GeometryCache.h"
#include "geometry/Polygon2d.h"
#include "core/ModuleInstantiation.h"
#include "core/State.h"
#include "core/ColorNode.h"
#include "core/OffsetNode.h"
#include "core/TransformNode.h"
#include "core/LinearExtrudeNode.h"
#include "core/PathExtrudeNode.h"
#include "core/RoofNode.h"
#include "geometry/roof_ss.h"
#include "geometry/roof_vd.h"
#include "core/RotateExtrudeNode.h"
#include "core/PullNode.h"
#include "core/DebugNode.h"
#include "core/WrapNode.h"
#include "geometry/rotextrude.h"
#include "core/CgalAdvNode.h"
#include "core/ProjectionNode.h"
#include "core/CsgOpNode.h"
#include "core/TextNode.h"
#include "core/RenderNode.h"
#include "geometry/ClipperUtils.h"
#include "geometry/PolySetUtils.h"
#include "geometry/PolySet.h"
#include "glview/Renderer.h"
#include "geometry/PolySetBuilder.h"
#include "utils/calc.h"
#include "utils/printutils.h"
#include "utils/calc.h"
#include "io/DxfData.h"
#include "glview/RenderSettings.h"
#include "utils/degree_trig.h"
#include <cmath>
#include <iterator>
#include <cassert>
#include <list>
#include <utility>
#include <memory>
#include <algorithm>
#include "utils/boost-utils.h"
#include "geometry/boolean_utils.h"
#include <hash.h>
#include <Selection.h>
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#include "geometry/cgal/cgalutils.h"
#include <CGAL/convex_hull_2.h>
#include <CGAL/Point_2.h>
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif
#include "geometry/linear_extrude.h"


#ifdef ENABLE_PYTHON
#include <src/python/python_public.h>
#endif
#include <cstddef>
#include <vector>

class Geometry;
class Polygon2d;
class Tree;

GeometryEvaluator::GeometryEvaluator(const Tree& tree) : tree(tree) { }

/*!
   Set allownef to false to force the result to _not_ be a Nef polyhedron

   There are some guarantees on the returned geometry:
   * 2D and 3D geometry cannot be mixed; we will return either _only_ 2D or _only_ 3D geometries
   * PolySet geometries are always 3D. 2D Polysets are only created for special-purpose rendering operations downstream from here.
   * Needs validation: Implementation-specific geometries shouldn't be mixed (Nef polyhedron, Manifold)
 */
std::shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode& node,
                                                               bool allownef)
{
  auto result = smartCacheGet(node, allownef);
  if (!result) {
    // If not found in any caches, we need to evaluate the geometry
    // traverse() will set this->root to a geometry, which can be any geometry
    // (including GeometryList if the lazyunions feature is enabled)
    this->traverse(node);
    result = this->root;

    // Insert the raw result into the cache.
    smartCacheInsert(node, result);
  }

  // Convert engine-specific 3D geometry to PolySet if needed
  // Note: we don't store the converted into the cache as it would conflict with subsequent calls where allownef is true.
  if (!allownef) {
    if (auto ps = PolySetUtils::getGeometryAsPolySet(result)) {
      assert(ps->getDimension() == 3);
      // We cannot render concave polygons, so tessellate any PolySets
      if (!ps->isEmpty() && !ps->isTriangular()) {
        // Since is_convex() doesn't handle non-planar faces, we need to tessellate
        // also in the indeterminate state so we cannot just use a boolean comparison. See #1061
        bool convex = bool(ps->convexValue()); // bool is true only if tribool is true, (not indeterminate and not false)
        if (!convex) {
          ps = PolySetUtils::tessellate_faces(*ps);
        }
      }
      return ps;
    }
  }
  return result;
}
void vectorDump(const char *msg, const Vector3d &vec) {
  printf("%s ",msg );
    printf("(%g/%g/%g) ",vec[0], vec[1], vec[2]);    
}

void triangleDump(const char *msg, const IndexedFace &face, const std::vector<Vector3d> &vert) {
  printf("%s ",msg );
  for(int i=0;i<face.size();i++) {
    const Vector3d &pt = vert[face[i]];
    vectorDump(" ", pt);
  }
}

Vector4d calcTriangleNormal(const std::vector<Vector3d> &vertices,const IndexedFace &pol)
{
	int n=pol.size();
	assert(pol.size() >= 3);
	Vector3d norm(0,0,0);
	for(int j=0;j<n-2;j++) {
		// need to calculate all normals, as 1st 2 could be in a concave corner
		Vector3d diff1=(vertices[pol[0]] - vertices[pol[j+1]]);
		Vector3d diff2=(vertices[pol[j+1]] - vertices[pol[j+2]]);
		norm += diff1.cross(diff2);
	}
	norm.normalize();
	Vector3d pt=vertices[pol[0]];
	double off=norm.dot(pt);
	return Vector4d(norm[0],norm[1],norm[2],off);
}


std::vector<Vector4d> calcTriangleNormals(const std::vector<Vector3d> &vertices, const std::vector<IndexedFace> &indices)
{
	std::vector<Vector4d>  faceNormal;
	for(unsigned int i=0;i<indices.size();i++) {
		IndexedFace pol = indices[i];
		assert (pol.size() >= 3);
		faceNormal.push_back(calcTriangleNormal(vertices, pol));
	}
	return faceNormal;

}

bool pointInPolygon(const std::vector<Vector3d> &vert, const IndexedFace &bnd, int ptind)
{
	int i,n;
	double dist;
	n=bnd.size();
	int cuts=0;
	Vector3d p1, p2;
	Vector3d pt=vert[ptind];
	Vector3d res;
	if(n < 3) return false;
	Vector3d raydir=vert[bnd[1]]-vert[bnd[0]];
	Vector3d fn=raydir.cross(vert[bnd[1]]-vert[bnd[2]]).normalized();
	// check, how many times the ray crosses the 3D fence, classical algorithm in 3D
	for(i=1;i<n;i++) { // 0 is always parallel
		// build fence side
		const Vector3d &p1=vert[bnd[i]];
		const Vector3d &p2=vert[bnd[(i+1)%n]];

                if(linsystem( p2-p1, raydir,fn,pt-p1,res)) continue;

		if(res[1] > 0) continue; // not behind
		if(res[0] < 0) continue; // within segment
		if(res[0] > 1) continue;
		cuts++;
	}
	return (cuts&1)?true:false;
}

unsigned int hash_value(const EdgeKey& r) {
        unsigned int i;
        i=r.ind1 |(r.ind2<<16) ;
        return i;
}
EdgeKey::EdgeKey(int i1, int i2) {
  this->ind1=i1<i2?i1:i2;
  this->ind2=i2>i1?i2:i1;
}

int operator==(const EdgeKey &t1, const EdgeKey &t2) 
{
        if(t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
        return 0;
}


std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey> > createEdgeDb(const std::vector<IndexedFace> &indices)
{
  std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey> > edge_db;
  EdgeKey edge;                                                    
  //
  // Create EDGE DB
  EdgeVal val;
  val.sel=0;
  val.facea=-1;
  val.faceb=-1;
  val.posa=-1;
  val.posb=-1;
  int ind1, ind2;
  for(int i=0;i<indices.size();i++) {
    int  n=indices[i].size();
    for(int j=0;j<n;j++) {
      ind1=indices[i][j];	    
      ind2=indices[i][(j+1)%n];	    
      if(ind2 > ind1){
        edge.ind1=ind1;
        edge.ind2=ind2;	
	if(edge_db.count(edge) == 0) edge_db[edge]=val;
	edge_db[edge].facea=i;
	edge_db[edge].posa=j;
      } else {
        edge.ind1=ind2;
        edge.ind2=ind1;	
	if(edge_db.count(edge) == 0) edge_db[edge]=val;
	edge_db[edge].faceb=i;
	edge_db[edge].posb=j;
      }
    }    
  }
  int error=0;
  for(auto &e: edge_db) {
    if(e.second.facea == -1 || e.second.faceb == -1) {
      printf("Mismatched EdgeDB ind1=%d idn2=%d facea=%d faceb=%d\n",e.first.ind1, e.first.ind2, e.second.facea, e.second.faceb);
      error=1;
    }
  }
  if(error) {
    for(unsigned int i=0;i<indices.size();i++)
    {
      auto &face=indices[i];
      printf("%d :",i);
      for(unsigned int j=0;j<face.size();j++) printf("%d ",face[j]);
      printf("\n");
    } // tri 5-9-11 missing
    assert(0);	      
  }
  return edge_db;
}
	
bool GeometryEvaluator::isValidDim(const Geometry::GeometryItem& item, unsigned int& dim) const {
  if (!item.first->modinst->isBackground() && item.second) {
    if (!dim) dim = item.second->getDimension();
    else if (dim != item.second->getDimension() && !item.second->isEmpty()) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Mixing 2D and 3D objects is not supported");
      return false;
    }
  }
  return true;
}

using Eigen::Vector4d;
typedef std::vector<IndexedFace> indexedFaceList;

bool mergeTrianglesOpposite(const IndexedFace & poly1, const IndexedFace &poly2)
{
	if(poly1.size() != poly2.size()) return false;
	int n=poly1.size();
	int off=-1;
	for(int i=0;off == -1 && i<n;i++)
		if(poly1[0] == poly2[i]) off=i;
	if(off == -1) return false;

	for(int i=1;i<n;i++)
		if(poly1[i] != poly2[(off+n-i)%n])
			return false;

	return true;
}

typedef std::vector<int> intList;
typedef std::vector<intList> intListList;

class TriCombineStub
{
	public:
		int ind1, ind2, ind3 ;
		int operator==(const TriCombineStub ref)
		{
			if(this->ind1 == ref.ind1 && this->ind2 == ref.ind2) return 1;
			return 0;
		}
};

unsigned int hash_value(const TriCombineStub& r) {
	unsigned int i;
	i=r.ind1 |(r.ind2<<16) ;
        return i;
}

int operator==(const TriCombineStub &t1, const TriCombineStub &t2) 
{
	if(t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
	return 0;
}

static indexedFaceList mergeTrianglesSub(const std::vector<IndexedFace> &triangles, const std::vector<Vector3d> &vert)
{
	unsigned int i,j,n;
	int ind1, ind2;
	std::unordered_set<TriCombineStub, boost::hash<TriCombineStub> > stubs_pos;
	std::unordered_set<TriCombineStub, boost::hash<TriCombineStub> > stubs_neg;

	TriCombineStub e;
	for(i=0;i<triangles.size();i++)
	{
		n=triangles[i].size();
		for(j=0;j<n;j++)
		{
			ind1=triangles[i][j];
			ind2=triangles[i][(j+1)%n];
			if(ind2 > ind1) // positive edge
			{
				e.ind1=ind1;
				e.ind2=ind2;
				if(stubs_neg.find(e) != stubs_neg.end()) stubs_neg.erase(e);
				else if(stubs_pos.find(e) != stubs_pos.end()) printf("Duplicate Edge %d->%d \n",ind1,ind2);
				else stubs_pos.insert(e);
			}
			if(ind2 < ind1) // negative edge
			{
				e.ind1=ind2;
				e.ind2=ind1;
				if(stubs_pos.find(e) != stubs_pos.end() ) stubs_pos.erase(e);
				else if(stubs_neg.find(e) != stubs_neg.end() ) printf("Duplicate Edge %d->%d \n",ind2,ind1);
				else stubs_neg.insert(e);
			}
		}
	}	

	// now chain everything
	std::unordered_map<int,int> stubs_chain;
	std::vector<TriCombineStub> stubs;
	std::vector<TriCombineStub> stubs_bak;
	for( const auto& stubs : stubs_pos ) {
		if(stubs_chain.count(stubs.ind1) > 0)
		{
			stubs_bak.push_back(stubs);
		} else stubs_chain[stubs.ind1]=stubs.ind2;
	}

	for( const auto& stubs : stubs_neg ) {
		if(stubs_chain.count(stubs.ind2) > 0)
		{
			TriCombineStub ts;
			ts.ind1=stubs.ind2;
			ts.ind2=stubs.ind1;
			stubs_bak.push_back(ts);
		} else stubs_chain[stubs.ind2]=stubs.ind1;
	}
	std::vector<IndexedFace> result;

	while(stubs_chain.size() > 0)
	{
		int ind,ind_new;
		auto [ind_start,dummy]  = *(stubs_chain.begin());
		ind=ind_start;
		IndexedFace poly;
		while(1)
		{
			if(stubs_chain.count(ind) > 0)
			{
				ind_new=stubs_chain[ind];
				stubs_chain.erase(ind);
			}
			else
			{
				ind_new=-1;
				for(i=0;ind_new == -1 && i<stubs_bak.size();i++)
				{
					if(stubs_bak[i].ind1 == ind)
					{
						ind_new=stubs_bak[i].ind2;
						std::vector<TriCombineStub>::iterator it=stubs_bak.begin();
						std::advance(it,i);
						stubs_bak.erase(it);
						break;
					}
				}
				if(ind_new == -1) break;
			}
			poly.push_back(ind_new);
			if(ind_new == ind_start) break; // chain closed
			ind=ind_new;
		}

		// spitz-an-spitz loesen, in einzelketten trennen 
		unsigned int beg,end, begbest;
		int dist, distbest,repeat;
		do
		{
			repeat=0;
			std::unordered_map<int,int> value_pos;
			distbest=-1;
			n=poly.size();
			for(i=0;i<n;i++)
			{
				ind=poly[i];
				if(value_pos.count(ind) > 0)
				{
					beg=value_pos[ind];
					end=i;
					dist=end-beg;
					std::unordered_map<int,int> value_pos2;
					int doubles=0;
					for(j=beg;j<end;j++)
					{
						if(value_pos2.count(poly[j]) > 0)
							doubles=1;
					}

					if(dist > distbest && doubles == 0) // es duerfen sich keine doppelten zahlen drinnen befinden
					{
						distbest=dist;
						begbest=beg;
					}
					if(end-beg == 2)
					{
						for(int i=0;i<2;i++) { // TODO make more efficient
							auto it=poly.begin();
							std::advance(it,beg);
							poly.erase(it);
						}
						repeat=1;
						distbest=-1;
						break;
					}
					if(beg+n-end == 2)
					{
						IndexedFace polynew;
						for(j=beg;j<end;j++)
							polynew.push_back(poly[j]);
						poly=polynew;
						repeat=1;
						distbest=-1;
						break;
					}
				}
				value_pos[ind]=i;
			}
			if(distbest  != -1)
			{
				IndexedFace polynew;
				for(i=begbest;i<begbest+distbest;i++)
				{
					polynew.push_back(poly[i]);
				}
				if(polynew.size() >= 3) result.push_back(polynew);
				for(int i=0;i<distbest;i++) { // TODO make more efficient
					auto  it=poly.begin();
					std::advance(it,begbest);
					poly.erase(it);
				}
				repeat=1;
			}
		}
		while(repeat);
		// Reduce colinear points
		int n=poly.size();
		IndexedFace poly_new;
		int last=poly[n-1],cur=poly[0],next;
		for(int i=0;i< n;i++) {
			next=poly[(i+1)%n];
			Vector3d p0=vert[last];
			Vector3d p1=vert[cur];
			Vector3d p2=vert[next];
			if(1) { // (p2-p1).cross(p1-p0).norm() > 0.00001) {
			// TODO enable again, need partner also to remove
				poly_new.push_back(cur);
				last=cur;
				cur=next;
			} else {
				cur=next;
			}
		}
		
		if(poly_new.size() > 2) result.push_back(poly_new);
	}
	return result;
}


std::vector<IndexedFace> mergeTriangles(const std::vector<IndexedFace> polygons,const std::vector<Vector4d> normals,std::vector<Vector4d> &newNormals, std::vector<int> &faceParents, const std::vector<Vector3d> &vert) 
{
	indexedFaceList emptyList;
	std::vector<Vector4d> norm_list;
	std::vector<indexedFaceList>  polygons_sorted;
	// sort polygons into buckets of same orientation
	for(unsigned int i=0;i<polygons.size();i++) {
		Vector4d norm=normals[i];
		const IndexedFace &triangle = polygons[i]; 

		int norm_ind=-1;
		for(unsigned int j=0;norm_ind == -1 && j<norm_list.size();j++) {
			const auto &cur = norm_list[j];
			if(cur.head<3>().dot(norm.head<3>()) > 0.99999 && fabs(cur[3] - norm[3]) < 0.001) {
				norm_ind=j;
			}
			if(cur.norm() < 1e-6 && norm.norm() < 1e-6) norm_ind=j; // zero vector matches zero vector
		}
		if(norm_ind == -1) {
			norm_ind=norm_list.size();
			norm_list.push_back(norm);
			polygons_sorted.push_back(emptyList);
		}
		polygons_sorted[norm_ind].push_back(triangle);
	}

	//  now put back hole of polygons into the correct bucket
	for(unsigned int i=0;i<polygons_sorted.size();i++ ) {
		// check if bucket has an opposite oriented bucket
		//Vector4d n = norm_list[i];
		int i_=-1;
		Vector3d nref = norm_list[i].head<3>();
		for(unsigned int j=0;j<polygons_sorted.size();j++) {
			if(j == i) continue;
			if(norm_list[j].head<3>().dot(nref) < -0.999 && fabs(norm_list[j][3] + norm_list[i][3]) < 0.005) {
				i_=j;
				break;
			}
		}
		if(i_ == -1) continue;
		// assuming that i_ contains the holes, find, it there is a match

		for(unsigned int k=0;k< polygons_sorted[i].size();k++) {
			IndexedFace poly = polygons_sorted[i][k];
			for(unsigned int l=0;l<polygons_sorted[i_].size();l++) {
				IndexedFace hole = polygons_sorted[i_][l];
//				// holes dont intersect with the polygon, so its sufficent to check, if one point of the hole is inside the polygon
				if(mergeTrianglesOpposite(poly, hole)){
					polygons_sorted[i].erase(polygons_sorted[i].begin()+k);
					polygons_sorted[i_].erase(polygons_sorted[i_].begin()+l);
					k--;
					l--;
				}
				else if(pointInPolygon(vert, poly, hole[0])){
					polygons_sorted[i].push_back(hole);
					polygons_sorted[i_].erase(polygons_sorted[i_].begin()+l);
					l--; // could have more holes
				}
			}
		}
		
		
	}

	// now merge the polygons in all buckets independly
	std::vector<IndexedFace> indices;
	newNormals.clear();
	faceParents.clear();
	for(unsigned int i=0;i<polygons_sorted.size();i++ ) {
		indexedFaceList indices_sub = mergeTrianglesSub(polygons_sorted[i], vert);
		int off=indices.size();
		for(unsigned int j=0;j<indices_sub.size();j++) {
			indices.push_back(indices_sub[j]);
			newNormals.push_back(norm_list[i]);
			Vector4d loc_norm = calcTriangleNormal(vert,indices_sub[j]);
			if(norm_list[i].head<3>().dot(loc_norm.head<3>()) > 0) 
				faceParents.push_back(-1); 
			else {
				int par=-1;
				for(unsigned int k=0;k< indices_sub.size();k++)
				{
					if(k == j) continue;
					if(pointInPolygon(vert, indices_sub[k],indices_sub[j][0])) {
						par=k;
					}
				}
				if(par == -1) printf("par not found here\n");
				// assert(par != -1); TODO fix
				faceParents.push_back(par+off);
			}			
		}
	}


	return indices;
}


Map3DTree::Map3DTree(void) {
	for(int i=0;i<8;ind[i++]=-1);
	ptlen=0;
}

Map3D::Map3D(Vector3d min, Vector3d max) {
	this->min=min;
	this->max=max;
}
void Map3D::add_sub(int ind, Vector3d min, Vector3d max,Vector3d pt, int ptind, int disable_local_num) {
	int indnew;
	int corner;
	Vector3d mid;
	do {
		if(items[ind].ptlen >= 0 && disable_local_num != ind) {
			if(items[ind].ptlen < BUCKET) {
				for(int i=0;i<items[ind].ptlen;i++)
					if(items[ind].pts[i] == pt) return;
				items[ind].pts[items[ind].ptlen]=pt;
				items[ind].ptsind[items[ind].ptlen]=ptind;
				items[ind].ptlen++;
				return;
			} else {  
				for(int i=0;i<items[ind].ptlen;i++) {
					add_sub(ind, min, max,items[ind].pts[i],items[ind].ptsind[i],ind);
				}
				items[ind].ptlen=-1;
				// run through
			} 
		}
		mid[0]=(min[0]+max[0])/2.0;
		mid[1]=(min[1]+max[1])/2.0;
		mid[2]=(min[2]+max[2])/2.0;
		corner=(pt[0]>=mid[0]?1:0)+(pt[1]>=mid[1]?2:0)+(pt[2]>=mid[2]?4:0);
		indnew=items[ind].ind[corner];
		if(indnew == -1) {
			indnew=items.size();
			items.push_back(Map3DTree());
			items[ind].ind[corner]=indnew;					
		}
		if(corner&1) min[0]=mid[0]; else max[0]=mid[0];
		if(corner&2) min[1]=mid[1]; else max[1]=mid[1];
		if(corner&4) min[2]=mid[2]; else max[2]=mid[2];
		ind=indnew;
	}
	while(1);

}
void Map3D::add(Vector3d pt, int ind) { 
	if(items.size() == 0) {
		items.push_back(Map3DTree());
		items[0].pts[0]=pt;
		items[0].ptsind[0]=ind;
		items[0].ptlen++;
		return;
	}
	add_sub(0,this->min, this->max, pt,ind,-1);
}

void Map3D::del(Vector3d pt) {
	int ind=0;
	int corner;
	Vector3d min=this->min;
	Vector3d max=this->max;
	Vector3d mid;
	printf("Deleting %g/%g/%g\n",pt[0], pt[1], pt[2]);
	while(ind != -1) {
		for(int i=0;i<items[ind].ptlen;i++) {
			if(items[ind].pts[i]==pt) {
				for(int j=i+1;j<items[ind].ptlen;j++)
					items[ind].pts[j-1]=items[ind].pts[j];
				items[ind].ptlen--;
				return;
			}
			// was wenn leer wird dnn sind ind immer noch -1
		}
		mid[0]=(min[0]+max[0])/2.0;
		mid[1]=(min[1]+max[1])/2.0;
		mid[2]=(min[2]+max[2])/2.0;
		corner=(pt[0]>mid[0]?1:0)+(pt[1]>mid[1]?2:0)+(pt[2]>mid[2]?4:0);
		printf("corner=%d\n",corner);
		ind=items[ind].ind[corner];
		if(corner&1) min[0]=mid[0]; else max[0]=mid[0];
		if(corner&2) min[1]=mid[1]; else max[1]=mid[1];
		if(corner&4) min[2]=mid[2]; else max[2]=mid[2];
	}
}

void Map3D::find_sub(int ind, double minx, double miny, double minz, double maxx, double maxy, double maxz, Vector3d pt, double r,std::vector<Vector3d> &result,std::vector<int> &resultind, int maxresult){
	if(ind == -1) return;
	if(this->items[ind].ptlen > 0){
		for(int i=0;i<this->items[ind].ptlen;i++) {
			if((this->items[ind].pts[i]-pt).norm() < r) {
				result.push_back(this->items[ind].pts[i]);
				resultind.push_back(this->items[ind].ptsind[i]);
			}
			if(result.size() >= maxresult) return;
		}
		return;
	}
	double midx,midy, midz;
//	printf("find_sub ind=%d %g/%g/%g - %g/%g/%g\n",ind, minx, miny,  minz, maxx, maxy, maxz );
	midx=(minx+maxx)/2.0;
	midy=(miny+maxy)/2.0;
	midz=(minz+maxz)/2.0;
	if(result.size() >= maxresult) return;
	if( pt[2]+r >= minz && pt[2]-r < midz ) {
		if( pt[1]+r >= miny && pt[1]-r < midy) {
			if(pt[0]+r >= minx && pt[0]-r < midx ) find_sub(this->items[ind].ind[0],minx, miny, minz, midx, midy, midz,pt,r,result,resultind, maxresult);
			if(pt[0]+r >= midx && pt[0]-r < maxx ) find_sub(this->items[ind].ind[1],midx, miny, minz, maxx, midy, midz,pt,r,result,resultind, maxresult);
		}
		if( pt[1]+r >= midy && pt[1]-r < maxy) {
			if(pt[0]+r >= minx && pt[0]-r < midx ) find_sub(this->items[ind].ind[2],minx, midy, minz, midx, maxy, midz,pt,r,result,resultind, maxresult);
			if(pt[0]+r >= midx && pt[0]-r < maxx ) find_sub(this->items[ind].ind[3],midx, midy, minz, maxx, maxy, midz,pt,r,result,resultind, maxresult);
		}
	}
	if( pt[2]+r >= midz && pt[2]-r < maxz ) {
		if( pt[1]+r >= miny && pt[1]-r < midy) {
			if(pt[0]+r >= minx && pt[0]-r < midx ) find_sub(this->items[ind].ind[4],minx, miny, midz, midx, midy, maxz,pt,r,result,resultind, maxresult);
			if(pt[0]+r >= midx && pt[0]-r < maxx ) find_sub(this->items[ind].ind[5],midx, miny, midz, maxx, midy, maxz,pt,r,result,resultind, maxresult);
		}
		if( pt[1]+r >= midy && pt[1]-r < maxy) {
			if(pt[0]+r >= minx && pt[0]-r < midx ) find_sub(this->items[ind].ind[6],minx, midy, midz, midx, maxy, maxz,pt,r,result,resultind, maxresult);
			if(pt[0]+r >= midx && pt[0]-r < maxx ) find_sub(this->items[ind].ind[7],midx, midy, midz, maxx, maxy, maxz,pt,r,result,resultind, maxresult);
		}
	}

}
int Map3D::find(Vector3d pt, double r,std::vector<Vector3d> &result, std::vector<int> &resultind, int maxresult){
	int results=0;
	if(items.size() == 0) return results;
	result.clear();
	resultind.clear();
	find_sub(0,this->min[0], this->min[1], this->min[2], this->max[0], this->max[1], this->max[2], pt,r,result, resultind, maxresult);
	return result.size();
}

void Map3D::dump_hier(int i, int hier, float minx, float miny, float minz, float maxx, float maxy, float maxz) {
	for(int i=0;i<hier;i++) printf("  ");
	printf("%d inds ",i);
	for(int j=0;j<8;j++) printf("%d ",items[i].ind[j]);
	printf("pts ");
	for(int j=0;j<items[i].ptlen;j++) printf("%g/%g/%g ",items[i].pts[j][0],items[i].pts[j][1],items[i].pts[j][2]);

	float midx, midy, midz;
	midx=(minx+maxx)/2.0;
	midy=(miny+maxy)/2.0;
	midz=(minz+maxz)/2.0;
	printf(" (%g/%g/%g - %g/%g/%g)\n", minx, miny, minz, maxx, maxy, maxz);
	if(items[i].ind[0] != -1) dump_hier(items[i].ind[0],hier+1,minx,miny, minz, midx, midy, midz);
	if(items[i].ind[1] != -1) dump_hier(items[i].ind[1],hier+1,midx,miny, minz, maxx, midy, midz);
	if(items[i].ind[2] != -1) dump_hier(items[i].ind[2],hier+1,minx,midy, minz, midx, maxy, midz);
	if(items[i].ind[3] != -1) dump_hier(items[i].ind[3],hier+1,midx,midy, minz, maxx, maxy, midz);
	if(items[i].ind[4] != -1) dump_hier(items[i].ind[4],hier+1,minx,miny, midz, midx, midy, maxz);
	if(items[i].ind[5] != -1) dump_hier(items[i].ind[5],hier+1,midx,miny, midz, maxx, midy, maxz);
	if(items[i].ind[6] != -1) dump_hier(items[i].ind[6],hier+1,minx,midy, midz, midx, maxy, maxz);
	if(items[i].ind[7] != -1) dump_hier(items[i].ind[7],hier+1,midx,midy, midz, maxx, maxy, maxz);
}
void Map3D::dump(void) {
	dump_hier(0,0,min[0],min[1],min[2],max[0], max[1],max[2]);
}



GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode& node, OpenSCADOperator op)
{
  unsigned int dim = 0;
  for (const auto& item : this->visitedchildren[node.index()]) {
    if (!isValidDim(item, dim)) break;
  }
  if (dim == 2) return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyToChildren2D(node, op)));
  else if (dim == 3) return applyToChildren3D(node, op);
  return {};
}

int cut_face_face_face(Vector3d p1, Vector3d n1, Vector3d p2,Vector3d n2, Vector3d p3, Vector3d n3, Vector3d &res,double *detptr)
{
        //   vec1     vec2     vec3
        // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
        // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
        // x*dirx + y*diry + z*dirz =( posx*dirx + posy*diry + posz*dirz )
        Vector3d vec1,vec2,vec3,sum;
        vec1[0]=n1[0]; vec1[1]= n2[0] ; vec1[2] = n3[0];
        vec2[0]=n1[1]; vec2[1]= n2[1] ; vec2[2] = n3[1];
        vec3[0]=n1[2]; vec3[1]= n2[2] ; vec3[2] = n3[2];
        sum[0]=p1.dot(n1);
        sum[1]=p2.dot(n2);
        sum[2]=p3.dot(n3);
        return linsystem( vec1,vec2,vec3,sum,res,detptr);
}

int cut_face_line(Vector3d fp, Vector3d fn, Vector3d lp, Vector3d ld, Vector3d &res, double *detptr)
{
	Vector3d c1 = fn.cross(ld);
	Vector3d c2 = fn.cross(c1);
	Vector3d diff=fp-lp;
        if(linsystem(ld, c1, c2, diff,res,detptr)) return 1;
	res=lp+ld*res[0];
	return 0;
}
//
// combine all tringles into polygons where applicable


void offset3D_RemoveColinear(const std::vector<Vector3d> &vertices, std::vector<IndexedFace> &indices, std::vector<intList> &pointToFaceInds, std::vector<intList> &pointToFacePos)
{
	
	// ----------------------------------------------------------
	// create a point database and use it. and form polygons
	// ----------------------------------------------------------
	pointToFaceInds.clear();
	pointToFacePos.clear();
	intList emptyList;
	for(unsigned int i=0;i<vertices.size();i++) {
		pointToFaceInds.push_back(emptyList);
		pointToFacePos.push_back(emptyList);
	}

	// -------------------------------
	// calculate point-to-polygon relation
	// -------------------------------
	for(unsigned int i=0;i<indices.size();i++) {
		IndexedFace pol = indices[i];
		for(unsigned int j=0;j<pol.size(); j++) {
			pointToFaceInds[pol[j]].push_back(i);
			pointToFacePos[pol[j]].push_back(j);
		}
	}
	
	for(unsigned int i=0;i<pointToFaceInds.size();i++) {
		const auto &index = pointToFaceInds[i];
		if(index.size() != 2) continue; // only works with 2 point uses
		int valid=1;
		for(int j=0;valid && j<2;j++) {
			const IndexedFace &face = indices[index[j]];
			int n=face.size();
			int pos = pointToFacePos[i][j];

			const Vector3d &prev =  vertices[face[(pos+n-1)%n]];
			const Vector3d &cur =   vertices[face[pos%n]];
			const Vector3d &next =  vertices[face[(pos+1)%n]];
			Vector3d d1=cur-prev;
			Vector3d d2=next-cur;
			if(d1.norm() < 0.001) continue; 
			if(d2.norm() < 0.001) continue;
			d1.normalize();
			d2.normalize();
			if(d1.dot(d2) < 0.999) valid=0;
		}
		if(valid){
			for(int j=0; j<2;j++) {
				int faceind=index[j];
				int pointpos = pointToFacePos[i][j];
				IndexedFace &face = indices[faceind];
				face.erase(face.begin()+pointpos);
				// db anpassen
				for(unsigned int k=0;k<pointToFaceInds.size();k++) {
					for(unsigned int l=0;l<pointToFaceInds[k].size();l++) {
						if(pointToFaceInds[k][l] != faceind) continue;
						if(pointToFacePos[k][l] > pointpos) pointToFacePos[k][l]--;
						else if(pointToFacePos[k][l] == pointpos) {
							pointToFaceInds[k].erase(pointToFaceInds[k].begin()+l);
							pointToFacePos[k].erase(pointToFacePos[k].begin()+l);
							l--;
						}

					}
				}

			}
		}
	}
}

void offset_3D_dump(const std::vector<Vector3d> &vertices, const std::vector<IndexedFace> &indices)
{
	printf("Vertices:%ld Indices:%ld\n",vertices.size(),indices.size());
	for(unsigned int i=0;i<vertices.size();i++)
	{
		printf("%d: \t%g/\t%g/\t%g\n",i,vertices[i][0], vertices[i][1] ,vertices[i][2]);
	}
	printf("===========\n");
	for(unsigned int i=0;i<indices.size();i++)
	{
		auto &face=indices[i];
		printf("%d :",i);
		for(unsigned int j=0;j<face.size();j++)
			printf("%d ",face[j]);
		printf("\n");
	}
}
double offset3D_angle(const Vector3d &refdir, const Vector3d &edgedir, const Vector3d &facenorm)
{
	double c,s;
	Vector3d tmp=refdir.cross(edgedir);
	c=refdir.dot(edgedir);
	s=tmp.norm();
	if(tmp.dot(facenorm) < 0) s=-s;
	double ang=atan2(s,c)*180/3.1415926;
	if(ang < -1e-9) ang += 360;
	return ang;
}

void offset3D_calculateNefInteract(const std::vector<Vector4d> &faces, std::vector<IndexedFace> &faceinds,int selfind,  int newind) {
//	printf("Interact selfind is %d, newind=%d\n=====================\n",selfind, newind);
	if(faces[selfind].head<3>().dot(faces[newind].head<3>()) < -0.99999) return;
	if(faceinds[selfind].size() == 0) {
		faceinds[selfind].push_back(newind); 
		return;
	}

	// calculate the angles of the cuts and find out position of newind
	// calculate refedge
	double angdiff;
	Vector3d facenorm=faces[selfind].head<3>();
	Vector3d refdir=facenorm.cross(faces[faceinds[selfind][0]].head<3>()).normalized();
//	printf("norm is %g/%g/%g\n",facenorm[0], facenorm[1], facenorm[2]);
//	printf("refdir is %g/%g/%g\n",refdir[0], refdir[1], refdir[2]);
	// TODO angles nicht neu berechnen, sondern uebernehmen
	Vector3d edgedir;
	double angle;
	std::vector<double> angles;
	int n=faceinds[selfind].size();
	for(int j=0;j<n;j++) {
		edgedir=facenorm.cross(faces[faceinds[selfind][j]].head<3>()).normalized();
		angle=offset3D_angle(refdir, edgedir, facenorm);
		angles.push_back(angle);
	}
	edgedir=facenorm.cross(faces[newind].head<3>()).normalized();
	angle=offset3D_angle(refdir, edgedir, facenorm);

	Vector3d d1, p1, d2, p2, d3, p3, cutpt;

	d1=faces[selfind].head<3>();
	p1=d1*faces[selfind][3];
	d3=faces[faceinds[selfind][n-1]].head<3>(); 
	p3=d2*faces[faceinds[selfind][n-1]][3];

	// now insert newind in the right place
	IndexedFace faceindsnew;
	for(int j=0;j<n;j++) {
		p2=p3;
		d2=d3;
		d3=faces[faceinds[selfind][j]].head<3>();
		p3=d3*faces[faceinds[selfind][j]][3];
		if(fabs(angle-angles[j]) < 0.001) {
			// wer schneidet mehr ein: faceinds[selfind][j]  oder newind
			// Testpunkt ist punkt auf selfind
			Vector3d testpt=faces[selfind].head<3>() * faces[selfind][3];
			int presind=faceinds[selfind][j];
			double pres_dist = testpt.dot(faces[presind].head<3>())-faces[presind][3];
			double new_dist = testpt.dot(faces[newind].head<3>())-faces[newind][3];
			if(pres_dist > new_dist) { angle=1e9; } // keep it, never insert it
			else { faceindsnew.push_back(newind);  angles[j]=angle; angle=1e9; continue; } // insert new one instead							    
		} else if(angle < angles[j]) {
			int jp=(j+n-1)%n;
			bool valid=true;
			double angdiff=angles[j]-angles[jp];
			if(angdiff < 0) angdiff += 360;
			if(angdiff < 180) {
				if(cut_face_face_face( p1, d1, p2, d2, p3, d3, cutpt)) printf("Problem during cut face_face_face %d/%d/%d!\n",selfind, faceinds[selfind][j], faceinds[selfind][jp]);
				else {
					double off=cutpt.dot(faces[newind].head<3>())-faces[newind][3];
					if(off < 1e-6) valid=false; 
				}
			}
			if(valid) {
				faceindsnew.push_back(newind); 
				angles.insert(angles.begin()+j,angle);
			}
			angle=1e9;
		}
		faceindsnew.push_back(faceinds[selfind][j]);
	}
	if(angle < 1e9){
		bool valid=true;
		do {
			if(n < 2) break;
			angdiff=angles[0]-angles[n-1];
			if(angdiff < 0) angdiff += 360;
			if(angdiff > 180)  break;
			d2=d3; 
			p2=p3; // TODO code doppelt mit oben
			d3=faces[faceinds[selfind][0]].head<3>();
			p3=d3*faces[faceinds[selfind][0]][3];
			if(cut_face_face_face( p1, d1, p2, d2, p3, d3, cutpt))
			{
				printf("Problem during cut face_face_face %d/%d/%d!\n",selfind, faceinds[selfind][angles.size()-1], faceinds[selfind][0]);
				break;
			}
			double off=cutpt.dot(faces[newind].head<3>())-faces[newind][3];
			if(off < 0) valid=false;
		}
		while(0);		
		if(valid) {
			faceindsnew.push_back(newind);
			angles.push_back(angle);
		}
	}					     
	n=faceindsnew.size();
	// TODO newind richti speichern
	int insertpos=-1;
	for(unsigned int i=0;i<faceindsnew.size();i++)
		if(faceindsnew[i] == newind) insertpos=i;
	if(insertpos != -1 ) {
		while(n >= 3) {
			int ind1pos=(insertpos+1)%n;
			int ind2pos=(insertpos+2)%n;
			angdiff=angles[ind2pos]-angles[insertpos];
			if(angdiff < 0) angdiff += 360;
			if(angdiff > 180) break;
			d2=faces[faceindsnew[ind1pos]].head<3>();
			p2=d2*faces[faceindsnew[ind1pos]][3];
			d3=faces[faceindsnew[ind2pos]].head<3>();
			p3=d3*faces[faceindsnew[ind2pos]][3];
			if(cut_face_face_face( p1, d1, p2, d2, p3, d3, cutpt)) printf("Problem during cut!\n");
			double off=cutpt.dot(faces[newind].head<3>())-faces[newind][3];
			if(off > -1e-6) {
				faceindsnew.erase(faceindsnew.begin()+ind1pos);
				angles.erase(angles.begin()+ind1pos);
				if(ind1pos < insertpos) insertpos--;	
				n--;
			} else break;
		};
		while(n >= 3) {
			int ind1pos=(insertpos+n-1)%n;
			int ind2pos=(insertpos+n-2)%n;
			angdiff=angles[insertpos]-angles[ind2pos];
			if(angdiff < 0) angdiff += 360;
			if(angdiff > 180) break;
			d2=faces[faceindsnew[ind1pos]].head<3>();
			p2=d2*faces[faceindsnew[ind1pos]][3];
			d3=faces[faceindsnew[ind2pos]].head<3>();
			p3=d3*faces[faceindsnew[ind2pos]][3];
			if(cut_face_face_face( p1, d1, p2, d2, p3, d3, cutpt)) printf("Problem during cut 3!\n");
			double off=cutpt.dot(faces[newind].head<3>())-faces[newind][3];
			if(off > -1e-6) {
				faceindsnew.erase(faceindsnew.begin()+ind1pos); 
				angles.erase(angles.begin()+ind1pos);
				if(ind1pos < insertpos) insertpos--;	
				n--;
			} else break;
		};
	}
	faceinds[selfind] = faceindsnew;
}


void offset3D_calculateNefPolyhedron(const std::vector<Vector4d> &faces,std::vector<Vector3d> &vertices, std::vector<IndexedFace> & indices)
{
	unsigned int i;
	std::vector<IndexedFace> nef_db;
	// a nef contains an entry for each face showing the correct order of the surrounding edges
	// each with everybody
	for(i=0;i<faces.size();i++) {
		// add each single face in  a sequence to the database;
		int orgsize=nef_db.size();
		IndexedFace new_face;
		nef_db.push_back(new_face);
		for(int j=0;j<orgsize;j++) {
			offset3D_calculateNefInteract(faces, nef_db,j, i);
			offset3D_calculateNefInteract(faces, nef_db, orgsize,j);
		}
	}
	// Now dump the NEF db
	printf("NEF DB\n");
	for(i=0;i<nef_db.size();i++)
	{
		printf("Face %d: ",i);
		for(unsigned int j=0;j<nef_db[i].size();j++)
			printf("%d ",nef_db[i][j]);
		printf("\n");
	}

	// synthesize the db and form polygons
  	Reindexer<Vector3d> vertices_;
	for(unsigned int i=0;i<nef_db.size();i++) {
		IndexedFace face;
		IndexedFace &db=nef_db[i];
		Vector3d d1=faces[i].head<3>();
		Vector3d p1=d1*faces[i][3];
		for(unsigned int j=0;j<db.size();j++)
		{
			int k=(j+1)%db.size();
			Vector3d d2=faces[db[j]].head<3>();
			Vector3d p2=d2*faces[db[j]][3];

			Vector3d d3=faces[db[k]].head<3>();
			Vector3d p3=d3*faces[db[k]][3];
			// cut face i, db[j], db[j+1]
			Vector3d newpt;
			if(cut_face_face_face( p1, d1, p2, d2, p3, d3, newpt)) printf("Problem during cut!\n");
//			printf("newpt is %g/%g/%g\n",newpt[0], newpt[1], newpt[2]);
      			face.push_back(vertices_.lookup(newpt));
		}
		indices.push_back(face);
	}
	vertices_.copy(std::back_inserter(vertices));
}
#include <CGAL/Exact_integer.h>
#include <CGAL/Extended_homogeneous.h>
#include <CGAL/Nef_polyhedron_3.h>
#include "CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h"
//#include <cassert>
typedef CGAL::Extended_homogeneous<CGAL::Exact_integer>  Kernel;
//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;

typedef CGAL::Nef_polyhedron_3<Kernel>  Nef_polyhedron;
typedef Kernel::Plane_3  Plane_3;
typedef Kernel::Point_3                                     Point_3;
typedef CGAL::Surface_mesh<Point_3> Surface_mesh;


void offset3D_calculateNefPolyhedron_cgal(const std::vector<Vector4d> &faces,std::vector<Vector3d> &vertices, std::vector<IndexedFace> & indices) {
  Nef_polyhedron N1(Plane_3( 1, 0, 0,-1));
  Nef_polyhedron N2(Plane_3(-1, 0, 0,-1));
  Nef_polyhedron N3(Plane_3( 0, 1, 0,-1));
  Nef_polyhedron N4(Plane_3( 0,-1, 0,-1));
  Nef_polyhedron N5(Plane_3( 0, 0, 1,-1));
  Nef_polyhedron N6(Plane_3( 0, 0,-1,-1));
  Nef_polyhedron Cube = N1 * N2 * N3 * N4 * N5 * N6;
  printf("y\n");
  Surface_mesh resultMesh;	
  CGAL::convert_nef_polyhedron_to_polygon_mesh (Cube, resultMesh, true);

  vertices.clear();
 for (auto  vd : resultMesh.vertices()){
    const auto &v = resultMesh.point(vd);
    Vector3d pt(
    	CGAL::to_double(v.x()),
    	CGAL::to_double(v.y()),
    	CGAL::to_double(v.z()));
    vertices.push_back(pt);
    printf("%g %g %g\n",pt[0], pt[1], pt[2]);
  }

  for (const auto& f : resultMesh.faces()) {
    IndexedFace fi;	  
    for (auto vd : vertices_around_face(resultMesh.halfedge(f), resultMesh)) {
      fi.push_back(vd);	    
      printf("%d ",(int) vd);	    
    }
    indices.push_back(fi);
    printf("\n");
  }

}

std::vector<std::shared_ptr<const PolySet>>  offset3D_decompose(std::shared_ptr<const PolySet> ps)
{
	std::vector<std::shared_ptr<const PolySet>> results;
	if(ps->indices.size() == 0) return results;


	std::unordered_set<int> faces_included;
	//
	// edge db aufbauen
	std::unordered_map<TriCombineStub, int, boost::hash<TriCombineStub> > edge_db; // edge -> face
	TriCombineStub stub;							
	for(unsigned int i=0;i<ps->indices.size();i++) {
		auto &face = ps->indices[i];
		int n=face.size();
		for(int j=0;j<n;j++) {
			stub.ind1=face[j];
			stub.ind2=face[(j+1)%n];
			edge_db[stub]=i;
		}
	}
//	for(int i=0;i<ps->indices.size();i++) {
//		auto &face = ps->indices[i];
//		bool valid=true;
//		for(int j=0;valid && j<face.size();j++) {
//			if(fabs(ps->vertices[face[j]][1]-0.0) > 1e-3) valid=0; // all y coords must be 0
//		}
//		if(valid) {
//			printf("Face %d is interesting\n",i);
//		}
//
//	}

//	int count=0;
	while(1) {
		int newfaceind=-1;
		std::vector<int> faces_done; // for one result
		std::vector<int> faces_convex; // for one result
		std::vector<Vector4d> faces_norm;
		std::vector<int> faces_todo_face;
		std::vector<int> faces_todo_edge;
		for(unsigned int i=0;newfaceind == -1 && i<ps->indices.size();i++)
		{
			if(faces_included.count(i) == 0) {
				printf("New Round,Chosen to start with %d\n",i);
				newfaceind=i;
			}
		}
		if(newfaceind == -1) {
			printf("%ld results\n",results.size());
			return results;
		}

		while(faces_todo_face.size() > 0 || newfaceind != -1) {
			bool valid=false;
			if(newfaceind != -1){
				valid=true;
		       	}
			else
			{
				newfaceind=-1;
				int faceind=faces_todo_face[0];
				int edgeind=faces_todo_edge[0];
//				printf("Doing Face %d/%d | %d in queue \n",faceind, edgeind,faces_todo_face.size());
				//
				//which is the opposite face
				int n = ps->indices[faceind].size();
				stub.ind1=ps->indices[faceind][(edgeind+1)%n];	 
				stub.ind2=ps->indices[faceind][edgeind];	
				faces_todo_face.erase(faces_todo_face.begin());
				faces_todo_edge.erase(faces_todo_edge.begin());
				if(edge_db.count(stub) == 0) continue;
				newfaceind=edge_db[stub];

				if(find(faces_done.begin(), faces_done.end(), newfaceind) != faces_done.end()) { newfaceind=-1;  continue; }
				
				// check if all pts of opp+_ind are below all planes
				auto &newface = ps->indices[newfaceind]; // neue face die vielleicht dazu kommt
				int no = newface.size();
//				printf("Checking face %d :%d vertices against %d faces\n",newfaceind, no, faces_norm.size());
	
				// new face is valid if any of its points is will inside all existing faces
				valid=false; 
				Vector3d centerpt(0,0,0);
				for(int i=0;i<no;i++) centerpt += ps->vertices[newface[i]];
				centerpt /= no;

				for(int i=0;!valid &&  i<no;i++) {
					Vector3d pt=ps->vertices[newface[i]];
					bool valid1=true;
					for(unsigned int j=0;valid1 && j<faces_norm.size();j++) {
						double off=pt.dot(faces_norm[j].head<3>())-faces_norm[j][3];
						if(off > 1e-3) {
							valid1=false;
//							printf(" not valid for pt %d, norm= %g/%g/%g/%g\n",j, faces_norm[j][0],faces_norm[j][1],faces_norm[j][2], faces_norm[j][3]);
						}
						else if(off > -1e-3) {
							if((centerpt-pt).dot(faces_norm[j].head<3>()) > 0) valid1=false;
						}
					}
					if(valid1) valid=true;
				}
			}
			faces_done.push_back(newfaceind);
//			printf("face is %d, valid = %d registering new faces\n", newfaceind,valid);
			for(unsigned int i=0;i<ps->indices[newfaceind].size();i++) {
				faces_todo_face.push_back(newfaceind);
				faces_todo_edge.push_back(i);
			}
			if(valid){
				int n=faces_convex.size();
				Vector4d newface_norm = calcTriangleNormal(ps->vertices, ps->indices[newfaceind]);
				for(int i=0;i<n;i++) {
					auto &tri = ps->indices[faces_convex[i]]; // bestehednde
					// if tri is completely outside of new tri, skip it
					bool valid1=true;
					for(unsigned int j=0;j<tri.size();j++) {
						Vector3d pt = ps->vertices[tri[j]];
						double off=pt.dot(newface_norm.head<3>())-newface_norm[3];
						if(off < 1e-3) valid1=false;
					}
					if(valid1) {
//						printf("Kicking out %d\n",faces_convex[i]);
//						printf("agressor is %g/%g/%g/%g norm ind is %d\n",newface_norm[0],newface_norm[1],newface_norm[2],newface_norm[3],i);
						faces_norm.erase(faces_norm.begin()+i);
						faces_convex.erase(faces_convex.begin()+i);
						i--;
						n--;
					}
				}
//				printf("adding face  %g/%g/%g/%g as ind %d\n",newface_norm[0],newface_norm[1],newface_norm[2],newface_norm[3],faces_norm.size());
				faces_norm.push_back(newface_norm);
				faces_convex.push_back(newfaceind);
			}
			newfaceind=-1;	
		}
	
		std::vector<IndexedFace> result;
		printf("Result ");
		for(unsigned int i=0;i<faces_convex.size();i++)
		{
			result.push_back(ps->indices[faces_convex[i]]);
			printf("%d ",faces_convex[i]);
		}
		printf("\n");
		std::vector<Vector4d> normals;
		std::vector<Vector4d> faces_normals;
		for(unsigned int i=0;i<faces_convex.size();i++)
		{
			Vector4d norm = calcTriangleNormal(ps->vertices, ps->indices[faces_convex[i]]);
//			printf("r norm is %g/%g/%g/%g\n",norm[0], norm[1],norm[2], norm[3]);
			faces_normals.push_back(norm);
			unsigned int j;
			for(j=0;j<normals.size();j++) {
				if(normals[j].head<3>().dot(norm.head<3>()) > 0.999){
				       	if(faces_included.count(faces_convex[i]) == 0) { normals[j][3]=norm[3]; }
					break;
				}
			}
			if(j == normals.size()) normals.push_back(norm);
		}
		
		for(unsigned int i=0;i<faces_normals.size();i++) {
//			printf("Checking face %d\n",faces_convex[i]);
			for(unsigned int j=0;j<normals.size();j++) {
				if(normals[j].head<3>().dot(faces_normals[i].head<3>()) > 0.999
					&& fabs(faces_normals[i][3] - normals[j][3]) < 0.001){
					faces_included.insert(faces_convex[i]);
//					printf("include\n");
					break;
				}
			}
		}
		//
		printf("Normals\n");
		for(unsigned int i=0;i<normals.size();i++)
		{
			printf("%g/%g/%g/%g\n",normals[i][0],normals[i][1], normals[i][2], normals[i][3]);
		}


		std::vector<Vector3d> vertices1;
		std::vector<IndexedFace> indices1;
		offset3D_calculateNefPolyhedron(normals, vertices1, indices1);					
//		offset3D_calculateNefPolyhedron_cgal(normals, vertices1, indices1,debug);					

		PolySet *decomposed =  new PolySet(3,  true);
		decomposed->vertices = vertices1;
		decomposed->indices = indices1;
		results.push_back( std::shared_ptr<const PolySet>(decomposed));
//		printf("========================\n");
//		printf("Faces included size is %ld\n",faces_included.size());
	}
	return results;
}

extern std::vector<SelectedObject> debug_pts;
std::vector<IndexedFace>  offset3D_removeOverPoints(const std::vector<Vector3d> &vertices, std::vector<IndexedFace> indices,int  round)
{
	// go  thourh all indicices and calculate area
//	printf("Remove OverlapPoints\n");
//	debug_pts.clear();

	std::vector<IndexedFace> indicesNew;
	indicesNew.clear();
	std::vector<int> mapping;
	std::vector<int> uselist;
	for(unsigned int i=0;i<vertices.size();i++) mapping.push_back(i);
	for(unsigned int i=0;i<indices.size();i++) {
		auto &face=indices[i];
		IndexedFace facenew;
		int n=face.size();
		if(n < 3) continue;

		// ---------------------------------
		// skip overlapping vertices
		// ---------------------------------
		int curind=face[0];
		for(int j=0;j<n;j++) {
			int nextind=face[(j+1)%n];
			Vector3d d=vertices[nextind]-vertices[curind];
			if( d.norm() > 3e-3 ){
				curind=nextind;
				facenew.push_back(curind);
			} else {
				int newval=curind;
				while(newval != mapping[newval]) newval=mapping[newval];
				mapping[nextind]=newval;
			}
		}
	}

	for(unsigned int i=0;i<indices.size();i++) {
		auto &face=indices[i];
		IndexedFace facenew;
		int  indold=-1;
		for(unsigned int j=0;j<face.size();j++) {
			int ind=mapping[face[j]];
			if(ind == indold) continue;
			if(facenew.size() > 0 && ind == facenew[0]) continue; 
			facenew.push_back(ind);
			indold=	ind;
		}
		if(facenew.size() >= 3) indicesNew.push_back(facenew);
	}

	return indicesNew;
}


void offset3D_displayang(Vector3d dir){
	double pl=sqrt(dir[0]*dir[0]+dir[1]*dir[1]);
	printf("ang=%g elev=%g ",atan2(dir[1], dir[0])*180.0/3.14,atan2(dir[2],pl)*180.0/3.14);
}
double offset3D_area(Vector3d p1, Vector3d p2, Vector3d p3, Vector4d n) {
//	double l1=(p2-p3).norm();
//	double l2=(p2-p1).norm();
//	double ang=acos((p2-p3).dot(p2-p1)/(l1*l2))*180/3.14;


	Vector3d c = (p2-p3).cross(p2-p1);
	double l=c.norm();
	if(c.dot(n.head<3>()) < 0) l=-l;
	return l;
}

std::vector<Vector3d> offset3D_offset(std::vector<Vector3d> vertices, std::vector<IndexedFace> &indices,const std::vector<Vector4d> &faceNormal, std::vector<intList> &pointToFaceInds, std::vector<intList> &pointToFacePos, double &off, int round) // off always contains the value, which is still to be sized
{														    
//	offset_3D_dump(vertices, indices);
	// -------------------------------
	// now calc length of all edges
	// -------------------------------
	std::vector<double> edge_len;
	double edge_len_min=0;
	for(unsigned int i=0;i<indices.size();i++)
	{
		auto &pol = indices[i];
		int n=pol.size();
		for(int j=0;j<n;j++){
			int a=pol[j];
			int b=pol[(j+1)%n];
			if(b > a) {
				double dist=(vertices[b]-vertices[a]).norm();
				edge_len.push_back(dist);
				if(edge_len_min == 0 || edge_len_min > dist) edge_len_min=dist;
			}
		}
	}
	double off_act=off;
	if(off_act < 0) off_act=-edge_len_min/5.0; // TODO ist das sicher ?
	std::vector<Vector3d> verticesNew=vertices;
	std::map<int, intList> cornerFaces;
	for(unsigned int i=0;i<pointToFaceInds.size();i++) { // go through all vertices

		// -------------------------------
		// Find where several solid corners  share a single vertex
		// -------------------------------
		const intList &db=pointToFaceInds[i];		
		// now build chains
		//
		std::unordered_map<int, TriCombineStub> stubs;
		TriCombineStub s;
		// build corner edge database
//		printf("Vertex %d faces is %d\n",i,db.size());	
		for(unsigned int j=0;j<db.size();j++) {					   
			const IndexedFace &pol = indices[db[j]];
			int n=pol.size();
			int pos=pointToFacePos[i][j];
			int prev=pol[(pos+n-1)%n];
			int next=pol[(pos+1)%n];
			s.ind1=next; 
			s.ind2=db[j];
			s.ind3=pos;
			if(prev != next) stubs[prev]=s;
//			printf("%d -> %d\n",prev, next);
			// stubs are vertind -> nextvertind, faceind
		}
		std::vector<intList> polinds, polposs;
		std::unordered_set<int> vertex_visited;

		for(  auto it=stubs.begin();it != stubs.end();it++ ) {
			// using next available
			int ind=it->first;
			if(vertex_visited.count(ind) > 0) continue;
			int begind= ind;
			intList polind;
			intList polpos;
			do
			{
				polind.push_back(stubs[ind].ind2);
				polpos.push_back(stubs[ind].ind3);
				int indnew=stubs[ind].ind1;
				vertex_visited.insert(ind);
				if(ind == indnew) {
					printf("Programn error, loop! %d\n",ind); // TODO keep to catch a bug
					exit(1);
				}
				ind=indnew;
			} while(ind != begind);
			polinds.push_back(polind);
			polposs.push_back(polpos);
		}
//		printf("polinds size is: %d\n",polinds.size());
//		if(db.size() < 6) continue; // not possible  with less than 6 faces

		if(polinds.size() > 1) {
			for(unsigned int j=0;j<polinds.size();j++) { // 1st does not need treatment
				int vertind;
				if(j > 0) {
					vertind=vertices.size();
					vertices.push_back(vertices[i]);
					verticesNew.push_back(vertices[i]);
					pointToFaceInds.push_back(polinds[j]);
					pointToFacePos.push_back(polposs[j]);
					auto &polind =polinds[j];
					for(unsigned int k=0;k<polind.size();k++){
						IndexedFace &face = indices[polind[k]];
						for(unsigned int l=0;l<face.size();l++) {
							if(face[l] == (int) i) face[l]=vertind;
						}
					}
				} else {
					vertind=i;
					pointToFaceInds[vertind]=polinds[j];
					pointToFacePos[vertind]=polposs[j];
				}
				cornerFaces[vertind] = polinds[j];
			}
		} 
		else if(polinds.size() == 1) cornerFaces[i] = polinds[0];
	} 
	
	std::vector<TriCombineStub> keile;
	for( unsigned int i = 0; i< pointToFaceInds.size();i++ ) {

		// ---------------------------------------
		// Calculate offset position to each point
		// ---------------------------------------

		Vector3d newpt;
		intList faceInds = pointToFaceInds[i];
//		int valid;

		Vector3d oldpt=vertices[i];

		if(faceInds.size() >= 4) {
			if(cornerFaces.count(i) > 0) {
				auto &face_order = cornerFaces[i]; // alle faceinds rund um eine ecke
							   //
				unsigned int n=face_order.size();
				// create face_vpos
				std::vector<int> face_vpos; // TODO ist diese info schon verfuegbar ?
				for(unsigned int j=0;j<n;j++) {
					int pos=-1;
					auto &face= indices[face_order[j]];
					for(unsigned int k=0;k<face.size();k++)
						if(face[k] == i) pos=(int) k;
					assert(pos != -1);
					face_vpos.push_back(pos);
				}				
	
//				printf("Special alg for Vertex %d\n",i);
	
				IndexedFace mainfiller; // core of star

				int newind=verticesNew.size();
				
				for(unsigned int j=0;j<n;j++) {
					vertices.push_back(oldpt);
					verticesNew.push_back(oldpt);
				}
				std::vector<int> faces1best, faces2best;
				for(unsigned int j=0;j<n;j++) {
					int ind1, ind2, ind3;
					ind1=face_order[j];
					Vector3d xdir=faceNormal[ind1].head<3>();

					auto &face =indices[face_order[j]];  // alle punkte einer flaeche
					int nf=face.size();
					int pos=-1;
					for(int k=0;k<nf;k++)
						if(face[k] == i) pos=k;
					assert(pos != -1);

					double minarea=1e9;
					Vector3d bestpt;
					int face1best=-1, face2best=-1;
					for(unsigned int k=0;k<n;k++) {
						if(face_order[k] == ind1) continue;
						ind2=face_order[k]; 
						Vector3d ydir=faceNormal[ind2].head<3>();

						for(unsigned int l=0;l<n;l++) {

							if(face_order[l] == ind1) continue;
							if(face_order[l] == ind2) continue;
							ind3 = face_order[l];
							Vector3d zdir=faceNormal[ind3].head<3>();

							if(!cut_face_face_face( oldpt  +xdir*off_act  , xdir, oldpt  +ydir*off_act  , ydir, oldpt  +zdir*off_act  , zdir, newpt)){
								double area = offset3D_area(vertices[face[(pos+n-1)%nf]],newpt,vertices[face[(pos+1)%nf]],faceNormal[face_order[j]]); 
								if(area < minarea) {
									minarea = area;
									bestpt = newpt;
									face1best=face_order[k];
									face2best=face_order[l];
								}
							}
						}
					}
//					printf("bestpt is %g/%g/%g\n",bestpt[0], bestpt[1], bestpt[2]);
					mainfiller.insert(mainfiller.begin(),newind+j); // insert in reversed order
					verticesNew[newind+j]=bestpt;
					faces1best.push_back(face1best);
					faces2best.push_back(face2best);
				}
				Vector3d area(0,0,0);
			 	for(unsigned int j=0;j<mainfiller.size()-2;j++) {
					Vector3d diff1=(verticesNew[mainfiller[0]] - verticesNew[mainfiller[j+1]]);
					Vector3d diff2=(verticesNew[mainfiller[j+1]] - verticesNew[mainfiller[j+2]]);
					area += diff1.cross(diff2);
				}
//				printf("area is %g\n",area.norm());
				if(area.norm()  > 1e-6) { // only if points are actually diverging

					for(unsigned int j=0;j<face_order.size();j++) {
						auto &face= indices[face_order[j]];
						face[face_vpos[j]]=newind + j;
					}
	
					// insert missing triangles
					indices.push_back(mainfiller);
					for(unsigned int j=0;j<face_order.size();j++){
						auto &tmpface = indices[face_order[j]]; 
						int n1=tmpface.size();
			        		int commonpt= tmpface[(face_vpos[j]+1)%n1]; 
			
						IndexedFace filler;
						filler.push_back(newind+j);
						filler.push_back(newind+((j+1)%face_order.size()));
						filler.push_back(commonpt);
	
						TriCombineStub stub;
						stub.ind1=indices.size(); // index of stub
						stub.ind2=faces1best[j];
						stub.ind3=faces2best[j];;
						keile.push_back(stub);
						indices.push_back(filler);
					}
				} else 	verticesNew[i]=verticesNew[mainfiller[0]];
				continue;
			}
		} else if(faceInds.size() >= 3) {
			Vector3d dir0=faceNormal[faceInds[0]].head<3>();
			Vector3d dir1=faceNormal[faceInds[1]].head<3>();
			Vector3d dir2=faceNormal[faceInds[2]].head<3>();
			if(cut_face_face_face( oldpt  +dir0*off_act  , dir0, oldpt  +dir1*off_act  , dir1, oldpt  +dir2*off_act  , dir2, newpt)) { printf("problem during cut\n");  break; }
			verticesNew[i] = newpt;
			continue;
		} else if(faceInds.size() == 2) {
			Vector3d dir= faceNormal[faceInds[0]].head<3>() +faceNormal[faceInds[1]].head<3>() ;
			verticesNew[i] = vertices[i] + off_act* dir;
			continue;
		} else if(faceInds.size() == 1) {
			Vector3d dir= faceNormal[faceInds[0]].head<3>();
			verticesNew[i] = vertices[i] + off_act* dir;
			continue;
		}
	}
//	printf("%ld Keile found\n",keile.size());
	for(unsigned int i=0;i<keile.size();i++) {
		int faceind=keile[i].ind1;
		printf("keil faceind is %d\n",faceind);
		Vector3d d1=( verticesNew[indices[faceind][1]]-verticesNew[indices[faceind][0]]).normalized();
		Vector3d d2=( verticesNew[indices[faceind][1]]-verticesNew[indices[faceind][2]]).normalized();
		if(d1.cross(d2).norm() < 1e-6) { // TODO jeden keil mergen, nur muss dann die normale passen
			printf("Keil %dZero area others are %d %d\n", i,keile[i].ind2, keile[i].ind3);
			std::vector<IndexedFace> output;
			{
				std::vector<IndexedFace> input;
				input.push_back(indices[keile[i].ind2]);
				input.push_back(indices[keile[i].ind1]);
				output = mergeTrianglesSub(input, verticesNew);
			}
			if(output.size() == 1) {
				printf("Successful merge #1 %d -> %d\n",keile[i].ind1, keile[i].ind2);
				indices[keile[i].ind2]=output[0];
				indices[keile[i].ind1].clear();
				continue;
			}
			{
				std::vector<IndexedFace> input;
				input.push_back(indices[keile[i].ind3]);
				input.push_back(indices[keile[i].ind1]);
				output = mergeTrianglesSub(input, verticesNew);
			}
			if(output.size() == 1) {
				printf("Successful merge #2 %d -> %d\n",keile[i].ind1, keile[i].ind3);
				indices[keile[i].ind3]=output[0];
				indices[keile[i].ind1].clear();
				continue;
			}
			// now tryu to merge 
		}
	}
	// -------------------------------
	// now calc new length of all new edges
	// -------------------------------
	double off_max=-1e9;
	int cnt=0;
	for(unsigned int i=0;i<indices.size();i++)
	{
		auto &pol = indices[i];
		int n=pol.size();
		for(int j=0;j<n;j++){
			int a=pol[j];
			int b=pol[(j+1)%n];
			if(b > a) {
				double dist=(verticesNew[b]-verticesNew[a]).norm();
				double fact = (dist-edge_len[cnt])/off_act;
				if(fabs(edge_len[cnt]-dist) > 1e-5) {
					double off_max_sub = off_act/(edge_len[cnt]-dist);
					if(off_max_sub > off_max && off_max_sub < 0) off_max = off_max_sub;
				}
				cnt++;
			}
		}
	}

	double off_do=off;
	if(off_do <  off_max) off_do=off_max;
	for(unsigned int i=0;i<verticesNew.size();i++) {
		Vector3d d=(verticesNew[i]-vertices[i])*off_do/off_act;
		verticesNew[i]=vertices[i]+d;
	}			
	off -= off_do;
	return verticesNew;
}

#define NEW_REINDEX

void  offset3D_reindex(const std::vector<Vector3d> &vertices, std::vector<IndexedFace> & indices, std::vector<Vector3d> &verticesNew, std::vector<IndexedFace> &indicesNew)
{
  indicesNew.clear();
  verticesNew.clear();  
#ifndef NEW_REINDEX  
  Reindexer<Vector3d> vertices_;
#endif  
  for(unsigned int i=0;i<indices.size();i++) {
    auto face= indices[i];		
    if(face.size() < 3) continue;
    Vector3d diff1=vertices[face[1]] - vertices[face[0]].normalized();
    Vector3d diff2=vertices[face[2]] - vertices[face[1]].normalized();
    Vector3d norm = diff1.cross(diff2);
    if(norm.norm() < 0.0001) continue;
    IndexedFace facenew;
    for(unsigned int j=0;j<face.size();j++) {
      const Vector3d &pt=vertices[face[j]];	    
#ifdef NEW_REINDEX
    int ind=-1;	    // TODO besser alg
    for(unsigned int k=0;k<verticesNew.size();k++) {
      if((verticesNew[k]-pt).norm() <1e-6) {
	      ind=k;
      	      break;
      }
    }	    
    if(ind == -1) {
      ind=verticesNew.size();
      verticesNew.push_back(pt);      
    }
#else	    
      int ind=vertices_.lookup(pt);
#endif      
      if(facenew.size() > 0){
        if(facenew[0] == ind) continue;
        if(facenew[facenew.size()-1] == ind) continue;
      }
      facenew.push_back(ind);
    }
    if(facenew.size() >= 3) indicesNew.push_back(facenew);
  }
#ifndef NEW_REINDEX
  vertices_.copy(std::back_inserter(verticesNew));
#endif  
}

std::shared_ptr<const Geometry> offset3D_convex(const std::shared_ptr<const PolySet> &ps,double off) {
//  printf("Running offset3D %ld polygons\n",ps->indices.size());
//  if(off == 0) return ps;
  std::vector<Vector3d> verticesNew;
  std::vector<IndexedFace> indicesNew;
  std::vector<Vector4d> normals;
  std::vector<intList>  pointToFaceInds, pointToFacePos;
  if(off > 0 && 0) { // upsize
    std::vector<std::shared_ptr<const PolySet>> decomposed =  offset3D_decompose(ps);
    printf("%ld decompose results\n",decomposed.size());
    //
    std::shared_ptr<ManifoldGeometry> geom = nullptr;
    for(unsigned int i=0;i<decomposed.size();i++) {													   
      auto &ps = decomposed[i];	  

      printf("Remove OverlapPoints\n");
      std::vector<IndexedFace> indicesNew = offset3D_removeOverPoints(ps->vertices, ps->indices,0);

      printf("Calc Normals\n");
      std::vector<Vector4d>  faceNormal=calcTriangleNormals(ps->vertices, indicesNew);

      printf("Merge Triangles %ld %ld\n",indicesNew.size(), faceNormal.size());
      std::vector<Vector4d> newNormals;
      std::vector<int> faceParents;
      indicesNew  = mergeTriangles(indicesNew,faceNormal,newNormals, faceParents, ps->vertices );

      printf("Remove Colinear Points\n");
      offset3D_RemoveColinear(ps->vertices, indicesNew,pointToFaceInds, pointToFacePos);

      printf("offset\n");
      verticesNew = offset3D_offset(ps->vertices, indicesNew, newNormals, pointToFaceInds, pointToFacePos, off, 0);

      PolySet *sub_result =  new PolySet(3,  true);
      sub_result->vertices = verticesNew;
      sub_result->indices = indicesNew;

      std::shared_ptr<const ManifoldGeometry> term = ManifoldUtils::createManifoldFromGeometry(std::shared_ptr<const PolySet>(sub_result));
//    std::shared_ptr<const ManifoldGeometry> term = ManifoldUtils::createManifoldFromGeometry(decomposed[i]);
      if(i == 0) geom = std::make_shared<ManifoldGeometry>(*term);
      else *geom = *geom + *term;	
    }
    return geom;
  } else {
//    printf("Downsize %g\n",off);	  

    std::vector<Vector3d> vertices = ps->vertices;
    std::vector<IndexedFace> indices = ps->indices;
    int round=0;
    std::vector<Vector4d> normals;
    std::vector<Vector4d> newNormals;
    do
    {
//      printf("New Round %d Vertices %ld Faces %ld\n=======================\n",round,vertices.size(), indices.size());	    

      indices  = offset3D_removeOverPoints(vertices, indices,round);
//      printf("Reindex OP\n");
      offset3D_reindex(vertices, indices, verticesNew, indicesNew);
      vertices = verticesNew;
      indices = indicesNew;

//      printf("Normals OP\n");
      normals = calcTriangleNormals(vertices, indices);

//      printf("Merge OP\n");
      std::vector<int> faceParents;
      indicesNew  = mergeTriangles(indices,normals,newNormals, faceParents, vertices ); 
      normals = newNormals;									   
      indices = indicesNew;									       

//      printf("Remove Colinear Points\n");
      offset3D_RemoveColinear(vertices, indices,pointToFaceInds, pointToFacePos);

      if(indices.size() == 0 || fabs(off) <  0.0001) break;
      if(round == 2) break;

//      printf("Offset OP\n");
      verticesNew = offset3D_offset(vertices, indices, normals, pointToFaceInds, pointToFacePos, off,round);
      vertices = verticesNew;
      round++;
    }
    while(1);
//    offset_3D_dump(vertices, indices);
    // -------------------------------
    // Map all points and assemble
    // -------------------------------
    PolySet *offset_result =  new PolySet(3, /* convex */ false);
    offset_result->vertices = vertices;
    offset_result->indices = indices;
    return std::shared_ptr<PolySet>(offset_result);
  }
}

std::shared_ptr<const Geometry> offset3D(const std::shared_ptr<const PolySet> &ps,double off) {
  bool enabled=true; // geht mit 4faces
		     // geht  mit boxes
		     // sphere proigram error
		     // singlepoint prog error
  if(!enabled) return offset3D_convex(ps, off);


  std::vector<std::shared_ptr<const PolySet>> decomposed;
  decomposed.push_back(ps); //  =  offset3D_decompose(ps);
//  printf("Decomposed into %ld parts\n",decomposed.size());
  if(decomposed.size() == 0) {
    PolySet *offset_result =  new PolySet(3, /* convex */ true);
    return std::shared_ptr<PolySet>(offset_result);
  }
  if(decomposed.size() == 1) {
  	return offset3D_convex(decomposed[0], off);
  }
  std::shared_ptr<ManifoldGeometry> geom = nullptr;
  for(unsigned int i=0;i<decomposed.size();i++)
  {
  	std::shared_ptr<const ManifoldGeometry>term = ManifoldUtils::createManifoldFromGeometry(offset3D_convex(decomposed[i], off));
//  	std::shared_ptr<const ManifoldGeometry> term = ManifoldUtils::createMutableManifoldFromGeometry(decomposed[i]);
        if(i == 0) geom = std::make_shared<ManifoldGeometry>(*term);
        else *geom = *geom + *term;	
  }
  return geom;
}

std::unique_ptr<const Geometry> createFilletInt(std::shared_ptr<const PolySet> ps,  std::vector<bool> corner_selected, double r, int fn, double minang);

Vector3d createFilletRound(Vector3d pt)
{
   double x=((int)(pt[0]*1e6))/1e6;
   double y=((int)(pt[1]*1e6))/1e6;
   double z=((int)(pt[2]*1e6))/1e6;
   return Vector3d(x,y,z);
}
std::unique_ptr<const Geometry> addFillets(std::shared_ptr<const Geometry> result, const Geometry::Geometries & children, double r, int fn) {
  printf("do fillet magic\n");	
  std::unordered_set<Vector3d> points;
  Vector3d pt;
  std::shared_ptr<const PolySet> psr = PolySetUtils::getGeometryAsPolySet(result);
  for(const Vector3d pt : psr->vertices){
    points.insert(createFilletRound(pt));
  }

  for(const auto child: children) {
    std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(child.second);
    for(const Vector3d pt : ps->vertices) {
      points.erase(createFilletRound(pt));
    }
  }

  std::vector<bool> corner_selected;
  for(int i=0;i<psr->vertices.size();i++) corner_selected.push_back(points.count(createFilletRound(psr->vertices[i]))>0?true:false);

  return  createFilletInt(psr,corner_selected, r, fn,30.0);

}

/*!
   Applies the operator to all child nodes of the given node.

   May return nullptr or any 3D Geometry object
 */
GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren3D(const AbstractNode& node, OpenSCADOperator op)
{
  Geometry::Geometries children = collectChildren3D(node);
  if (children.empty()) return {};

  if (op == OpenSCADOperator::HULL) {
    return ResultObject::mutableResult(std::shared_ptr<Geometry>(applyHull(children)));
  } else if (op == OpenSCADOperator::FILL) {
    for (const auto& item : children) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "fill() not yet implemented for 3D");
    }
  }

  // Only one child -> this is a noop
  if (children.size() == 1 && op != OpenSCADOperator::OFFSET) return ResultObject::constResult(children.front().second);

  switch (op) {
  case OpenSCADOperator::MINKOWSKI:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return ResultObject::constResult(actualchildren.front().second);
    return ResultObject::constResult(applyMinkowski(actualchildren));
    break;
  }
  case OpenSCADOperator::UNION:
  {

    const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return ResultObject::constResult(actualchildren.front().second);
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      std::shared_ptr<const ManifoldGeometry> csgResult = ManifoldUtils::applyOperator3DManifold(actualchildren, op);	    
      if(csgOpNode != nullptr && csgOpNode->r != 0){
        std::unique_ptr<const Geometry> geom_u = addFillets(csgResult, actualchildren, csgOpNode->r, csgOpNode->fn);
	std::shared_ptr<const Geometry> geom_s(geom_u.release());
        return ResultObject::mutableResult(geom_s);
	//csgResult = ManifoldUtils::createManifoldFromGeometry(geom_s);
      }
      return ResultObject::mutableResult(csgResult);
    }
#endif
#ifdef ENABLE_CGAL
    return ResultObject::constResult(std::shared_ptr<const Geometry>(CGALUtils::applyUnion3D(*csgOpNode, actualchildren.begin(), actualchildren.end())));
#else
    assert(false && "No boolean backend available");
#endif
    break;
  }
  case OpenSCADOperator::OFFSET:
  {
    std::string instance_name; 
    AssignmentList inst_asslist;
    ModuleInstantiation *instance = new ModuleInstantiation(instance_name,inst_asslist, Location::NONE);
    auto node1 = std::make_shared<CsgOpNode>(instance,OpenSCADOperator::UNION);

    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    const OffsetNode *offNode = dynamic_cast<const OffsetNode *>(&node);
    std::shared_ptr<const Geometry> geom;
    switch(actualchildren.size())
    {
      case 0:
        return {};
        break;
      case 1:
        geom ={actualchildren.front().second};
        break;
      default:
        geom = {CGALUtils::applyUnion3D(*node1, actualchildren.begin(), actualchildren.end())};
	break;
    }
 
    std::shared_ptr<const PolySet> ps= PolySetUtils::getGeometryAsPolySet(geom);
    if(ps != nullptr) {
      auto ps_offset =  offset3D(ps,offNode->delta);

      geom = std::move(ps_offset);
      return ResultObject::mutableResult(geom);
    }
  }
  default:
  {
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      std::shared_ptr<const ManifoldGeometry> csgResult = ManifoldUtils::applyOperator3DManifold(children, op);	    
      const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
      if(csgOpNode->r != 0){
        std::unique_ptr<const Geometry> geom_u = addFillets(csgResult, children, csgOpNode->r,csgOpNode->fn);
	std::shared_ptr<const Geometry> geom_s(geom_u.release());
//	csgResult = ManifoldUtils::createManifoldFromGeometry(geom_s);
        return ResultObject::mutableResult(geom_s);
      }
      return ResultObject::mutableResult(csgResult);
    }
#endif
#ifdef ENABLE_CGAL
    const CsgOpNode *csgOpNode = dynamic_cast<const CsgOpNode *>(&node);
    return ResultObject::constResult(CGALUtils::applyOperator3D(*csgOpNode, children, op));
#else
    assert(false && "No boolean backend available");
    #endif
    break;
  }
  }
}



/*!
   Apply 2D hull.

   May return an empty geometry but will not return nullptr.
 */

std::unique_ptr<Polygon2d> GeometryEvaluator::applyHull2D(const AbstractNode& node)
{
  auto children = collectChildren2D(node);
  auto geometry = std::make_unique<Polygon2d>();

#ifdef ENABLE_CGAL
  using CGALPoint2 = CGAL::Point_2<CGAL::Cartesian<double>>;
  // Collect point cloud
  std::list<CGALPoint2> points;
  for (const auto& p : children) {
    if (p) {
      for (const auto& o : p->outlines()) {
        for (const auto& v : o.vertices) {
          points.emplace_back(v[0], v[1]);
        }
      }
    }
  }
  if (points.size() > 0) {
    // Apply hull
    std::list<CGALPoint2> result;
    try {
      CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter(result));
      // Construct Polygon2d
      Outline2d outline;
      for (const auto& p : result) {
        outline.vertices.emplace_back(p[0], p[1]);
      }
      geometry->addOutline(outline);
    } catch (const CGAL::Failure_exception& e) {
      LOG(message_group::Warning, "GeometryEvaluator::applyHull2D() during CGAL::convex_hull_2(): %1$s", e.what());
    }
  }
#endif
  return geometry;
}

std::unique_ptr<Polygon2d> GeometryEvaluator::applyFill2D(const AbstractNode& node)
{
  // Merge and sanitize input geometry
  auto geometry_in = ClipperUtils::apply(collectChildren2D(node), Clipper2Lib::ClipType::Union);
  assert(geometry_in->isSanitized());

  std::vector<std::shared_ptr<const Polygon2d>> newchildren;
  // Keep only the 'positive' outlines, eg: the outside edges
  for (const auto& outline : geometry_in->outlines()) {
    if (outline.positive) {
      newchildren.push_back(std::make_shared<Polygon2d>(outline));
    }
  }

  // Re-merge geometry in case of nested outlines
  return ClipperUtils::apply(newchildren, Clipper2Lib::ClipType::Union);
}

std::unique_ptr<Geometry> GeometryEvaluator::applyHull3D(const AbstractNode& node)
{
  Geometry::Geometries children = collectChildren3D(node);

  auto P = PolySet::createEmpty();
  return applyHull(children);
}

std::unique_ptr<Polygon2d> GeometryEvaluator::applyMinkowski2D(const AbstractNode& node)
{
  auto children = collectChildren2D(node);
  if (!children.empty()) {
    return ClipperUtils::applyMinkowski(children);
  }
  return nullptr;
}

/*!
   Returns a list of Polygon2d children of the given node.
   May return empty Polygon2d object, but not nullptr objects
 */
std::vector<std::shared_ptr<const Polygon2d>> GeometryEvaluator::collectChildren2D(const AbstractNode& node)
{
  std::vector<std::shared_ptr<const Polygon2d>> children;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    auto& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom) {
      if (chgeom->getDimension() == 3) {
        LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Ignoring 3D child object for 2D operation");
        children.push_back(nullptr); // replace 3D geometry with empty geometry
      } else {
        if (chgeom->isEmpty()) {
          children.push_back(nullptr);
        } else {
          const auto polygon2d = std::dynamic_pointer_cast<const Polygon2d>(chgeom);
          assert(polygon2d);
          children.push_back(polygon2d);
        }
      }
    } else {
      children.push_back(nullptr);
    }
  }
  return children;
}

/*!
   Since we can generate both Nef and non-Nef geometry, we need to insert it into
   the appropriate cache.
   This method inserts the geometry into the appropriate cache if it's not already cached.
 */
void GeometryEvaluator::smartCacheInsert(const AbstractNode& node,
                                         const std::shared_ptr<const Geometry>& geom)
{
  const std::string& key = this->tree.getIdString(node);

  if (CGALCache::acceptsGeometry(geom)) {
    if (!CGALCache::instance()->contains(key)) {
      CGALCache::instance()->insert(key, geom);
    }
  } else if (!GeometryCache::instance()->contains(key)) {
    // FIXME: Sanity-check Polygon2d as well?
    // if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    //   assert(!ps->hasDegeneratePolygons());
    // }

    // Perhaps add acceptsGeometry() to GeometryCache as well?
    if (!GeometryCache::instance()->insert(key, geom)) {
      LOG(message_group::Warning, "GeometryEvaluator: Node didn't fit into cache.");
    }
  }
}

bool GeometryEvaluator::isSmartCached(const AbstractNode& node)
{
  const std::string& key = this->tree.getIdString(node);
  return GeometryCache::instance()->contains(key) || CGALCache::instance()->contains(key);
}

std::shared_ptr<const Geometry> GeometryEvaluator::smartCacheGet(const AbstractNode& node, bool preferNef)
{
  const std::string& key = this->tree.getIdString(node);
  if(key.empty() ) return {};
  const bool hasgeom = GeometryCache::instance()->contains(key);
  const bool hascgal = CGALCache::instance()->contains(key);
  if (hascgal && (preferNef || !hasgeom)) return CGALCache::instance()->get(key);
  if (hasgeom) return GeometryCache::instance()->get(key);
  return {};
}

/*!
   Returns a list of 3D Geometry children of the given node.
   May return empty geometries, but not nullptr objects
 */
Geometry::Geometries GeometryEvaluator::collectChildren3D(const AbstractNode& node)
{
  Geometry::Geometries children;
  for (const auto& item : this->visitedchildren[node.index()]) {
    auto& chnode = item.first;
    const std::shared_ptr<const Geometry>& chgeom = item.second;
    if (chnode->modinst->isBackground()) continue;

    // NB! We insert into the cache here to ensure that all children of
    // a node is a valid object. If we inserted as we created them, the
    // cache could have been modified before we reach this point due to a large
    // sibling object.
    smartCacheInsert(*chnode, chgeom);

    if (chgeom && chgeom->getDimension() == 2) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "Ignoring 2D child object for 3D operation");
      children.push_back(std::make_pair(item.first, nullptr)); // replace 2D geometry with empty geometry
    } else {
      // Add children if geometry is 3D OR null/empty
      children.push_back(item);
    }
  }
  return children;
}

/*!

 */
std::unique_ptr<Polygon2d> GeometryEvaluator::applyToChildren2D(const AbstractNode& node, OpenSCADOperator op)
{
  node.progress_report();
  if (op == OpenSCADOperator::MINKOWSKI) {
    return applyMinkowski2D(node);
  } else if (op == OpenSCADOperator::HULL) {
    return applyHull2D(node);
  } else if (op == OpenSCADOperator::FILL) {
    return applyFill2D(node);
  }

  auto children = collectChildren2D(node);

  if (children.empty()) {
    return nullptr;
  }

  if (children.size() == 1 && op != OpenSCADOperator::OFFSET ) {
    if (children[0]) {
      return std::make_unique<Polygon2d>(*children[0]); // Copy
    } else {
      return nullptr;
    }
  }

  Clipper2Lib::ClipType clipType;
  switch (op) {
  case OpenSCADOperator::UNION:
  case OpenSCADOperator::OFFSET:
    clipType = Clipper2Lib::ClipType::Union;
    break;
  case OpenSCADOperator::INTERSECTION:
    clipType = Clipper2Lib::ClipType::Intersection;
    break;
  case OpenSCADOperator::DIFFERENCE:
    clipType = Clipper2Lib::ClipType::Difference;
    break;
  default:
    LOG(message_group::Error, "Unknown boolean operation %1$d", int(op));
    return nullptr;
    break;
  }

  auto pol = ClipperUtils::apply(children, clipType);

  if (op == OpenSCADOperator::OFFSET) {
	const OffsetNode *offNode = dynamic_cast<const OffsetNode *>(&node);
        // ClipperLib documentation: The formula for the number of steps in a full
        // circular arc is ... Pi / acos(1 - arc_tolerance / abs(delta))
        double n = Calc::get_fragments_from_r(std::abs(offNode->delta), 360.0, offNode->fn, offNode->fs, offNode->fa);
        double arc_tolerance = std::abs(offNode->delta) * (1 - cos_degrees(180 / n));
        return   ClipperUtils::applyOffset(*pol, offNode->delta, offNode->join_type, offNode->miter_limit, arc_tolerance);
  }
  return pol;
}

/*!
   Adds ourself to our parent's list of traversed children.
   Call this for _every_ node which affects output during traversal.
   Usually, this should be called from the postfix stage, but for some nodes,
   we defer traversal letting other components (e.g. CGAL) render the subgraph,
   and we'll then call this from prefix and prune further traversal.

   The added geometry can be nullptr if it wasn't possible to evaluate it.
 */
void GeometryEvaluator::addToParent(const State& state,
                                    const AbstractNode& node,
                                    const std::shared_ptr<const Geometry>& geom)
{
  this->visitedchildren.erase(node.index());
  if (state.parent()) {
    this->visitedchildren[state.parent()->index()].push_back(std::make_pair(node.shared_from_this(), geom));
  } else {
    // Root node
    this->root = geom;
    assert(this->visitedchildren.empty());
  }
}

Response GeometryEvaluator::visit(State& state, const ColorNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      // First union all children
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      if ((geom = res.constptr())) {
        auto mutableGeom = res.asMutableGeometry();
        if (mutableGeom) mutableGeom->setColor(node.color);
        geom = mutableGeom;
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   Custom nodes are handled here => implicit union
 */
Response GeometryEvaluator::visit(State& state, const AbstractNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, OpenSCADOperator::UNION).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   Pass children to parent without touching them. Used by e.g. for loops
 */
Response GeometryEvaluator::visit(State& state, const ListNode& node)
{
  if (state.parent()) {
    if (state.isPrefix() && node.modinst->isBackground()) {
      if (node.modinst->isBackground()) state.setBackground(true);
      return Response::PruneTraversal;
    }
    if (state.isPostfix()) {
      unsigned int dim = 0;
      for (const auto& item : this->visitedchildren[node.index()]) {
        if (!isValidDim(item, dim)) break;
        auto& chnode = item.first;
        const std::shared_ptr<const Geometry>& chgeom = item.second;
        addToParent(state, *chnode, chgeom);
      }
      this->visitedchildren.erase(node.index());
    }
    return Response::ContinueTraversal;
  } else {
    // Handle when a ListNode is given root modifier
    return lazyEvaluateRootNode(state, node);
  }
}

/*!
 */
Response GeometryEvaluator::visit(State& state, const GroupNode& node)
{
  return visit(state, (const AbstractNode&)node);
}

Response GeometryEvaluator::lazyEvaluateRootNode(State& state, const AbstractNode& node) {
  if (state.isPrefix()) {
    if (node.modinst->isBackground()) {
      state.setBackground(true);
      return Response::PruneTraversal;
    }
    if (isSmartCached(node)) {
      return Response::PruneTraversal;
    }
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;

    unsigned int dim = 0;
    GeometryList::Geometries geometries;
    for (const auto& item : this->visitedchildren[node.index()]) {
      if (!isValidDim(item, dim)) break;
      auto& chnode = item.first;
      const std::shared_ptr<const Geometry>& chgeom = item.second;
      if (chnode->modinst->isBackground()) continue;
      // NB! We insert into the cache here to ensure that all children of
      // a node is a valid object. If we inserted as we created them, the
      // cache could have been modified before we reach this point due to a large
      // sibling object.
      smartCacheInsert(*chnode, chgeom);
      // Only use valid geometries
      if (chgeom && !chgeom->isEmpty()) geometries.push_back(item);
    }
    if (geometries.size() == 1) geom = geometries.front().second;
    else if (geometries.size() > 1) geom = std::make_shared<GeometryList>(geometries);

    this->root = geom;
  }
  return Response::ContinueTraversal;
}

/*!
   Root nodes are handled specially; they will flatten any child group
   nodes to avoid doing an implicit top-level union.

   NB! This is likely a temporary measure until a better implementation of
   group nodes is in place.
 */
Response GeometryEvaluator::visit(State& state, const RootNode& node)
{
  // If we didn't enable lazy unions, just union the top-level objects
  if (!Feature::ExperimentalLazyUnion.is_enabled()) {
    return visit(state, (const GroupNode&)node);
  }
  return lazyEvaluateRootNode(state, node);
}

Response GeometryEvaluator::visit(State& state, const OffsetNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      ResultObject res = applyToChildren(node, OpenSCADOperator::OFFSET);
      auto mutableGeom = res.asMutableGeometry();
      if (mutableGeom) mutableGeom->setConvexity(1);
      geom = mutableGeom;
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   RenderNodes just pass on convexity
 */
Response GeometryEvaluator::visit(State& state, const RenderNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
      auto mutableGeom = res.asMutableGeometry();
      if (mutableGeom) mutableGeom->setConvexity(node.convexity);
      geom = mutableGeom;
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    node.progress_report();
    addToParent(state, node, geom);
  }
  return Response::ContinueTraversal;
}

/*!
   Leaf nodes can create their own geometry, so let them do that

   input: None
   output: PolySet or Polygon2d
 */
Response GeometryEvaluator::visit(State& state, const LeafNode& node)
{
  if (state.isPrefix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = node.createGeometry();
      assert(geom);
      if (const auto polygon = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
        if (!polygon->isSanitized()) {
          geom = ClipperUtils::sanitize(*polygon);
        }
      } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
//        assert(!ps->hasDegeneratePolygons());
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::PruneTraversal;
}

Response GeometryEvaluator::visit(State& state, const TextNode& node)
{
  if (state.isPrefix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      auto polygonlist = node.createPolygonList();
      geom = ClipperUtils::apply(polygonlist, Clipper2Lib::ClipType::Union);
    } else {
      geom = GeometryCache::instance()->get(this->tree.getIdString(node));
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::PruneTraversal;
}


/*!
   input: List of 2D or 3D objects (not mixed)
   output: Polygon2d or 3D PolySet
   operation:
    o Perform csg op on children
 */
Response GeometryEvaluator::visit(State& state, const CsgOpNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, node.type).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D or 3D objects (not mixed)
   output: Polygon2d or 3D PolySet
   operation:
    o Union all children
    o Perform transform
 */
Response GeometryEvaluator::visit(State& state, const TransformNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
        // due to the way parse/eval works we can't currently distinguish between NaN and Inf
        LOG(message_group::Warning, node.modinst->location(), this->tree.getDocumentPath(), "Transformation matrix contains Not-a-Number and/or Infinity - removing object.");
      } else {
        // First union all children
        ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
        if ((geom = res.constptr())) {
          if (geom->getDimension() == 2) {
            auto polygons =  std::dynamic_pointer_cast<Polygon2d>(res.asMutableGeometry());
            assert(polygons);

            Transform2d mat2;
            mat2.matrix() <<
              node.matrix(0, 0), node.matrix(0, 1), node.matrix(0, 3),
              node.matrix(1, 0), node.matrix(1, 1), node.matrix(1, 3),
              node.matrix(3, 0), node.matrix(3, 1), node.matrix(3, 3);
            polygons->transform(mat2);
            // FIXME: We lose the transform if we copied a const geometry above. Probably similar issue in multiple places
            // A 2D transformation may flip the winding order of a polygon.
            // If that happens with a sanitized polygon, we need to reverse
            // the winding order for it to be correct.
            if (polygons->isSanitized() && mat2.matrix().determinant() <= 0) {
              geom = ClipperUtils::sanitize(*polygons);
            }
          } else if (geom->getDimension() == 3) {
            auto mutableGeom = res.asMutableGeometry();
            if (mutableGeom) mutableGeom->transform(node.matrix);
            geom = mutableGeom;
          }
        }
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

Outline2d alterprofile(Outline2d profile,double scalex, double scaley, double origin_x, double origin_y,double offset_x, double offset_y, double rot)
{
	Outline2d result;
	double ang=rot*3.14/180.0;
	double c=cos(ang);
	double s=sin(ang);
	int n=profile.vertices.size();
	for(int i=0;i<n;i++) {
		double x=(profile.vertices[i][0]-origin_x)*scalex;
		double y=(profile.vertices[i][1]-origin_y)*scaley;
		double xr = (x*c - y*s)+origin_x + offset_x;
		double yr = (y*c + x*s)+origin_y + offset_y;
		result.vertices.push_back(Vector2d(xr,yr));
	}
	return result;
}

void calculate_path_dirs(Vector3d prevpt, Vector3d curpt,Vector3d nextpt,Vector3d vec_x_last, Vector3d vec_y_last, Vector3d *vec_x, Vector3d *vec_y) {
	Vector3d diff1,diff2;
	diff1 = curpt - prevpt;
	diff2 = nextpt - curpt;
	double xfac=1.0,yfac=1.0,beta;

	if(diff1.norm() > 0.001) diff1.normalize();
	if(diff2.norm() > 0.001) diff2.normalize();
	Vector3d diff=diff1+diff2;

	if(diff.norm() < 0.001) {
		printf("User Error!\n");
		return ;
	} 
	if(vec_y_last.norm() < 0.001)  { // Needed in first step only
		vec_y_last = diff2.cross(vec_x_last);
		if(vec_y_last.norm() < 0.001) { vec_x_last[0]=1; vec_x_last[1]=0; vec_x_last[2]=0; vec_y_last = diff.cross(vec_x_last); }
		if(vec_y_last.norm() < 0.001) { vec_x_last[0]=0; vec_x_last[1]=1; vec_x_last[2]=0; vec_y_last = diff.cross(vec_x_last); }
		if(vec_y_last.norm() < 0.001) { vec_x_last[0]=0; vec_x_last[1]=0; vec_x_last[2]=1; vec_y_last = diff.cross(vec_x_last); }
	} else {
		// make vec_last normal to diff1
		Vector3d xn= vec_y_last.cross(diff1).normalized();
		Vector3d yn= diff1.cross(vec_x_last).normalized();

		// now fix the angle between xn and yn
		Vector3d vec_xy_ = (xn + yn).normalized();
		Vector3d vec_xy = vec_xy_.cross(diff1).normalized();
		vec_x_last = (vec_xy_ + vec_xy).normalized();
		vec_y_last = diff1.cross(xn).normalized();
	}

	diff=(diff1+diff2).normalized();

	*vec_y = diff.cross(vec_x_last);
	if(vec_y->norm() < 0.001) { vec_x_last[0]=1; vec_x_last[1]=0; vec_x_last[2]=0; *vec_y = diff.cross(vec_x_last); }
	if(vec_y->norm() < 0.001) { vec_x_last[0]=0; vec_x_last[1]=1; vec_x_last[2]=0; *vec_y = diff.cross(vec_x_last); }
	if(vec_y->norm() < 0.001) { vec_x_last[0]=0; vec_x_last[1]=0; vec_x_last[2]=1; *vec_y = diff.cross(vec_x_last); }
	vec_y->normalize(); 

	*vec_x = vec_y_last.cross(diff);
	if(vec_x->norm() < 0.001) { vec_y_last[0]=1; vec_y_last[1]=0; vec_y_last[2]=0; *vec_x = vec_y_last.cross(diff); }
	if(vec_x->norm() < 0.001) { vec_y_last[0]=0; vec_y_last[1]=1; vec_y_last[2]=0; *vec_x = vec_y_last.cross(diff); }
	if(vec_x->norm() < 0.001) { vec_y_last[0]=0; vec_y_last[1]=0; vec_y_last[2]=1; *vec_x = vec_y_last.cross(diff); }
	vec_x->normalize(); 

	if(diff1.norm() > 0.001 && diff2.norm() > 0.001) {
		beta = (*vec_x).dot(diff1); 
		xfac=sqrt(1-beta*beta);
		beta = (*vec_y).dot(diff1);
		yfac=sqrt(1-beta*beta);

	}
	(*vec_x) /= xfac;
	(*vec_y) /= yfac;
}

std::vector<Vector3d> calculate_path_profile(Vector3d *vec_x, Vector3d *vec_y,Vector3d curpt, const std::vector<Vector2d> &profile) {

	std::vector<Vector3d> result;
	for(unsigned int i=0;i<profile.size();i++) {
		result.push_back( Vector3d(
			curpt[0]+(*vec_x)[0]*profile[i][0]+(*vec_y)[0]*profile[i][1],
			curpt[1]+(*vec_x)[1]*profile[i][0]+(*vec_y)[1]*profile[i][1],
			curpt[2]+(*vec_x)[2]*profile[i][0]+(*vec_y)[2]*profile[i][1]
				));
	}
	return result;
}



static std::unique_ptr<Geometry> extrudePolygon(const PathExtrudeNode& node, const Polygon2d& poly)
{
  PolySetBuilder builder;
  builder.setConvexity(node.convexity);
  std::vector<Vector3d> path_os;
  std::vector<double> length_os;
  gboolean intersect=false;

  // Round the corners with radius
  int xdir_offset = 0; // offset in point list to apply the xdir
  std::vector<Vector3d> path_round; 
  unsigned int m = node.path.size();
  for(unsigned i=0;i<node.path.size();i++)
  {
	int draw_arcs=0;
	Vector3d diff1, diff2,center,arcpt;
	int secs;
	double ang;
	Vector3d cur=node.path[i].head<3>();
	double r=node.path[i][3];
	do
	{
		if(i == 0 && node.closed == 0) break;
		if(i == m-1 && node.closed == 0) break;

		Vector3d prev=node.path[(i+m-1)%m].head<3>();
		Vector3d next=node.path[(i+1)%m].head<3>();
		diff1=(prev-cur).normalized();
		diff2=(next-cur).normalized();
		Vector3d diff=(diff1+diff2).normalized();
	
		ang=acos(diff1.dot(-diff2));
		double arclen=ang*r;
		center=cur+(r/cos(ang/2.0))*diff;

		secs=node.fn;
		int secs_a,secs_s;
		secs_a=(int) ceil(180.0*ang/(G_PI*node.fa));
		if(secs_a > secs) secs=secs_a;

		secs_s=(int) ceil(arclen/node.fs);
		if(secs_s > secs) secs=secs_s;


		if(r == 0) break;
		if(secs  == 0)  break;
		draw_arcs=1;
	}
	while(false);
	if(draw_arcs) {
		draw_arcs=1;
		Vector3d diff1n=diff1.cross(diff2.cross(diff1)).normalized();
		for(int j=0;j<=secs;j++) {
			arcpt=center
				-diff1*r*sin(ang*j/(double) secs)
				-diff1n*r*cos(ang*j/(double) secs);
  			path_round.push_back(arcpt);
		}
		if(node.closed > 0 && i == 0) xdir_offset=secs; // user wants to apply xdir on this point
	} else path_round.push_back(cur);

  }

  // xdir_offset is claculated in in next step automatically
  //
  // Create oversampled path with fs. for streights
  path_os.push_back(path_round[xdir_offset]);
  length_os.push_back(0);
  m = path_round.size();
  int ifinal=node.closed?m:m-1;

  for(int i=1;i<=ifinal;i++) {
	  Vector3d prevPt = path_round[(i+xdir_offset-1)%m];
	  Vector3d curPt = path_round[(i+xdir_offset)%m];
	  Vector3d seg=curPt - prevPt;
	  double length_seg = seg.norm();
	  int split=ceil(length_seg/node.fs);
	  if(node.twist == 0 && node.scale_x == 1.0 && node.scale_y == 1.0
			  ) split=1;
	  for(int j=1;j<=split;j++) {
		double ratio=(double)j/(double)split;
	  	path_os.push_back(prevPt+seg*ratio);
	  	length_os.push_back((i-1+(double)j/(double)split)/(double) (path_round.size()-1));
	  }
  }
  if(node.closed) { // let close do its last pt itself
	  path_os.pop_back();
	  length_os.pop_back();
  }

  Vector3d lastPt, curPt, nextPt;
  Vector3d vec_x_last(node.xdir_x,node.xdir_y,node.xdir_z);
  Vector3d vec_y_last(0,0,0);
  vec_x_last.normalize();

  // in case of custom profile,poly shall exactly have one dummy outline,will be replaced
  for(const Outline2d &profile2d: poly.outlines()) {
  
    std::vector<Vector3d> lastProfile;
    std::vector<Vector3d> startProfile; 
    unsigned int m=path_os.size();
    unsigned int mfinal=(node.closed == true)?m+1:m-1;
    for (unsigned int i = 0; i <= mfinal; i++) {
        std::vector<Vector3d> curProfile; 
	double cur_twist;

#ifdef ENABLE_PYTHON
        if(node.twist_func != NULL) {
          cur_twist = python_doublefunc(node.twist_func, length_os[i]);
        } else 
#endif	
	cur_twist=node.twist *length_os[i];

	double cur_scalex=1.0+(node.scale_x-1.0)*length_os[i];
	double cur_scaley=1.0+(node.scale_y-1.0)*length_os[i];
	Outline2d profilemod;
	#ifdef ENABLE_PYTHON  
	if(node.profile_func != NULL)
	{
		Outline2d tmpx=python_getprofile(node.profile_func, node.fn, length_os[i%m]);
        	profilemod = alterprofile(tmpx,cur_scalex,cur_scaley,node.origin_x, node.origin_y,0, 0, cur_twist);
	}
	else
	#endif  
        profilemod = alterprofile(profile2d,cur_scalex,cur_scaley,node.origin_x, node.origin_y,0, 0, cur_twist);

	unsigned int n=profilemod.vertices.size();
	curPt = path_os[i%m];
	if(i > 0) lastPt = path_os[(i-1)%m]; else lastPt = path_os[i%m]; 
	if(node.closed == true) {
		nextPt = path_os[(i+1)%m];
	} else {
		if(i < m-1 ) nextPt = path_os[(i+1)%m];  else  nextPt = path_os[i%m]; 
	}
  	Vector3d vec_x, vec_y;
	if(i != m+1) {
		calculate_path_dirs(lastPt, curPt,nextPt,vec_x_last, vec_y_last, &vec_x, &vec_y);
		curProfile = calculate_path_profile(&vec_x, &vec_y,curPt,  profilemod.vertices);
	} else 	curProfile = startProfile;
	if(i == 1 && node.closed == true) startProfile=curProfile;

	if((node.closed == false && i == 1) || ( i >= 2)){ // create ring
		// collision detection
		Vector3d vec_z_last = vec_x_last.cross(vec_y_last);
		// check that all new points are above old plane lastPt, vec_z_last
		for(unsigned int j=0;j<n;j++) {
			double dist=(curProfile[j]-lastPt).dot(vec_z_last);
			if(dist < 0) intersect=true;
		}
		
		for(unsigned int j=0;j<n;j++) {
			builder.beginPolygon(3);
			builder.addVertex( builder.vertexIndex(Vector3d(lastProfile[(j+0)%n][0], lastProfile[(j+0)%n][1], lastProfile[(j+0)%n][2])));
			builder.addVertex( builder.vertexIndex(Vector3d(lastProfile[(j+1)%n][0], lastProfile[(j+1)%n][1], lastProfile[(j+1)%n][2])));
			builder.addVertex( builder.vertexIndex(Vector3d( curProfile[(j+1)%n][0],  curProfile[(j+1)%n][1],  curProfile[(j+1)%n][2])));
			builder.beginPolygon(3);
			builder.addVertex( builder.vertexIndex(Vector3d(lastProfile[(j+0)%n][0], lastProfile[(j+0)%n][1], lastProfile[(j+0)%n][2])));
			builder.addVertex( builder.vertexIndex(Vector3d( curProfile[(j+1)%n][0],  curProfile[(j+1)%n][1],  curProfile[(j+1)%n][2])));
			builder.addVertex(builder.vertexIndex(Vector3d(  curProfile[(j+0)%n][0],  curProfile[(j+0)%n][1],  curProfile[(j+0)%n][2])));
		}
	}
       if(node.closed == false && (i == 0 || i == m-1)) {
		Polygon2d face_poly;
		face_poly.addOutline(profilemod);
		std::unique_ptr<PolySet> ps_face = face_poly.tessellate(); 

		if(i == 0) {
			// Flip vertex ordering for bottom polygon
			for (auto & polygon : ps_face->indices) {
				std::reverse(polygon.begin(), polygon.end());
			}
		}
		for (auto &p3d : ps_face->indices) { 
			std::vector<Vector2d> p2d;
			for(unsigned int i=0;i<p3d.size();i++) {
				Vector3d pt = ps_face->vertices[p3d[i]];
				p2d.push_back(Vector2d(pt[0],pt[1]));
			}
			std::vector<Vector3d> newprof = calculate_path_profile(&vec_x, &vec_y,(i == 0)?curPt:nextPt,  p2d);
			builder.beginPolygon(newprof.size());
			for(Vector3d pt: newprof) {
				builder.addVertex(builder.vertexIndex(pt));
			}
		}
	}

	vec_x_last = vec_x.normalized();
	vec_y_last = vec_y.normalized();
	
	lastProfile = curProfile;
    }

  }
  if(intersect == true && node.allow_intersect == false) {
          LOG(message_group::Warning, "Model is self intersecting. Result is unpredictable. ");
  }
  return builder.build();
}

/*!
   input: List of 2D objects
   output: 3D PolySet
   operation:
    o Union all children
    o Perform extrude
 */
Response GeometryEvaluator::visit(State& state, const LinearExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      std::shared_ptr<const Geometry> geometry;
      if (!node.filename.empty()) {
        DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale_x);

        auto p2d = dxf.toPolygon2d();
        if (p2d) geometry = ClipperUtils::sanitize(*p2d);
      } else {
        geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      }
      if (geometry) {
        const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        geom = extrudePolygon(node, *polygons);
        assert(geom);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

Response GeometryEvaluator::visit(State& state, const PathExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      std::shared_ptr<Geometry> geometry =  applyToChildren2D(node, OpenSCADOperator::UNION);
      if (geometry) {
        const auto polygons = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        auto extruded = extrudePolygon(node, *polygons);
//	printf("extrude = %p\n",extruded);
        assert(extruded);
        geom = std::move(extruded);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}
/*!
   input: List of 2D objects
   output: 3D PolySet
   operation:
    o Union all children
    o Perform extrude
 */
Response GeometryEvaluator::visit(State& state, const RotateExtrudeNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      std::shared_ptr<const Polygon2d> geometry;
      if (!node.filename.empty()) {
        DxfData dxf(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale);
        auto p2d = dxf.toPolygon2d();
        if (p2d) geometry = ClipperUtils::sanitize(*p2d);
      } else {
        geometry = applyToChildren2D(node, OpenSCADOperator::UNION);
      }
      if (geometry) {
        geom = rotatePolygon(node, *geometry);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

static int pullObject_calccut(const PullNode &node, Vector3d p1, Vector3d p2,Vector3d &r)
{
	Vector3d dir=p2-p1;
	Vector3d res;
	Vector3d v1, v2;
	Vector3d z(0,0,1);
	v1=node.dir.cross(dir);
	v2=node.dir.cross(v1);
	if(linsystem(dir,v1,v2,node.anchor-p1,res)) return 1;

	if(res[0] < 0 || res[0] > 1) return 1;
	r =p1+dir*res[0];
	return 0;
}
static void pullObject_addtri(PolySetBuilder &builder,Vector3d a, Vector3d b, Vector3d c)
{
	builder.beginPolygon(3);
	builder.addVertex(builder.vertexIndex(Vector3d(c[0], c[1], c[2])));
	builder.addVertex(builder.vertexIndex(Vector3d(b[0], b[1], b[2])));
	builder.addVertex(builder.vertexIndex(Vector3d(a[0], a[1], a[2])));
}

static std::unique_ptr<PolySet> pullObject(const PullNode& node, const PolySet *ps)
{
  PolySetBuilder builder(0,0,3,true);
  auto ps_tess = PolySetUtils::tessellate_faces( *ps);
  for(unsigned int i=0;i<ps_tess->indices.size();i++) {
	  auto pol = ps_tess->indices[i];

	  //count upper points
	  int upper=0;
	  int lowind=0;
	  int highind=0;

	  for(int j=0;j<3;j++) { 
		Vector3d pt=ps_tess->vertices[pol[j]];
		float dist=(pt-node.anchor).dot(node.dir);
		if(dist > 0) { upper++; highind += j; } else {lowind += j; }
	  }
	  switch(upper)
	  {
		  case 0:
	  		builder.beginPolygon(3);
			for(int j=0;j<3;j++) { 
			        builder.addVertex(pol[2-j]);
			}
			break;
		  case 1:
			{
				std::vector<int> pol1;
				pol1.push_back(pol[(highind+1)%3]);
				pol1.push_back(pol[(highind+2)%3]);
				pol1.push_back(pol[highind]);
				// pol1[2] ist oben 
				//
				Vector3d p02, p12;
				if(pullObject_calccut(node, ps_tess->vertices[pol1[0]],ps_tess->vertices[pol1[2]],p02)) break;
				if(pullObject_calccut(node, ps_tess->vertices[pol1[1]],ps_tess->vertices[pol1[2]],p12)) break;

				pullObject_addtri(builder, ps_tess->vertices[pol1[0]],ps_tess->vertices[pol1[1]], p12);
				pullObject_addtri(builder, ps_tess->vertices[pol1[0]], p12,p02);

				pullObject_addtri(builder, p02,p12,p12+node.dir);
				pullObject_addtri(builder, p02,p12+node.dir,p02+node.dir);

				pullObject_addtri(builder, p02+node.dir,p12+node.dir,ps_tess->vertices[pol1[2]]+node.dir);
			}
		  case 2: 
			{
				std::vector<int> pol1;
				pol1.push_back(pol[(lowind+1)%3]);
				pol1.push_back(pol[(lowind+2)%3]);
				pol1.push_back(pol[lowind]);
				// pol1[2] ist unten 
				//
				Vector3d p02, p12;
				if(pullObject_calccut(node, ps_tess->vertices[pol1[0]],ps_tess->vertices[pol1[2]],p02)) break;
				if(pullObject_calccut(node, ps_tess->vertices[pol1[1]],ps_tess->vertices[pol1[2]],p12)) break;

				pullObject_addtri(builder, ps_tess->vertices[pol1[2]],p02, p12);

				pullObject_addtri(builder, p12, p02, p02+node.dir);
				pullObject_addtri(builder, p12, p02+node.dir,p12+node.dir) ;

				pullObject_addtri(builder, p12+node.dir,p02+node.dir,ps_tess->vertices[pol1[0]]+node.dir);
				pullObject_addtri(builder, p12+node.dir,ps_tess->vertices[pol1[0]]+node.dir,ps_tess->vertices[pol1[1]]+node.dir);
			}
			break;
		  case 3:
	  		builder.beginPolygon(3);
			for(int j=0;j<3;j++) { 
				Vector3d pt=ps_tess->vertices[pol[2-j]]+node.dir;
			        builder.addVertex(builder.vertexIndex(Vector3d(pt[0],pt[1], pt[2])));
			}
			break;
	  }
  }

  return builder.build();
}

static std::unique_ptr<PolySet> wrapObject(const WrapNode& node, const PolySet *ps)
{
  PolySetBuilder builder(0,0,3,true);
  int segments1=360.0/node.fa;
  int segments2=2*G_PI*node.r/node.fs;
  int segments=segments1>segments2?segments1:segments2;	  
  if(node.fn > 0) segments=node.fn;
  double arclen=2*G_PI*node.r/segments;

  for(const auto &p : ps->indices) {
    // find leftmost point		 
    int n=p.size();
    int minind=0;
    for(int j=1;j<p.size();j++) {
      if(ps->vertices[p[j]][0] < ps->vertices[p[minind]][0])
      minind=j;		
    }
    int forw_ind=minind;
    int back_ind=minind;
    double xcur, xnext;

    xcur=ps->vertices[p[minind]][0];
    std::vector<Vector3d> curslice;
    curslice.push_back(ps->vertices[p[minind]]);

    int end=0;
    do {
      if(xcur >= 0) xnext = ceil((xcur+1e-6)/arclen)*arclen;
      else xnext = -floor((-xcur+1e-6)/arclen)*arclen;
      while(ps->vertices[p[(forw_ind+1)%n]][0] <= xnext && ((forw_ind+1)%n) != back_ind ) {
        forw_ind= (forw_ind+1)%n;
        curslice.push_back(ps->vertices[p[forw_ind]]);
      }
      while(ps->vertices[p[(back_ind+n-1)%n]][0] <= xnext && ((back_ind+n-1)%n) != forw_ind) {
        back_ind= (back_ind+n-1)%n;
        curslice.insert(curslice.begin(),ps->vertices[p[back_ind]]);
      }

      Vector3d  forw_pt, back_pt;
      if(back_ind == ((forw_ind+1)%n)) {
      end=1;
      } else {
        // calculate intermediate forward point
        Vector3d tmp1, tmp2;
	
        tmp1 = ps->vertices[p[forw_ind]];
        tmp2 = ps->vertices[p[(forw_ind+1)%n]];
        forw_pt = tmp1 +(tmp2-tmp1)*(xnext-tmp1[0])/(tmp2[0]-tmp1[0]);
        curslice.push_back(forw_pt);									      
        tmp1 = ps->vertices[p[back_ind]];
        tmp2 = ps->vertices[p[(back_ind+n-1)%n]];
        back_pt = tmp1 +(tmp2-tmp1)*(xnext-tmp1[0])/(tmp2[0]-tmp1[0]);
        curslice.insert(curslice.begin(), back_pt);									      
      }  
									      
      double ang, rad;

      for(int j=0;j<curslice.size();j++) {
        auto &pt = curslice[j];
        ang=pt[0]/node.r;
        rad = node.r-pt[1];
        pt=Vector3d(rad*cos(ang),rad*sin(ang),pt[2]);
      }
      for(int j=0;j<curslice.size()-2;j++) {
        builder.beginPolygon(curslice.size());	  
        builder.addVertex(curslice[0]);	    
        builder.addVertex(curslice[j+1]);	    
        builder.addVertex(curslice[j+2]);	    
        builder.endPolygon();
      }
// TODO color alpha
      curslice.clear();
      xcur=xnext;
      curslice.push_back(back_pt);	    
      curslice.push_back(forw_pt);	    
    } while(end == 0);
  }
  auto ps1 = builder.build();
  return ps1;
}

Response GeometryEvaluator::visit(State& state, const PullNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps=PolySetUtils::getGeometryAsPolySet(geom);
    if(ps != nullptr) {
      std::unique_ptr<Geometry> ps_pulled =  pullObject(node,ps.get());
      newgeom = std::move(ps_pulled);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}

static std::unique_ptr<PolySet> debugObject(const DebugNode& node, const PolySet *ps)
{
  auto psx  = std::make_unique<PolySet>(ps->getDimension(), ps->convexValue());	  
  *psx = *ps;

  if(psx->color_indices.size() < psx->indices.size()) {
    auto cs = ColorMap::inst()->defaultColorScheme();
    Color4f  def_color = ColorMap::getColor(cs, RenderColor::OPENCSG_FACE_FRONT_COLOR);
    int defind = psx->colors.size();
    psx->colors.push_back(def_color);    
    while(psx->color_indices.size() < psx->indices.size()) {
      psx->color_indices.push_back(defind);	  
    }
  }
  Color4f debug_color = Color4f(255,0,0,255);
  int colorind = psx->colors.size();
  psx->colors.push_back(debug_color);    
  for(int i=0;i<node.faces.size();i++) {
   int ind=node.faces[i];
   psx->color_indices[ind] = colorind;
  }

  return psx;
}

Response GeometryEvaluator::visit(State& state, const DebugNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps=nullptr;
    if(std::shared_ptr<const ManifoldGeometry> mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) 
      ps=mani->toPolySet();
    else ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if(ps != nullptr) {
      std::unique_ptr<Geometry> ps_pulled =  debugObject(node,ps.get());
      newgeom = std::move(ps_pulled);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}


Response GeometryEvaluator::visit(State& state, const WrapNode& node)
{
  std::shared_ptr<const Geometry> newgeom;
  std::shared_ptr<const Geometry> geom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (geom) {
    std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(geom);
    if(ps != nullptr) {
       ps = PolySetUtils::tessellate_faces(*ps);
    } else ps= PolySetUtils::getGeometryAsPolySet(geom);
    if(ps != nullptr) { 
      std::unique_ptr<Geometry> ps_wrapped =  wrapObject(node,ps.get());
      newgeom = std::move(ps_wrapped);
      addToParent(state, node, newgeom);
      node.progress_report();
    }
  }
  return Response::ContinueTraversal;
}

/*!
   FIXME: Not in use
 */
Response GeometryEvaluator::visit(State& /*state*/, const AbstractPolyNode& /*node*/)
{
  assert(false);
  return Response::AbortTraversal;
}

std::shared_ptr<const Geometry> GeometryEvaluator::projectionCut(const ProjectionNode& node)
{
  std::shared_ptr<const Geometry> geom;
  std::shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
  if (newgeom) {
#ifdef ENABLE_MANIFOLD
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
      auto manifold = ManifoldUtils::createManifoldFromGeometry(newgeom);
      auto poly2d = manifold->slice();
      return std::shared_ptr<const Polygon2d>(ClipperUtils::sanitize(poly2d));
    }
#endif
#ifdef ENABLE_CGAL
    auto Nptr = CGALUtils::getNefPolyhedronFromGeometry(newgeom);
    if (Nptr && !Nptr->isEmpty()) {
      auto poly = CGALUtils::project(*Nptr, node.cut_mode);
      if (poly) {
        poly->setConvexity(node.convexity);
        geom = std::move(poly);
      }
    }
#endif
  }
  return geom;
}

std::shared_ptr<const Geometry> GeometryEvaluator::projectionNoCut(const ProjectionNode& node)
{
#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    const std::shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
    if (newgeom) {
        auto manifold = ManifoldUtils::createManifoldFromGeometry(newgeom);
        auto poly2d = manifold->project();
        return std::shared_ptr<const Polygon2d>(ClipperUtils::sanitize(poly2d));
    } else {
      return std::make_shared<Polygon2d>();
    }
  }
#endif

  std::vector<std::shared_ptr<const Polygon2d>> tmp_geom;
  for (const auto& [chnode, chgeom] : this->visitedchildren[node.index()]) {
    if (chnode->modinst->isBackground()) continue;

    // Clipper version of Geometry projection
    // Clipper doesn't handle meshes very well.
    // It's better in V6 but not quite there. FIXME: stand-alone example.
    // project chgeom -> polygon2d
    if (auto chPS = PolySetUtils::getGeometryAsPolySet(chgeom)) {
      if (auto poly = PolySetUtils::project(*chPS)) {
        tmp_geom.push_back(std::shared_ptr(std::move(poly)));
      }
    }
  }
  auto projected = ClipperUtils::applyProjection(tmp_geom);
  return std::shared_ptr(std::move(projected));
}


/*!
   input: List of 3D objects
   output: Polygon2d
   operation:
    o Union all children
    o Perform projection
 */
Response GeometryEvaluator::visit(State& state, const ProjectionNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (isSmartCached(node)) {
      geom = smartCacheGet(node, false);
    } else {
      if (node.cut_mode) {
        geom = projectionCut(node);
      } else {
        geom = projectionNoCut(node);
      }
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

/*!
   input: List of 2D or 3D objects (not mixed)
   output: any Geometry
   operation:
    o Perform cgal operation
 */
Response GeometryEvaluator::visit(State& state, const CgalAdvNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      switch (node.type) {
      case CgalAdvType::MINKOWSKI: {
        ResultObject res = applyToChildren(node, OpenSCADOperator::MINKOWSKI);
        geom = res.constptr();
        // If we added convexity, we need to pass it on
        if (geom && geom->getConvexity() != node.convexity) {
          auto editablegeom = res.asMutableGeometry();
          editablegeom->setConvexity(node.convexity);
          geom = editablegeom;
        }
        break;
      }
      case CgalAdvType::HULL: {
        geom = applyToChildren(node, OpenSCADOperator::HULL).constptr();
        break;
      }
      case CgalAdvType::FILL: {
        geom = applyToChildren(node, OpenSCADOperator::FILL).constptr();
        break;
      }
      case CgalAdvType::RESIZE: {
        ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
        auto editablegeom = res.asMutableGeometry();
        geom = editablegeom;
        if (editablegeom) {
          editablegeom->setConvexity(node.convexity);
          editablegeom->resize(node.newsize, node.autosize);
        }
        break;
      }
      default:
        assert(false && "not implemented");
      }
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

Response GeometryEvaluator::visit(State& state, const AbstractIntersectionNode& node)
{
  if (state.isPrefix()) {
    if (isSmartCached(node)) return Response::PruneTraversal;
    state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
  }
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      geom = applyToChildren(node, OpenSCADOperator::INTERSECTION).constptr();
    } else {
      geom = smartCacheGet(node, state.preferNef());
    }
    addToParent(state, node, geom);
    node.progress_report();
  }
  return Response::ContinueTraversal;
}

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
// FIXME: What is the convex/manifold situation of the resulting PolySet?
static std::unique_ptr<Geometry> roofOverPolygon(const RoofNode& node, const Polygon2d& poly)
{
  std::unique_ptr<PolySet> roof;
  if (node.method == "voronoi") {
    roof = roof_vd::voronoi_diagram_roof(poly, node.fa, node.fs);
    roof->setConvexity(node.convexity);
  } else if (node.method == "straight") {
    roof = roof_ss::straight_skeleton_roof(poly);
    roof->setConvexity(node.convexity);
  } else {
    assert(false && "Invalid roof method");
  }

  return roof;
}

Response GeometryEvaluator::visit(State& state, const RoofNode& node)
{
  if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
  if (state.isPostfix()) {
    std::shared_ptr<const Geometry> geom;
    if (!isSmartCached(node)) {
      const auto polygon2d = applyToChildren2D(node, OpenSCADOperator::UNION);
      if (polygon2d) {
        std::unique_ptr<Geometry> roof;
        try {
          roof = roofOverPolygon(node, *polygon2d);
        } catch (RoofNode::roof_exception& e) {
          LOG(message_group::Error, node.modinst->location(), this->tree.getDocumentPath(),
              "Skeleton computation error. " + e.message());
          roof = PolySet::createEmpty();
        }
        assert(roof);
        geom = std::move(roof);
      }
    } else {
      geom = smartCacheGet(node, false);
    }
    addToParent(state, node, geom);
  }
  return Response::ContinueTraversal;
}
#endif
