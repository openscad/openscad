#include "GeometryEvaluator.h"
#include "Tree.h"
#include "GeometryCache.h"
#include "Polygon2d.h"
#include "ModuleInstantiation.h"
#include "State.h"
#include "OffsetNode.h"
#include "TransformNode.h"
#include "LinearExtrudeNode.h"
#include "RoofNode.h"
#include "roof_ss.h"
#include "roof_vd.h"
#include "RotateExtrudeNode.h"
#include "CgalAdvNode.h"
#include "ProjectionNode.h"
#include "CsgOpNode.h"
#include "TextNode.h"
#include "RenderNode.h"
#include "ClipperUtils.h"
#include "PolySetUtils.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "calc.h"
#include "printutils.h"
#include "calc.h"
#include "DxfData.h"
#include "degree_trig.h"
#include <ciso646> // C alternative tokens (xor)
#include <algorithm>
#include "boost-utils.h"
#include <hash.h>
#include "boolean_utils.h"
#ifdef ENABLE_CGAL
#include "CGALCache.h"
#include "CGALHybridPolyhedron.h"
#include "cgalutils.h"
#include <CGAL/convex_hull_2.h>
#include <CGAL/Point_2.h>
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#include "manifoldutils.h"
#endif

class Geometry;
class Polygon2d;
class Tree;

GeometryEvaluator::GeometryEvaluator(const Tree& tree) : tree(tree) { }

/*!
   Set allownef to false to force the result to _not_ be a Nef polyhedron
 */
std::shared_ptr<const Geometry> GeometryEvaluator::evaluateGeometry(const AbstractNode& node,
                                                               bool allownef)
{
  const std::string& key = this->tree.getIdString(node);
  if (!GeometryCache::instance()->contains(key)) {
    std::shared_ptr<const Geometry> N;
#ifdef ENABLE_CGAL
    if (CGALCache::instance()->contains(key)) {
      N = CGALCache::instance()->get(key);
    }
#endif

    // If not found in any caches, we need to evaluate the geometry
    if (N) {
      this->root = N;
    } else {
      this->traverse(node);
    }
#ifdef ENABLE_CGAL
    if (std::dynamic_pointer_cast<const CGALHybridPolyhedron>(this->root)) {
      this->root = PolySetUtils::getGeometryAsPolySet(this->root);
    }
#endif
#ifdef ENABLE_MANIFOLD
    if (std::dynamic_pointer_cast<const ManifoldGeometry>(this->root)) {
      this->root = PolySetUtils::getGeometryAsPolySet(this->root);
    }
#endif

    if (!allownef) {
      // We cannot render concave polygons, so tessellate any 3D PolySets
      auto ps = PolySetUtils::getGeometryAsPolySet(this->root);
      if (ps && !ps->isEmpty()) {
        // Since is_convex() doesn't handle non-planar faces, we need to tessellate
        // also in the indeterminate state so we cannot just use a boolean comparison. See #1061
        bool convex = bool(ps->convexValue()); // bool is true only if tribool is true, (not indeterminate and not false)
        if (!convex) {
          assert(ps->getDimension() == 3);
          this->root = PolySetUtils::tessellate_faces(*ps);
        }
      }
    }
    smartCacheInsert(node, this->root);
    return this->root;
  }
  return GeometryCache::instance()->get(key);
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

GeometryEvaluator::ResultObject GeometryEvaluator::applyToChildren(const AbstractNode& node, OpenSCADOperator op)
{
  unsigned int dim = 0;
  for (const auto& item : this->visitedchildren[node.index()]) {
    if (!isValidDim(item, dim)) break;
  }
  if (dim == 2) return {std::shared_ptr<Geometry>(applyToChildren2D(node, op))};
  else if (dim == 3) return applyToChildren3D(node, op);
  return {};
}

int linsystem( Vector3d v1,Vector3d v2,Vector3d v3,Vector3d pt,Vector3d &res,double *detptr)
{
        float det,ad11,ad12,ad13,ad21,ad22,ad23,ad31,ad32,ad33;
        det=v1[0]*(v2[1]*v3[2]-v3[1]*v2[2])-v1[1]*(v2[0]*v3[2]-v3[0]*v2[2])+v1[2]*(v2[0]*v3[1]-v3[0]*v2[1]);
        if(detptr != NULL) *detptr=det;
        ad11=v2[1]*v3[2]-v3[1]*v2[2];
        ad12=v3[0]*v2[2]-v2[0]*v3[2];
        ad13=v2[0]*v3[1]-v3[0]*v2[1];
        ad21=v3[1]*v1[2]-v1[1]*v3[2];
        ad22=v1[0]*v3[2]-v3[0]*v1[2];
        ad23=v3[0]*v1[1]-v1[0]*v3[1];
        ad31=v1[1]*v2[2]-v2[1]*v1[2];
        ad32=v2[0]*v1[2]-v1[0]*v2[2];
        ad33=v1[0]*v2[1]-v2[0]*v1[1];

        if(fabs(det) < 0.00001)
                return 1;
        
        res[0] = (ad11*pt[0]+ad12*pt[1]+ad13*pt[2])/det;
        res[1] = (ad21*pt[0]+ad22*pt[1]+ad23*pt[2])/det;
        res[2] = (ad31*pt[0]+ad32*pt[1]+ad33*pt[2])/det;
        return 0;
}

int cut_face_face_face(Vector3d p1, Vector3d n1, Vector3d p2,Vector3d n2, Vector3d p3, Vector3d n3, Vector3d &res,double *detptr=NULL)
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

typedef std::vector<int> intList;
typedef std::vector<intList> intListList;

class TriCombineStub
{
	public:
		int begin, end;
		int operator==(const TriCombineStub ref)
		{
			if(this->begin == ref.begin && this->end == ref.end) return 1;
			return 0;
		}
};

unsigned int hash_value(const TriCombineStub& r) {
	unsigned int i;
	i=r.begin |(r.end<<16);
        return i;
}

int operator==(const TriCombineStub &t1, const TriCombineStub &t2) 
{
	if(t1.begin == t2.begin && t1.end == t2.end) return 1;
	return 0;
}
//
// combine all tringles into polygons where applicable
typedef std::vector<IndexedFace> indexedFaceList;
static indexedFaceList stl_tricombine(const std::vector<IndexedFace> &triangles)
{
	int i,j,n;
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
				e.begin=ind1;
				e.end=ind2;
				if(stubs_neg.find(e) != stubs_neg.end()) stubs_neg.erase(e);
				else if(stubs_pos.find(e) != stubs_pos.end()) printf("Duplicate Edge %d->%d \n",ind1,ind2);
				else stubs_pos.insert(e);
			}
			if(ind2 < ind1) // negative edge
			{
				e.begin=ind2;
				e.end=ind1;
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
		if(stubs_chain.count(stubs.begin) > 0)
		{
			stubs_bak.push_back(stubs);
		} else stubs_chain[stubs.begin]=stubs.end;
	}

	for( const auto& stubs : stubs_neg ) {
		if(stubs_chain.count(stubs.end) > 0)
		{
			TriCombineStub ts;
			ts.begin=stubs.end;
			ts.end=stubs.begin;
			stubs_bak.push_back(ts);
		} else stubs_chain[stubs.end]=stubs.begin;
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
					if(stubs_bak[i].begin == ind)
					{
						ind_new=stubs_bak[i].end;
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
		int beg,end,dist,distbest,begbest,repeat;
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
						value_pos2[poly[j]],j;
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
		
		if(poly.size() > 2) result.push_back(poly);
	}
	return result;
}

std::vector<IndexedFace> mergetriangles(const std::vector<IndexedFace> triangles,const std::vector<Vector3d> normals,std::vector<Vector3d> &newNormals)
{
	int i,j,k;
	int n;
	indexedFaceList emptyList;
	std::vector<IndexedFace> polygons;
	std::unordered_map<Vector3d,int>  norm_map;
	std::vector<Vector3d> norm_list;
	std::vector<indexedFaceList>  triangles_sorted;
	// sort triangles into buckets of same orientation
	for(int i=0;i<triangles.size();i++) {
		Vector3d norm=normals[i];
		int norm_ind;
		if(norm_map.count(norm) == 0) {
			norm_ind=norm_list.size();

			norm_list.push_back(norm);
			triangles_sorted.push_back(emptyList);

			norm_map[norm]=norm_ind;
		} else norm_ind = norm_map[norm];
		triangles_sorted[norm_ind].push_back(triangles[i]);
	}

	// now merge the triangles in all buckets independly
	for(int i=0;i<triangles_sorted.size();i++ ) {
		indexedFaceList polygons_sub = stl_tricombine(triangles_sorted[i]);
		for(int j=0;j<polygons_sub.size();j++) {
			polygons.push_back(polygons_sub[j]);
			newNormals.push_back(norm_list[i]);
		}
	}

	return polygons;
}

void roundNumber(double &x)
{
	int i;
	if(x > 0) {
		i=x*100000.0+0.5;
		x=(double)i/100000.0;
	}
	if(x < 0) {
		i=-x*100000.0+0.5;
		x=-(double)i/100000.0;
	}
}

double  offset_3D_round(double x)
{
	if(x >= 0) return ((int)(x*1e6+0.5))*1e-6;
	else return -((int)(-x*1e6+0.5))*1e-6;
}

std::vector<Vector3d> offset3D_sub(const std::vector<Vector3d> &vertices, const std::vector<IndexedFace> &indices, double &off)
{
	// ----------------------------------------------------------
	// create a point database and use it. and form polygons
	// ----------------------------------------------------------
	std::unordered_map<Vector3d, int, boost::hash<Vector3d> > pointIntMap;
	std::vector<Vector3d> verticesNew; // list of all the points in the object
	std::vector<intList>  pointToFaceInds; //  mapping pt_ind -> list of polygon inds which use it
	intList emptyList;
	for(int i=0;i<vertices.size();i++)
		pointToFaceInds.push_back(emptyList);

	// -------------------------------
	// Calculating face normals
	// -------------------------------
	std::vector<Vector3d>  faceNormal;
	for(int i=0;i<indices.size();i++) {
		IndexedFace pol = indices[i];
		assert (pol.size() >= 3);
		Vector3d diff1=vertices[pol[1]] - vertices[pol[0]];
		Vector3d diff2=vertices[pol[2]] - vertices[pol[1]];
		Vector3d norm = diff1.cross(diff2);
		assert(norm.norm() > 0.0001);
		norm.normalize();
		roundNumber(norm[0]);
		roundNumber(norm[1]);
		roundNumber(norm[2]);
		norm.normalize();
		faceNormal.push_back(norm);
	}

	std::vector<Vector3d> newNormals;
	std::vector<IndexedFace> polygons_merged  = mergetriangles(indices,faceNormal,newNormals); // TODO sind es immer dreiecke ?
	faceNormal=newNormals;
	printf("now %d faces\n",polygons_merged.size());

	// -------------------------------
	// now calc length of all edges
	// -------------------------------
	std::vector<double> edge_len, edge_len_factor;
	if(off < 0) {
		for(int i=0;i<polygons_merged.size();i++)
		{
			auto &pol = polygons_merged[i];
			int n=pol.size();
			for(int j=0;j<n;j++){
				int a=pol[j];
				int b=pol[(j+1)%n];
				if(b > a) {
					double dist=(vertices[b]-vertices[a]).norm();
					edge_len.push_back(dist);
					printf("%d - %d: %g\n",a,b, dist);
				}
			}
		}
	}

	// -------------------------------
	// calculate point-to-polygon relation
	// -------------------------------
	for(int i=0;i<polygons_merged.size();i++) {
		IndexedFace pol = polygons_merged[i];
		for(int j=0;j<pol.size(); j++) {
			pointToFaceInds[pol[j]].push_back(i);
		}
	}
	
	// ---------------------------------------
	// Calculate offset position to each point
	// ---------------------------------------
	double off_act=off;
	if(off_act < 0) off_act=-0.001; // TODO dynamisch

	for( int i = 0; i< pointToFaceInds.size();i++ ) {
		Vector3d newpt;
		intList indexes = pointToFaceInds[i];
		int valid;
		do
		{
			valid=0;
			double xmax=-1, ymax=-1, zmax=-1;
			int xind=-1, yind=-1, zind=-1;
			
			// find closest  normal to xdir
			for(int i=0;i<indexes.size();i++) {
				if(fabs(faceNormal[indexes[i]][0]) > xmax) {
					xmax=fabs(faceNormal[indexes[i]][0]);
					xind=indexes[i];
				}
			}
			if(xind == -1)  break;

			// find closest  normal to ydir
			for(int i=0;i<indexes.size();i++) {
				if(fabs(faceNormal[indexes[i]][1]) > ymax && faceNormal[indexes[i]].dot(faceNormal[xind]) < 0.99999) {
					ymax=fabs(faceNormal[indexes[i]][1]);
					yind=indexes[i];
				}
			}
			if(yind == -1)  break; 

			// find closest  normal to zdir
			for(int i=0;i<indexes.size();i++) {
				if(fabs(faceNormal[indexes[i]][2]) > zmax && faceNormal[indexes[i]].dot(faceNormal[xind]) < 0.99999 && faceNormal[indexes[i]].dot(faceNormal[yind]) < 0.99999 ) {
					zmax=fabs(faceNormal[indexes[i]][2]);
					zind=indexes[i];
				}
			}
			if(zind == -1) break;
			
			// now calculate the new pt
			if(cut_face_face_face(
						vertices[i]  +faceNormal[xind]*off_act  , faceNormal[xind], 
						vertices[i]  +faceNormal[yind]*off_act  , faceNormal[yind], 
						vertices[i]  +faceNormal[zind]*off_act  , faceNormal[zind], 
						newpt)){ printf("d\n");  break; }
			valid=1;
		} while(0);
		if(!valid)
		{
			Vector3d dir={0,0,0};
			for(int j=0;j<indexes.size();j++)
				dir += faceNormal[j];
			newpt  = vertices[i] + off_act* dir.normalized();
		}
		verticesNew.push_back(newpt);
	}
	// -------------------------------
	// now calc new length of all new edges
	// -------------------------------
	if(off < 0) {
		double off_min=0;
		int cnt=0;
		for(int i=0;i<polygons_merged.size();i++)
		{
			auto &pol = polygons_merged[i];
			int n=pol.size();
			for(int j=0;j<n;j++){
				int a=pol[j];
				int b=pol[(j+1)%n];
				if(b > a) {
					double dist=(verticesNew[b]-verticesNew[a]).norm();
					double fact = (dist-edge_len[cnt])/off_act;
					printf("%d - %d: %g %g \n",a,b, dist, fact);
					// find maximal downsize value
					double t_off_min=-edge_len[cnt]/fact;
					if(cnt == 0 ||off_min > t_off_min) off_min=t_off_min;
					edge_len_factor.push_back(fact);
					cnt++;
				}
			}
		}
		if(off <  off_min) off=off_min;
		printf("actual off is  %g\n",off);
		for(int i=0;i<verticesNew.size();i++) {
			Vector3d d=(verticesNew[i]-vertices[i])*off/off_act;
			verticesNew[i]=vertices[i]+d;
			verticesNew[i][0]=offset_3D_round(verticesNew[i][0]);
			verticesNew[i][1]=offset_3D_round(verticesNew[i][1]);
			verticesNew[i][2]=offset_3D_round(verticesNew[i][2]);
		}			
	}
	return verticesNew;

}

std::shared_ptr<PolySet> offset3D(const std::shared_ptr<const PolySet> &ps,double off) {
  printf("Running offset3D %d polygons\n",ps->indices.size());
//  if(off == 0) return ps;
  if(off > 0) { // upsize
    // TODO decomposition and assemble	
    std::vector<Vector3d> pointList = offset3D_sub(ps->vertices, ps->indices,off);
    // -------------------------------
    // Map all points and assemble
    // -------------------------------
    PolySet *offset_result =  new PolySet(3, /* convex */ true);
    offset_result->vertices = pointList;
    offset_result->indices = ps->indices;
    return std::shared_ptr<PolySet>(offset_result);
  } else {
    printf("Downsize\n");	  

    double off_left=off;
    std::vector<Vector3d> pointList=ps->vertices;
    std::vector<IndexedFace> indices = ps->indices;
    do
    {
      std::vector<Vector3d> pointListNew = offset3D_sub(pointList, indices,off);
      if(fabs(off-off_left) > 0.0001) {
        printf("offset was reduced!\n");

        // reindex to get rid of collapsed triangles
	std::vector<IndexedFace> indicesNew;
	Reindexer<Vector3d> vertices_;
	for(int i=0;i<ps->indices.size();i++) {
          auto face= ps->indices[i];		
	  IndexedFace facenew;
	  for(int j=0;j<face.size();j++) {
            int ind=vertices_.lookup(pointListNew[face[j]]);
	    if(facenew.size() > 0){
              if(facenew[0] == ind) continue;
              if(facenew[facenew.size()-1] == ind) continue;
	    }
	    facenew.push_back(ind);
	  }
	  if(facenew.size() >= 3) indicesNew.push_back(facenew);
	}
        std::vector<Vector3d> pointListNewX;
	vertices_.copy(std::back_inserter(pointListNewX));

	printf("new faces: %d: %d\n",pointListNewX.size(), indicesNew.size());
	for(int i=0;i<pointListNewX.size();i++) {
          printf("%g %g %g\n",pointListNewX[i][0]-pointListNewX[0][0], 	pointListNewX[i][1], pointListNewX[i][2]);	
	}
	pointList = pointListNewX;
	indices = indicesNew;
        off_left -= off;
        break;
      } else {
	pointList = pointListNew;
	break;
      }
    }
    while(true);
    // -------------------------------
    // Map all points and assemble
    // -------------------------------
    PolySet *offset_result =  new PolySet(3, /* convex */ true);
    offset_result->vertices = pointList;
    offset_result->indices = indices;
    return std::shared_ptr<PolySet>(offset_result);
  }
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
    return {std::shared_ptr<Geometry>(applyHull(children))};
  } else if (op == OpenSCADOperator::FILL) {
    for (const auto& item : children) {
      LOG(message_group::Warning, item.first->modinst->location(), this->tree.getDocumentPath(), "fill() not yet implemented for 3D");
    }
  }

  // Only one child -> this is a noop
  if (children.size() == 1 && op != OpenSCADOperator::OFFSET) return {children.front().second};

  switch (op) {
  case OpenSCADOperator::MINKOWSKI:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return {actualchildren.front().second};
    return {applyMinkowski(actualchildren)};
    break;
  }
  case OpenSCADOperator::UNION:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
    if (actualchildren.size() == 1) return {actualchildren.front().second};
#ifdef ENABLE_MANIFOLD
    if (Feature::ExperimentalManifold.is_enabled()) {
      return {ManifoldUtils::applyOperator3DManifold(actualchildren, op)};
    }
#endif
#ifdef ENABLE_CGAL
    else if (Feature::ExperimentalFastCsg.is_enabled()) {
      return {std::shared_ptr<Geometry>(CGALUtils::applyUnion3DHybrid(actualchildren.begin(), actualchildren.end()))};
    }
    return {CGALUtils::applyUnion3D(actualchildren.begin(), actualchildren.end())};
#else
    assert(false && "No boolean backend available");
#endif
    break;
  }
  case OpenSCADOperator::OFFSET:
  {
    Geometry::Geometries actualchildren;
    for (const auto& item : children) {
      if (item.second && !item.second->isEmpty()) actualchildren.push_back(item);
    }
    if (actualchildren.empty()) return {};
//    if (actualchildren.size() == 1) return {actualchildren.front().second};
    std::shared_ptr<const Geometry> geom = {CGALUtils::applyUnion3D(actualchildren.begin(), actualchildren.end())};
 
    const OffsetNode *offNode = dynamic_cast<const OffsetNode *>(&node);
    if(std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
      auto ps_offset =  offset3D(ps,offNode->delta);

      geom = std::move(ps_offset);
      return geom;
    } else if(const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom).get()) {
      for (const Geometry::GeometryItem& item : geomlist->getChildren()) { // TODO
      }
        
    } else if (std::shared_ptr<const CGAL_Nef_polyhedron> nef = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
      const CGAL_Nef_polyhedron nefcont=*(nef.get());
      std::shared_ptr<PolySet> ps = CGALUtils::createPolySetFromNefPolyhedron3(*(nefcont.p3));
      std::shared_ptr<PolySet> ps_offset =  offset3D(ps,offNode->delta);
      geom = std::move(ps_offset);
      return geom;
    } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) { // TODO
    }
  }
  default:
  {
#ifdef ENABLE_MANIFOLD
    if (Feature::ExperimentalManifold.is_enabled()) {
      return {ManifoldUtils::applyOperator3DManifold(children, op)};
    }
#endif
#ifdef ENABLE_CGAL
    if (Feature::ExperimentalFastCsg.is_enabled()) {
      // FIXME: It's annoying to have to disambiguate here:
      return {std::shared_ptr<Geometry>(CGALUtils::applyOperator3DHybrid(children, op))};
    }
    return {CGALUtils::applyOperator3D(children, op)};
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
  auto geometry_in = ClipperUtils::apply(collectChildren2D(node), ClipperLib::ctUnion);

  std::vector<std::shared_ptr<const Polygon2d>> newchildren;
  // Keep only the 'positive' outlines, eg: the outside edges
  for (const auto& outline : geometry_in->outlines()) {
    if (outline.positive) {
      newchildren.push_back(std::make_shared<Polygon2d>(outline));
    }
  }

  // Re-merge geometry in case of nested outlines
  return ClipperUtils::apply(newchildren, ClipperLib::ctUnion);
}

std::unique_ptr<Geometry> GeometryEvaluator::applyHull3D(const AbstractNode& node)
{
  Geometry::Geometries children = collectChildren3D(node);

  auto P = std::make_unique<PolySet>(3);
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

#ifdef ENABLE_CGAL
  if (CGALCache::acceptsGeometry(geom)) {
    if (!CGALCache::instance()->contains(key)) CGALCache::instance()->insert(key, geom);
  } else {
#endif
    if (!GeometryCache::instance()->contains(key)) {
      if (!GeometryCache::instance()->insert(key, geom)) {
        LOG(message_group::Warning, "GeometryEvaluator: Node didn't fit into cache.");
      }
    }
#ifdef ENABLE_CGAL
  }
#endif
}

bool GeometryEvaluator::isSmartCached(const AbstractNode& node)
{
  const std::string& key = this->tree.getIdString(node);
  return (GeometryCache::instance()->contains(key)
#ifdef ENABLE_CGAL
	  || CGALCache::instance()->contains(key)
#endif
    );
}

std::shared_ptr<const Geometry> GeometryEvaluator::smartCacheGet(const AbstractNode& node, bool preferNef)
{
  const std::string& key = this->tree.getIdString(node);
  std::shared_ptr<const Geometry> geom;
  bool hasgeom = GeometryCache::instance()->contains(key);
#ifdef ENABLE_CGAL
  bool hascgal = CGALCache::instance()->contains(key);
  if (hascgal && (preferNef || !hasgeom)) geom = CGALCache::instance()->get(key);
  else
#endif
  if (hasgeom) geom = GeometryCache::instance()->get(key);
  return geom;
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

  ClipperLib::ClipType clipType;
  switch (op) {
  case OpenSCADOperator::UNION:
  case OpenSCADOperator::OFFSET:
    clipType = ClipperLib::ctUnion;
    break;
  case OpenSCADOperator::INTERSECTION:
    clipType = ClipperLib::ctIntersection;
    break;
  case OpenSCADOperator::DIFFERENCE:
    clipType = ClipperLib::ctDifference;
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
        double n = Calc::get_fragments_from_r(std::abs(offNode->delta), offNode->fn, offNode->fs, offNode->fa);
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
      auto geometrylist = node.createGeometryList();
      std::vector<std::shared_ptr<const Polygon2d>> polygonlist;
      for (const auto& geometry : geometrylist) {
        const auto polygon = std::dynamic_pointer_cast<const Polygon2d>(geometry);
        assert(polygon);
        polygonlist.push_back(polygon);
      }
      geom = ClipperUtils::apply(polygonlist, ClipperLib::ctUnion);
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
            std::shared_ptr<const Polygon2d> polygons = std::dynamic_pointer_cast<const Polygon2d>(geom);
            assert(polygons);

            // If we got a const object, make a copy
            std::shared_ptr<Polygon2d> newpoly;
            if (res.isConst()) {
              newpoly = std::make_shared<Polygon2d>(*polygons);
	    }
            else {
              newpoly = std::dynamic_pointer_cast<Polygon2d>(res.ptr());
	    }

            Transform2d mat2;
            mat2.matrix() <<
              node.matrix(0, 0), node.matrix(0, 1), node.matrix(0, 3),
              node.matrix(1, 0), node.matrix(1, 1), node.matrix(1, 3),
              node.matrix(3, 0), node.matrix(3, 1), node.matrix(3, 3);
            newpoly->transform(mat2);
	    // FIXME: We lose the transform if we copied a const geometry above. Probably similar issue in multiple places
            // A 2D transformation may flip the winding order of a polygon.
            // If that happens with a sanitized polygon, we need to reverse
            // the winding order for it to be correct.
            if (newpoly->isSanitized() && mat2.matrix().determinant() <= 0) {
              geom = ClipperUtils::sanitize(*newpoly);
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

static void translate_PolySet(PolySet& ps, const Vector3d& translation) // TODO duplicate with CGALutils ?
{
  for (auto& v : ps.vertices) {
      v += translation;
  }
}

/*
   Compare Euclidean length of vectors
   Return:
    -1 : if v1  < v2
     0 : if v1 ~= v2 (approximation to compoensate for floating point precision)
     1 : if v1  > v2
 */
int sgn_vdiff(const Vector2d& v1, const Vector2d& v2) {
  constexpr double ratio_threshold = 1e5; // 10ppm difference
  double l1 = v1.norm();
  double l2 = v2.norm();
  // Compare the average and difference, to be independent of geometry scale.
  // If the difference is within ratio_threshold of the avg, treat as equal.
  double scale = (l1 + l2);
  double diff = 2 * std::fabs(l1 - l2) * ratio_threshold;
  return diff > scale ? (l1 < l2 ? -1 : 1) : 0;
}

/*
   Enable/Disable experimental 4-way split quads for linear_extrude, with added midpoint.
   These look very nice when(and only when) diagonals are near equal.
   This typically happens when an edge is colinear with the origin.
 */
//#define LINEXT_4WAY

/*
   Attempt to triangulate quads in an ideal way.
   Each quad is composed of two adjacent outline vertices: (prev1, curr1)
   and their corresponding transformed points one step up: (prev2, curr2).
   Quads are triangulated across the shorter of the two diagonals, which works well in most cases.
   However, when diagonals are equal length, decision may flip depending on other factors.
 */
static void add_slice(PolySetBuilder &builder, const Polygon2d& poly,
                      double rot1, double rot2,
                      double h1, double h2,
                      const Vector2d& scale1,
                      const Vector2d& scale2)
{
  Eigen::Affine2d trans1(Eigen::Scaling(scale1) * Eigen::Affine2d(rotate_degrees(-rot1)));
  Eigen::Affine2d trans2(Eigen::Scaling(scale2) * Eigen::Affine2d(rotate_degrees(-rot2)));
#ifdef LINEXT_4WAY
  Eigen::Affine2d trans_mid(Eigen::Scaling((scale1 + scale2) / 2) * Eigen::Affine2d(rotate_degrees(-(rot1 + rot2) / 2)));
  bool is_straight = rot1 == rot2 && scale1[0] == scale1[1] && scale2[0] == scale2[1];
#endif
  bool any_zero = scale2[0] == 0 || scale2[1] == 0;
  bool any_non_zero = scale2[0] != 0 || scale2[1] != 0;
  // Not likely to matter, but when no twist (rot2 == rot1),
  // setting back_twist true helps keep diagonals same as previous builds.
  bool back_twist = rot2 <= rot1;

  for (const auto& o : poly.outlines()) {
    Vector2d prev1 = trans1 * o.vertices[0];
    Vector2d prev2 = trans2 * o.vertices[0];

    // For equal length diagonals, flip selected choice depending on direction of twist and
    // whether the outline is negative (eg circle hole inside a larger circle).
    // This was tested against circles with a single point touching the origin,
    // and extruded with twist.  Diagonal choice determined by whichever option
    // matched the direction of diagonal for neighboring edges (which did not exhibit "equal" diagonals).
    bool flip = ((!o.positive) xor (back_twist));

    for (size_t i = 1; i <= o.vertices.size(); ++i) {
      Vector2d curr1 = trans1 * o.vertices[i % o.vertices.size()];
      Vector2d curr2 = trans2 * o.vertices[i % o.vertices.size()];

      int diff_sign = sgn_vdiff(prev1 - curr2, curr1 - prev2);
      bool splitfirst = diff_sign == -1 || (diff_sign == 0 && !flip);

#ifdef LINEXT_4WAY
      // Diagonals should be equal whenever an edge is co-linear with the origin (edge itself need not touch it)
      if (!is_straight && diff_sign == 0) {
        // Split into 4 triangles, with an added midpoint.
        //Vector2d mid_prev = trans3 * (prev1 +curr1+curr2)/4;
        Vector2d mid = trans_mid * (o.vertices[(i - 1) % o.vertices.size()] + o.vertices[i % o.vertices.size()]) / 2;
        double h_mid = (h1 + h2) / 2;
        builder.appendPoly(3);
        builder.insertVertex(prev1[0], prev1[1], h1);
        builder.insertVertex(mid[0],   mid[1], h_mid);
        builder.insertVertex(curr1[0], curr1[1], h1);
        builder.appendPoly(3);
        builder.insertVertex(curr1[0], curr1[1], h1);
        builder.insertVertex(mid[0],   mid[1], h_mid);
        builder.insertVertex(curr2[0], curr2[1], h2);
        builder.appendPoly(3);
        builder.insertVertex(curr2[0], curr2[1], h2);
        builder.insertVertex(mid[0],   mid[1], h_mid);
        builder.insertVertex(prev2[0], prev2[1], h2);
        builder.appendPoly(3);
        builder.insertVertex(prev2[0], prev2[1], h2);
        builder.insertVertex(mid[0],   mid[1], h_mid);
        builder.insertVertex(prev1[0], prev1[1], h1);
      } else
#endif // ifdef LINEXT_4WAY
      // Split along shortest diagonal,
      // unless at top for a 0-scaled axis (which can create 0 thickness "ears")
      if (splitfirst xor any_zero) {
        builder.appendPoly({
		Vector3d(curr1[0], curr1[1], h1),
		Vector3d(curr2[0], curr2[1], h2),
		Vector3d(prev1[0], prev1[1], h1)
		});
        if (!any_zero || (any_non_zero && prev2 != curr2)) {
          builder.appendPoly({
		Vector3d(prev2[0], prev2[1], h2),
		Vector3d(prev1[0], prev1[1], h1),
		Vector3d(curr2[0], curr2[1], h2)
	  });
        }
      } else {
        builder.appendPoly({
		Vector3d(curr1[0], curr1[1], h1),
		Vector3d(prev2[0], prev2[1], h2),
		Vector3d(prev1[0], prev1[1], h1)
	});
        if (!any_zero || (any_non_zero && prev2 != curr2)) {
          builder.appendPoly({
		Vector3d(curr1[0], curr1[1], h1),
		Vector3d(curr2[0], curr2[1], h2),
		Vector3d(prev2[0], prev2[1], h2)
	  });	
        }
      }
      prev1 = curr1;
      prev2 = curr2;
    }
  }
}

// Insert vertices for segments interpolated between v0 and v1.
// The last vertex (t==1) is not added here to avoid duplicate vertices,
// since it will be the first vertex of the *next* edge.
static void add_segmented_edge(Outline2d& o, const Vector2d& v0, const Vector2d& v1, unsigned int edge_segments) {
  for (unsigned int j = 0; j < edge_segments; ++j) {
    double t = static_cast<double>(j) / edge_segments;
    o.vertices.push_back((1 - t) * v0 + t * v1);
  }
}

// For each edge in original outline, find its max length over all slice transforms,
// and divide into segments no longer than fs.
static Outline2d splitOutlineByFs(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fs, unsigned int slices)
{
  const auto sz = o.vertices.size();

  Vector2d v0 = o.vertices[0];
  Outline2d o2;
  o2.positive = o.positive;

  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      auto edge_segments = static_cast<unsigned int>(std::ceil(max_edgelen / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      unsigned int edge_segments = static_cast<unsigned int>(std::ceil((v1 - v0).norm() * max_scale / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  }
  return o2;
}

// While total outline segments < fn, increment segment_count for edge with largest
// (max_edge_length / segment_count).
static Outline2d splitOutlineByFn(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fn, unsigned int slices)
{

  struct segment_tracker {
    size_t edge_index;
    double max_edgelen;
    unsigned int segment_count{1u};
    segment_tracker(size_t i, double len) : edge_index(i), max_edgelen(len) { }
    // metric for comparison: average between (max segment length, and max segment length after split)
    [[nodiscard]] double metric() const { return max_edgelen / (segment_count + 0.5); }
    bool operator<(const segment_tracker& rhs) const { return this->metric() < rhs.metric();  }
    [[nodiscard]] bool close_match(const segment_tracker& other) const {
      // Edges are grouped when metrics match by at least 99.9%
      constexpr double APPROX_EQ_RATIO = 0.999;
      double l1 = this->metric(), l2 = other.metric();
      return std::min(l1, l2) / std::max(l1, l2) >= APPROX_EQ_RATIO;
    }
  };

  const auto sz = o.vertices.size();
  std::vector<unsigned int> segment_counts(sz, 1);
  std::priority_queue<segment_tracker, std::vector<segment_tracker>> q;

  Vector2d v0 = o.vertices[0];
  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= sz; ++i) {
      Vector2d v1 = o.vertices[i % sz];
      double max_edgelen = (v1 - v0).norm() * max_scale;
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  }

  std::vector<segment_tracker> tmp_q;
  // Process priority_queue until number of segments is reached.
  size_t seg_total = sz;
  while (seg_total < fn) {
    auto current = q.top();

    // Group similar length segmented edges to keep result roughly symmetrical.
    while (!q.empty() && (tmp_q.empty() || current.close_match(tmp_q.front()))) {
      q.pop();
      tmp_q.push_back(current);
      current = q.top();
    }

    if (seg_total + tmp_q.size() <= fn) {
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        ++current.segment_count;
        ++segment_counts[current.edge_index];
        ++seg_total;
        q.push(current);
      }
    } else {
      // fn too low to segment last group, push back onto queue without change.
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        q.push(current);
      }
      break;
    }
  }

  // Create final segmented edges.
  Outline2d o2;
  o2.positive = o.positive;
  v0 = o.vertices[0];
  for (size_t i = 1; i <= sz; ++i) {
    Vector2d v1 = o.vertices[i % sz];
    add_segmented_edge(o2, v0, v1, segment_counts[i - 1]);
    v0 = v1;
  }

  assert(o2.vertices.size() <= fn);
  return o2;
}


/*!
   Input to extrude should be sanitized. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.
 */
static std::unique_ptr<Geometry> extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly)
{
  bool non_linear = node.twist != 0 || node.scale_x != node.scale_y;
  boost::tribool isConvex{poly.is_convex()};
  // Twist or non-uniform scale makes convex polygons into unknown polyhedrons
  if (isConvex && non_linear) isConvex = unknown;
  PolySetBuilder builder(0,0,3,isConvex);
  builder.setConvexity(node.convexity);
  if (node.height <= 0) return std::make_unique<PolySet>(3);

  size_t slices;
  if (node.has_slices) {
    slices = node.slices;
  } else if (node.has_twist) {
    double max_r1_sqr = 0; // r1 is before scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines())
      for (const auto& v : o.vertices)
        max_r1_sqr = fmax(max_r1_sqr, v.squaredNorm());
    // Calculate Helical curve length for Twist with no Scaling
    if (node.scale_x == 1.0 && node.scale_y == 1.0) {
      slices = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height, node.twist, node.fn, node.fs, node.fa);
    } else if (node.scale_x != node.scale_y) {  // non uniform scaling with twist using max slices from twist and non uniform scale
      double max_delta_sqr = 0; // delta from before/after scaling
      Vector2d scale(node.scale_x, node.scale_y);
      for (const auto& o : poly.outlines()) {
        for (const auto& v : o.vertices) {
          max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
        }
      }
      size_t slicesNonUniScale;
      size_t slicesTwist;
      slicesNonUniScale = (unsigned int)Calc::get_diagonal_slices(max_delta_sqr, node.height, node.fn, node.fs);
      slicesTwist = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height, node.twist, node.fn, node.fs, node.fa);
      slices = std::max(slicesNonUniScale, slicesTwist);
    } else { // uniform scaling with twist, use conical helix calculation
      slices = (unsigned int)Calc::get_conical_helix_slices(max_r1_sqr, node.height, node.twist, node.scale_x, node.fn, node.fs, node.fa);
    }
  } else if (node.scale_x != node.scale_y) {
    // Non uniform scaling, w/o twist
    double max_delta_sqr = 0; // delta from before/after scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines()) {
      for (const auto& v : o.vertices) {
        max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
      }
    }
    slices = Calc::get_diagonal_slices(max_delta_sqr, node.height, node.fn, node.fs);
  } else {
    // uniform or [1,1] scaling w/o twist needs only one slice
    slices = 1;
  }

  // Calculate outline segments if appropriate.
  Polygon2d seg_poly;
  bool is_segmented = false;
  if (node.has_segments) {
    // Set segments = 0 to disable
    if (node.segments > 0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.segments) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.segments, slices));
        }
      }
      is_segmented = true;
    }
  } else if (non_linear) {
    if (node.fn > 0.0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.fn) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.fn, slices));
        }
      }
    } else { // $fs and $fa based segmentation
      auto fa_segs = static_cast<unsigned int>(std::ceil(360.0 / node.fa));
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= fa_segs) {
          seg_poly.addOutline(o);
        } else {
          // try splitting by $fs, then check if $fa results in less segments
          auto fsOutline = splitOutlineByFs(o, node.twist, node.scale_x, node.scale_y, node.fs, slices);
          if (fsOutline.vertices.size() >= fa_segs) {
            seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, fa_segs, slices));
          } else {
            seg_poly.addOutline(std::move(fsOutline));
          }
        }
      }
    }
    is_segmented = true;
  }

  const Polygon2d& polyref = is_segmented ? seg_poly : poly;

  double h1, h2;
  if (node.center) {
    h1 = -node.height / 2.0;
    h2 = +node.height / 2.0;
  } else {
    h1 = 0;
    h2 = node.height;
  }

  // Create bottom face.
  auto ps_bottom = polyref.tessellate(); // bottom
  // Flip vertex ordering for bottom polygon
  for (auto& p : ps_bottom->indices) {
    std::reverse(p.begin(), p.end());
  }
  translate_PolySet(*ps_bottom, Vector3d(0, 0, h1));
  builder.append(*ps_bottom);

  // Create slice sides.
  for (unsigned int j = 0; j < slices; j++) {
    double rot1 = node.twist * j / slices;
    double rot2 = node.twist * (j + 1) / slices;
    double height1 = h1 + (h2 - h1) * j / slices;
    double height2 = h1 + (h2 - h1) * (j + 1) / slices;
    Vector2d scale1(1 - (1 - node.scale_x) * j / slices,
                    1 - (1 - node.scale_y) * j / slices);
    Vector2d scale2(1 - (1 - node.scale_x) * (j + 1) / slices,
                    1 - (1 - node.scale_y) * (j + 1) / slices);
    add_slice(builder, polyref, rot1, rot2, height1, height2, scale1, scale2);
  }

  // Create top face.
  // If either scale components are 0, then top will be zero-area, so skip it.
  if (node.scale_x != 0 && node.scale_y != 0) {
    Polygon2d top_poly(polyref);
    Eigen::Affine2d trans(Eigen::Scaling(node.scale_x, node.scale_y) * Eigen::Affine2d(rotate_degrees(-node.twist)));
    top_poly.transform(trans);
    auto ps_top = top_poly.tessellate();
    translate_PolySet(*ps_top, Vector3d(0, 0, h2));
    builder.append(*ps_top);
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
        auto extruded = extrudePolygon(node, *polygons);
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

static void fill_ring(std::vector<Vector3d>& ring, const Outline2d& o, double a, bool flip)
{
  if (flip) {
    unsigned int l = o.vertices.size() - 1;
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[l - i][0] * sin_degrees(a);
      ring[i][1] = o.vertices[l - i][0] * cos_degrees(a);
      ring[i][2] = o.vertices[l - i][1];
    }
  } else {
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[i][0] * sin_degrees(a);
      ring[i][1] = o.vertices[i][0] * cos_degrees(a);
      ring[i][2] = o.vertices[i][1];
    }
  }
}

/*!
   Input to extrude should be clean. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.

   FIXME: We should handle some common corner cases better:
   o 2D polygon having an edge being on the Y axis:
    In this case, we don't need to generate geometry involving this edge as it
    will be an internal edge.
   o 2D polygon having a vertex touching the Y axis:
    This is more complex as the resulting geometry will (may?) be nonmanifold.
    In any case, the previous case is a specialization of this, so the following
    should be handled for both cases:
    Since the ring associated with this vertex will have a radius of zero, it will
    collapse to one vertex. Any quad using this ring will be collapsed to a triangle.

   Currently, we generate a lot of zero-area triangles

 */
static std::unique_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly)
{
  if (node.angle == 0) return nullptr;

  PolySetBuilder builder;
  builder.setConvexity(node.convexity);

  double min_x = 0;
  double max_x = 0;
  unsigned int fragments = 0;
  for (const auto& o : poly.outlines()) {
    for (const auto& v : o.vertices) {
      min_x = fmin(min_x, v[0]);
      max_x = fmax(max_x, v[0]);
    }
  }

  if ((max_x - min_x) > max_x && (max_x - min_x) > fabs(min_x)) {
    LOG(message_group::Error, "all points for rotate_extrude() must have the same X coordinate sign (range is %1$.2f -> %2$.2f)", min_x, max_x);
    return nullptr;
  }

  fragments = (unsigned int)std::ceil(fmax(Calc::get_fragments_from_r(max_x - min_x, node.fn, node.fs, node.fa) * std::abs(node.angle) / 360, 1));

  bool flip_faces = (min_x >= 0 && node.angle > 0 && node.angle != 360) || (min_x < 0 && (node.angle < 0 || node.angle == 360));

  if (node.angle != 360) {
    auto ps_start = poly.tessellate(); // starting face
    Transform3d rot(angle_axis_degrees(90, Vector3d::UnitX()));
    ps_start->transform(rot);
    // Flip vertex ordering
    if (!flip_faces) {
      for (auto& p : ps_start->indices) {
        std::reverse(p.begin(), p.end());
      }
    }
    builder.append(*ps_start);

    auto ps_end = poly.tessellate();
    Transform3d rot2(angle_axis_degrees(node.angle, Vector3d::UnitZ()) * angle_axis_degrees(90, Vector3d::UnitX()));
    ps_end->transform(rot2);
    if (flip_faces) {
      for (auto& p : ps_end->indices) {
        std::reverse(p.begin(), p.end());
      }
    }
    builder.append(*ps_end);
  }

  for (const auto& o : poly.outlines()) {
    std::vector<Vector3d> rings[2];
    rings[0].resize(o.vertices.size());
    rings[1].resize(o.vertices.size());

    fill_ring(rings[0], o, (node.angle == 360) ? -90 : 90, flip_faces); // first ring
    for (unsigned int j = 0; j < fragments; ++j) {
      double a;
      if (node.angle == 360) a = -90 + ((j + 1) % fragments) * 360.0 / fragments; // start on the -X axis, for legacy support
      else a = 90 - (j + 1) * node.angle / fragments; // start on the X axis
      fill_ring(rings[(j + 1) % 2], o, a, flip_faces);

      for (size_t i = 0; i < o.vertices.size(); ++i) {
        builder.appendPoly({
		rings[j % 2][(i + 1) % o.vertices.size()],
		rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
		rings[j % 2][i]
	});		

        builder.appendPoly({
		rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
		rings[(j + 1) % 2][i],
		rings[j % 2][i]
	});
      }
    }
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
  std::shared_ptr<const Geometry> geom;
  std::vector<std::unique_ptr<Polygon2d>> tmp_geom;
  BoundingBox bounds;
  for (const auto& [chnode, chgeom] : this->visitedchildren[node.index()]) {
    if (chnode->modinst->isBackground()) continue;

    // Clipper version of Geometry projection
    // Clipper doesn't handle meshes very well.
    // It's better in V6 but not quite there. FIXME: stand-alone example.
    // project chgeom -> polygon2d
    if (auto chPS = PolySetUtils::getGeometryAsPolySet(chgeom)) {
      if (auto poly = PolySetUtils::project(*chPS)) {
	bounds.extend(poly->getBoundingBox());
	tmp_geom.push_back(std::move(poly));
      }
    }
  }
  int pow2 = ClipperUtils::getScalePow2(bounds);

  ClipperLib::Clipper sumclipper;
  for (auto &poly : tmp_geom) {
    ClipperLib::Paths result = ClipperUtils::fromPolygon2d(*poly, pow2);
    // Using NonZero ensures that we don't create holes from polygons sharing
    // edges since we're unioning a mesh
    result = ClipperUtils::process(result, ClipperLib::ctUnion, ClipperLib::pftNonZero);
    // Add correctly winded polygons to the main clipper
    sumclipper.AddPaths(result, ClipperLib::ptSubject, true);
  }

  ClipperLib::PolyTree sumresult;
  // This is key - without StrictlySimple, we tend to get self-intersecting results
  sumclipper.StrictlySimple(true);
  sumclipper.Execute(ClipperLib::ctUnion, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
  if (sumresult.Total() > 0) {
    geom = ClipperUtils::toPolygon2d(sumresult, pow2);
  }

  return geom;
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
	  std::shared_ptr<Geometry> editablegeom;
          // If we got a const object, make a copy
          if (res.isConst()) editablegeom = geom->copy();
          else editablegeom = res.ptr();
          geom = editablegeom;
          editablegeom->setConvexity(node.convexity);
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
          roof = std::make_unique<PolySet>(3);
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
