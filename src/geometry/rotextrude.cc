#include "rotextrude.h"

#include <queue>
#include <boost/logic/tribool.hpp>

#include "GeometryUtils.h"
#include "RotateExtrudeNode.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "PolySetUtils.h"
#include "calc.h"
#include "degree_trig.h"
#include "Feature.h"
#include <cmath>

#include "manifoldutils.h"

Outline2d alterprofile(Outline2d profile,double scalex, double scaley, double
		origin_x, double origin_y,double offset_x, double offset_y,
		double rot);

void  append_rotary_vertex(PolySetBuilder &builder,const Outline2d *face, int index, double ang)
{
	double a=ang*G_PI / 180.0;
	builder.addVertex(builder.vertexIndex(Vector3d(
			face->vertices[index][0]*cos(a),
			face->vertices[index][0]*sin(a),
			face->vertices[index][1])));
}



static void fill_ring(std::vector<Vector3d>& ring, const std::vector<Vector2d> & vertices, double a, Vector3d dv, double fact, double xmid,bool flip)
{
  unsigned int l = vertices.size() - 1;
  for (unsigned int i = 0; i < vertices.size(); ++i) {
    unsigned int j = flip?l - i : i;	  
    //
    // cos(atan(x))=1/sqrt(1+x*x)
    // sin(atan(x))=x/sqrt(1+x*x)
    double tan_pitch= fact/(std::isnan(xmid)?vertices[j][0]:xmid);
    double cf=1/sqrt(1+tan_pitch*tan_pitch);
    double sf=cf*tan_pitch;
    Vector3d centripedal=Vector3d( cos_degrees(a), sin_degrees(a) ,0);
    Vector3d progress=Vector3d(-sin_degrees(a)*cf, cos_degrees(a)*cf, sf);
    Vector3d upwards=centripedal.cross(progress);
    ring[i] =  centripedal * vertices[j][0] + upwards * vertices[j][1] + dv;
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
std::unique_ptr<Geometry> rotatePolygonSub(const RotateExtrudeNode& node, const Polygon2d& poly, int fragments, int fragstart, int fragend, bool flip_faces)
{

  PolySetBuilder builder;
  builder.setConvexity(node.convexity);



  double fact=(node.v[2]/node.angle)*(180.0/G_PI);


#ifdef ENABLE_PYTHON  
  if(node.profile_func != NULL)
  {
	fragments=node.fn; // TODO fix
	Outline2d lastFace;
	Outline2d curFace;
	double last_ang=0, cur_ang=0;
	double last_twist=0.0, cur_twist=0.0;

        if(node.twist_func != NULL) {
          last_twist = python_doublefunc(node.twist_func, 0);
        } else last_twist=0;
	

	lastFace = alterprofile(python_getprofile(node.profile_func, node.fn, 0),1.0, 1.0,node.origin_x, node.origin_y, node.offset_x, node.offset_y, last_twist);
	if(node.angle != 360 || node.v.norm() > 0) {
    	  auto ps = poly.tessellate(); // starting face
          double xmid=NAN;
          if(node.method == "centered") {
            double xmin, xmax;
            xmin=xmax=ps->vertices[0][0];
            for(const auto &v : ps->vertices) {
              if(v[0] < xmin) xmin=v[0];	    
              if(v[0] > xmax) xmax=v[0];	    
            }
            xmid=(xmin+xmax)/2;
          } 

          std::vector<Vector3d> ring;
          ring.resize(3);
          for (auto& p : ps->indices) {
            std::vector<Vector2d> vertices;
            for(int j=0;j<3;j++)
            vertices.push_back(ps->vertices[p[j]].head<2>());

            fill_ring(ring , vertices, node.angle*fragstart/fragments, node.v*fragstart/fragments, fact, xmid, !flip_faces); // close start
            builder.appendPolygon(ring);

            fill_ring(ring, vertices, node.angle*fragend/fragments   , node.v*fragend/fragments  , fact, xmid, flip_faces); // close end
            builder.appendPolygon(ring);
          }
	}
  	for (unsigned int i = fragstart + 1; i <= fragend; i++) {
		cur_ang=i*node.angle/fragments;

		if(node.twist_func != NULL) {
		  cur_twist = python_doublefunc(node.twist_func, i/(double) fragments);
		} else
		cur_twist=i*node.twist /fragments;

		curFace = alterprofile(python_getprofile(node.profile_func, node.fn, cur_ang), 1.0, 1.0 , node.origin_x, node.origin_y, node.offset_x, node.offset_y , cur_twist);

		if(lastFace.vertices.size() == curFace.vertices.size()) {
			unsigned int n=lastFace.vertices.size();
			for(unsigned int j=0;j<n;j++) {
				builder.beginPolygon(3);
				append_rotary_vertex(builder,&lastFace,(j+0)%n, last_ang);
				append_rotary_vertex(builder,&lastFace,(j+1)%n, last_ang);
				append_rotary_vertex(builder,&curFace,(j+1)%n, cur_ang);
				builder.beginPolygon(3);
				append_rotary_vertex(builder,&lastFace,(j+0)%n, last_ang);
				append_rotary_vertex(builder,&curFace,(j+1)%n, cur_ang);
				append_rotary_vertex(builder,&curFace,(j+0)%n, cur_ang);
			}
		}

		lastFace = curFace;
		last_ang = cur_ang;
		last_twist = cur_twist;
	}
       }
  else
#endif
  {	  
  if (node.angle != 360 || node.v.norm() > 0) {
    auto ps = poly.tessellate(); // starting face
    double xmid=NAN;
    if(node.method == "centered") {
      double xmin, xmax;
      xmin=xmax=ps->vertices[0][0];
      for(const auto &v : ps->vertices) {
        if(v[0] < xmin) xmin=v[0];	    
        if(v[0] > xmax) xmax=v[0];	    
      }
      xmid=(xmin+xmax)/2;
    }  

    std::vector<Vector3d> ring;
    ring.resize(3);
    for (auto& p : ps->indices) {
      std::vector<Vector2d> vertices;
      for(int j=0;j<3;j++)
        vertices.push_back(ps->vertices[p[j]].head<2>());

      fill_ring(ring , vertices, node.angle*fragstart/fragments, node.v*fragstart/fragments, fact, xmid, !flip_faces); // close start
      builder.appendPolygon(ring);

      fill_ring(ring, vertices, node.angle*fragend/fragments   , node.v*fragend/fragments  , fact, xmid, flip_faces); // close end
      builder.appendPolygon(ring);
    }
  }

  for (const auto& o : poly.outlines()) {
    std::vector<Vector3d> rings[2];
    rings[0].resize(o.vertices.size());
    rings[1].resize(o.vertices.size());

    double xmid=NAN;

    if(node.method == "centered") {
      double xmin, xmax;
      xmin=xmax=o.vertices[0][0];
      for(const auto &v : o.vertices) {
        if(v[0] < xmin) xmin=v[0];	    
        if(v[0] > xmax) xmax=v[0];	    
      }
      xmid=(xmin+xmax)/2;
    }  
    Vector3d dv = node.v*fragstart/fragments;
    double a;
    if (node.angle == 360 && node.v.norm() == 0) a=180;
     else a = fragstart * node.angle / fragments;

    fill_ring(rings[fragstart % 2 ], o.vertices, a, dv,fact,  xmid, flip_faces); // first ring

    for (unsigned int j = fragstart; j < fragend; ++j) {
      dv = node.v*(j+1)/fragments;
      if (node.angle == 360 && node.v.norm() == 0) a = 180 - ((j + 1) % fragments) * 360.0 / fragments; // start on the -X axis, for legacy support
      else a = (j + 1) * node.angle / fragments; // start on the X axis
      fill_ring(rings[(j + 1) % 2], o.vertices, a, dv, fact, xmid, flip_faces);
      for (size_t i = 0; i < o.vertices.size(); ++i) {
        builder.appendPolygon({
                rings[j % 2][(i + 1) % o.vertices.size()],
                rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
                rings[j % 2][i]
        });                

        builder.appendPolygon({
                rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
                rings[(j + 1) % 2][i],
                rings[j % 2][i]
        });
      }
    }
  }
  }
  return builder.build();
}

std::shared_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly){

  if (node.angle == 0) return nullptr;
  double min_x = 0;
  double max_x = 0;
  double min_y = 0;
  double max_y = 0;
  unsigned int fragments = 0;
  for (const auto& o : poly.outlines()) {
    for (const auto& v : o.vertices) {
      min_x = fmin(min_x, v[0]);
      max_x = fmax(max_x, v[0]);
      min_y = fmin(min_y, v[1]);
      max_y = fmax(max_y, v[1]);
    }
  }

  if (max_x > 0 && min_x < 0){
    LOG(message_group::Error, "all points for rotate_extrude() must have the same X coordinate sign (range is %1$.2f -> %2$.2f)", min_x, max_x);
    return nullptr;
  }
  fragments = (unsigned int)std::ceil(fmax(Calc::get_fragments_from_r(max_x - min_x, node.angle, node.fn, node.fs, node.fa) * std::abs(node.angle) / 360, 1));
  bool flip_faces = (min_x >= 0 && node.angle > 0 && node.angle != 360) || (min_x < 0 && (node.angle < 0 || node.angle == 360));

  // check if its save to extrude
  bool safe=true;
  do
  {
    if(node.angle < 300) break;	  
    if(node.v.norm() == 0) break;
    if(node.v[2]/(node.angle/360.0)  >  (max_y-min_y) * 1.5) break;
    safe=false;

  } while(false);
  if(safe) return rotatePolygonSub(node, poly, fragments, 0, fragments, flip_faces);	

  // now create a fragment splitting plan
  int splits=ceil(node.angle/300.0);
  int fragstart=0,fragend;
  std::shared_ptr<ManifoldGeometry> result = nullptr;

  for(int i=0;i<splits;i++) {
    fragend=fragstart+(fragments/splits)+1;    	 
    if(fragend > fragments) fragend=fragments;
    std::unique_ptr<Geometry> part_u =rotatePolygonSub(node, poly, fragments, fragstart, fragend, flip_faces);	
    std::shared_ptr<Geometry> part_s = std::move(part_u);
    std::shared_ptr<const ManifoldGeometry>term = ManifoldUtils::createManifoldFromGeometry(part_s);
    if(result == nullptr) result = std::make_shared<ManifoldGeometry>(*term);
      else *result = *result + *term;	
    fragstart=fragend-1;
  }
  return  result; 
}



