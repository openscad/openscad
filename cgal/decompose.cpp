#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <sstream>
#include <iostream>
#include <locale.h>

#include "cgalutils.h"
#include "memory.h"
#include "export.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

using namespace CGALUtils;
namespace fs = boost::filesystem;

// Nef polyhedron are using CGAL_Kernel3 (Cartesian<Gmpq>)
// Triangulation will use Epick
typedef CGAL::Epick K;
typedef CGAL::Polyhedron_3<K> PolyhedronK;

#include <boost/algorithm/string.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope
std::vector<Color4f> colors = boost::assign::list_of
  (Color4f(240, 248, 255))
  (Color4f(250, 235, 215))
  (Color4f(0, 255, 255))
  (Color4f(127, 255, 212))
  (Color4f(240, 255, 255))
  (Color4f(245, 245, 220))
  (Color4f(255, 228, 196))
  (Color4f(0, 0, 0))
  (Color4f(255, 235, 205))
  (Color4f(0, 0, 255))
  (Color4f(138, 43, 226))
  (Color4f(165, 42, 42))
  (Color4f(222, 184, 135))
  (Color4f(95, 158, 160))
  (Color4f(127, 255, 0))
  (Color4f(210, 105, 30))
  (Color4f(255, 127, 80))
  (Color4f(100, 149, 237))
  (Color4f(255, 248, 220))
  (Color4f(220, 20, 60))
  (Color4f(0, 255, 255))
  (Color4f(0, 0, 139))
  (Color4f(0, 139, 139))
  (Color4f(184, 134, 11))
  (Color4f(169, 169, 169))
  (Color4f(0, 100, 0))
  (Color4f(169, 169, 169))
  (Color4f(189, 183, 107))
  (Color4f(139, 0, 139))
  (Color4f(85, 107, 47))
  (Color4f(255, 140, 0))
  (Color4f(153, 50, 204))
  (Color4f(139, 0, 0))
  (Color4f(233, 150, 122))
  (Color4f(143, 188, 143))
  (Color4f(72, 61, 139))
  (Color4f(47, 79, 79))
  (Color4f(47, 79, 79))
  (Color4f(0, 206, 209))
  (Color4f(148, 0, 211))
  (Color4f(255, 20, 147))
  (Color4f(0, 191, 255))
  (Color4f(105, 105, 105))
  (Color4f(105, 105, 105))
  (Color4f(30, 144, 255))
  (Color4f(178, 34, 34))
  (Color4f(255, 250, 240))
  (Color4f(34, 139, 34))
  (Color4f(255, 0, 255))
  (Color4f(220, 220, 220))
  (Color4f(248, 248, 255))
  (Color4f(255, 215, 0))
  (Color4f(218, 165, 32))
  (Color4f(128, 128, 128))
  (Color4f(0, 128, 0))
  (Color4f(173, 255, 47))
  (Color4f(128, 128, 128))
  (Color4f(240, 255, 240))
  (Color4f(255, 105, 180))
  (Color4f(205, 92, 92))
  (Color4f(75, 0, 130))
  (Color4f(255, 255, 240))
  (Color4f(240, 230, 140))
  (Color4f(230, 230, 250))
  (Color4f(255, 240, 245))
  (Color4f(124, 252, 0))
  (Color4f(255, 250, 205))
  (Color4f(173, 216, 230))
  (Color4f(240, 128, 128))
  (Color4f(224, 255, 255))
  (Color4f(250, 250, 210))
  (Color4f(211, 211, 211))
  (Color4f(144, 238, 144))
  (Color4f(211, 211, 211))
  (Color4f(255, 182, 193))
  (Color4f(255, 160, 122))
  (Color4f(32, 178, 170))
  (Color4f(135, 206, 250))
  (Color4f(119, 136, 153))
  (Color4f(119, 136, 153))
  (Color4f(176, 196, 222))
  (Color4f(255, 255, 224))
  (Color4f(0, 255, 0))
  (Color4f(50, 205, 50))
  (Color4f(250, 240, 230))
  (Color4f(255, 0, 255))
  (Color4f(128, 0, 0))
  (Color4f(102, 205, 170))
  (Color4f(0, 0, 205))
  (Color4f(186, 85, 211))
  (Color4f(147, 112, 219))
  (Color4f(60, 179, 113))
  (Color4f(123, 104, 238))
  (Color4f(0, 250, 154))
  (Color4f(72, 209, 204))
  (Color4f(199, 21, 133))
  (Color4f(25, 25, 112))
  (Color4f(245, 255, 250))
  (Color4f(255, 228, 225))
  (Color4f(255, 228, 181))
  (Color4f(255, 222, 173))
  (Color4f(0, 0, 128))
  (Color4f(253, 245, 230))
  (Color4f(128, 128, 0))
  (Color4f(107, 142, 35))
  (Color4f(255, 165, 0))
  (Color4f(255, 69, 0))
  (Color4f(218, 112, 214))
  (Color4f(238, 232, 170))
  (Color4f(152, 251, 152))
  (Color4f(175, 238, 238))
  (Color4f(219, 112, 147))
  (Color4f(255, 239, 213))
  (Color4f(255, 218, 185))
  (Color4f(205, 133, 63))
  (Color4f(255, 192, 203))
  (Color4f(221, 160, 221))
  (Color4f(176, 224, 230))
  (Color4f(128, 0, 128))
  (Color4f(255, 0, 0))
  (Color4f(188, 143, 143))
  (Color4f(65, 105, 225))
  (Color4f(139, 69, 19))
  (Color4f(250, 128, 114))
  (Color4f(244, 164, 96))
  (Color4f(46, 139, 87))
  (Color4f(255, 245, 238))
  (Color4f(160, 82, 45))
  (Color4f(192, 192, 192))
  (Color4f(135, 206, 235))
  (Color4f(106, 90, 205))
  (Color4f(112, 128, 144))
  (Color4f(112, 128, 144))
  (Color4f(255, 250, 250))
  (Color4f(0, 255, 127))
  (Color4f(70, 130, 180))
  (Color4f(210, 180, 140))
  (Color4f(0, 128, 128))
  (Color4f(216, 191, 216))
  (Color4f(255, 99, 71))
  (Color4f(0, 0, 0, 0))
  (Color4f(64, 224, 208))
  (Color4f(238, 130, 238))
  (Color4f(245, 222, 179))
  (Color4f(255, 255, 255))
  (Color4f(245, 245, 245))
  (Color4f(255, 255, 0))
  (Color4f(154, 205, 50));

#include <boost/unordered_set.hpp>
#include <CGAL/convex_hull_3.h>
template<typename Polyhedron>
bool is_weakly_convex(Polyhedron const& p) {
  for (typename Polyhedron::Edge_const_iterator i = p.edges_begin(); i != p.edges_end(); ++i) {
    typename Polyhedron::Plane_3 p(i->opposite()->vertex()->point(), i->vertex()->point(), i->next()->vertex()->point());
    if (p.has_on_positive_side(i->opposite()->next()->vertex()->point()) &&
        CGAL::squared_distance(p, i->opposite()->next()->vertex()->point()) > 1e-8) {
      return false;
    }
  }
  // Also make sure that there is only one shell:
  boost::unordered_set<typename Polyhedron::Facet_const_handle, typename CGAL::Handle_hash_function> visited;
  // c++11
  // visited.reserve(p.size_of_facets());
  
  std::queue<typename Polyhedron::Facet_const_handle> to_explore;
  to_explore.push(p.facets_begin()); // One arbitrary facet
  visited.insert(to_explore.front());
  
  while (!to_explore.empty()) {
    typename Polyhedron::Facet_const_handle f = to_explore.front();
    to_explore.pop();
    typename Polyhedron::Facet::Halfedge_around_facet_const_circulator he, end;
    end = he = f->facet_begin();
    CGAL_For_all(he,end) {
      typename Polyhedron::Facet_const_handle o = he->opposite()->facet();
      
      if (!visited.count(o)) {
        visited.insert(o);
        to_explore.push(o);
      }
    }
  }
  
  return visited.size() == p.size_of_facets();
}

class Shell_explorer
{
public:
  std::vector<K::Point_3> vertices;

  Shell_explorer() {}
  void visit(CGAL_Nef_polyhedron3::Vertex_const_handle v) {
    vertices.push_back(K::Point_3(to_double(v->point()[0]),
                                            to_double(v->point()[1]),
                                            to_double(v->point()[2])));
  }
  void visit(CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
  void visit(CGAL_Nef_polyhedron3::Halffacet_const_handle ) {}
  void visit(CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
  void visit(CGAL_Nef_polyhedron3::SHalfloop_const_handle ) {}
  void visit(CGAL_Nef_polyhedron3::SFace_const_handle ) {}
};

template<class Output>
void decompose(const CGAL_Nef_polyhedron3 *N, Output out_iter)
{
  int parts = 0;
  assert(N);
  CGAL_Polyhedron poly;
  if (N->is_simple()) {
    nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>(*N, poly);
  }
  if (is_weakly_convex(poly)) {
    PRINTD("Minkowski: Object is convex and Nef");
    PolyhedronK poly2;
    CGALUtils::copyPolyhedron(poly, poly2);
    *out_iter++ = poly2;
    return;
  }
  else {
    PRINTD("Minkowski: Object is nonconvex Nef, decomposing...");
    CGAL_Nef_polyhedron3 decomposed_nef = *N;
    CGAL::convex_decomposition_3(decomposed_nef);
    
    // the first volume is the outer volume, which ignored in the decomposition
    CGAL_Nef_polyhedron3::Volume_const_iterator ci = ++decomposed_nef.volumes_begin();
    // Convert each convex volume to a Polyhedron
    for(; ci != decomposed_nef.volumes_end(); ++ci) {
      if(ci->mark()) {
        //        CGAL_Polyhedron poly;
        //        decomposed_nef.convert_inner_shell_to_polyhedron(ci->shells_begin(), poly);
        //        P.push_back(poly);


        auto s = CGAL_Nef_polyhedron3::SFace_const_handle(ci->shells_begin());

        CGAL_Nef_polyhedron3::SFace_const_iterator sf = ci->shells_begin();
        Shell_explorer SE;
        decomposed_nef.visit_shell_objects(CGAL_Nef_polyhedron3::SFace_const_handle(sf),SE);

        PolyhedronK poly;
        CGAL::convex_hull_3(SE.vertices.begin(), SE.vertices.end(), poly);
        *out_iter++ = poly;
        parts++;
      }
    }
    
    PRINTDB("Minkowski: decomposed into %d convex parts", parts);
  }
}

Geometry const * minkowskitest(const Geometry::Geometries &children)
{
  CGAL::Timer t,t_tot;
  assert(children.size() >= 2);
  // Iterate over children, perform pairwise minkowski on children:
  //   operands = [ch, ch+1]
  Geometry::Geometries::const_iterator minkowski_ch_it = children.begin();
  t_tot.start();
  Geometry const *operands[2] = {minkowski_ch_it->second.get(), NULL};
  try {
    while (++minkowski_ch_it != children.end()) {
      operands[1] = minkowski_ch_it->second.get();

      std::vector<PolyhedronK> convexP[2]; // Stores decomposed operands 
      std::list<PolyhedronK> result_parts;

      for (int i = 0; i < 2; i++) {
        shared_ptr<const CGAL_Nef_polyhedron> N;
        if (const PolySet *ps = dynamic_cast<const PolySet *>(operands[i])) {
          if (ps->is_convex()) {
            PRINTDB("Minkowski: child %d is convex and PolySet", i);
            PolyhedronK poly;
            CGALUtils::createPolyhedronFromPolySet(*ps, poly);
            convexP[i].push_back(poly);
          }
          else {
            PRINTDB("Minkowski: child %d is nonconvex PolySet, transforming to Nef", i);
            N.reset(createNefPolyhedronFromGeometry(*ps));
          }
        }
        else if (const CGAL_Nef_polyhedron *n = dynamic_cast<const CGAL_Nef_polyhedron *>(operands[i])) {
          CGAL_Polyhedron poly;
          if (n->p3->is_simple()) {
            nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>(*n->p3, poly);
            // FIXME: Can we calculate weakly_convex on a PolyhedronK instead?
            if (is_weakly_convex(poly)) {
              PRINTDB("Minkowski: child %d is convex and Nef", i);
              PolyhedronK poly2;
              CGALUtils::copyPolyhedron(poly, poly2);
              convexP[i].push_back(poly2);
            }
            else {
              PRINTDB("Minkowski: child %d is nonconvex Nef",i);
              N.reset(n);
            }
          }
          else throw 0; // We cannot handle this, fall back to CGAL's minkowski
        }

        // If not convex...
        if (N && N->p3) {
          PRINTD("Decomposing...");
          decompose(N->p3.get(), std::back_inserter(convexP[i]));
        }

        PRINTD("Hulling convex parts...");
        std::vector<K::Point_3> points[2];
        std::vector<K::Point_3> minkowski_points;
        
        // For each permutation of convex operands..
        BOOST_FOREACH(const PolyhedronK &p0, convexP[0]) {
          BOOST_FOREACH(const PolyhedronK &p1, convexP[1]) {
            t.start();
            
            // Create minkowski pointcloud
            minkowski_points.clear();
            minkowski_points.reserve(p0.size_of_vertices() * p0.size_of_vertices());
            BOOST_FOREACH(const K::Point_3 &p0p, std::make_pair(p0.points_begin(), p0.points_end())) {
              BOOST_FOREACH(const K::Point_3 &p1p, std::make_pair(p1.points_begin(), p1.points_end())) {
                minkowski_points.push_back(p0p+(p1p-CGAL::ORIGIN));
              }
            }
            
            t.stop();
            
            // Ignore empty volumes
            if (minkowski_points.size() <= 3) continue;
            
            // Hull point cloud
            PolyhedronK result;
            PRINTDB("Minkowski: Point cloud creation (%d ⨉ %d -> %d) took %f ms",
                    points[0].size() % points[1].size() % minkowski_points.size() % (t.time()*1000));
            t.reset();
            t.start();
            CGAL::convex_hull_3(minkowski_points.begin(), minkowski_points.end(), result);
            
            std::vector<K::Point_3> strict_points;
            strict_points.reserve(minkowski_points.size());
            
            for (PolyhedronK::Vertex_iterator i = result.vertices_begin(); i != result.vertices_end(); ++i) {
              K::Point_3 const &p = i->point();
              
              PolyhedronK::Vertex::Halfedge_handle h,e;
              h = i->halfedge();
              e = h;
              bool collinear = false;
              bool coplanar = true;
              
              do {
                K::Point_3 const& q = h->opposite()->vertex()->point();
                if (coplanar && !CGAL::coplanar(p,q,
                                                h->next_on_vertex()->opposite()->vertex()->point(),
                                                h->next_on_vertex()->next_on_vertex()->opposite()->vertex()->point())) {
                  coplanar = false;
                }
                
                
                for (PolyhedronK::Vertex::Halfedge_handle j = h->next_on_vertex();
                     j != h && !collinear && ! coplanar;
                     j = j->next_on_vertex()) {
                  
                  K::Point_3 const& r = j->opposite()->vertex()->point();
                  if (CGAL::collinear(p,q,r)) {
                    collinear = true;
                  }
                }
                
                h = h->next_on_vertex();
              } while (h != e && !collinear);
              
              if (!collinear && !coplanar) strict_points.push_back(p);
            }
            
            result.clear();
            CGAL::convex_hull_3(strict_points.begin(), strict_points.end(), result);
            
            t.stop();
            PRINTDB("Minkowski: Computing convex hull took %f s", t.time());
            t.reset();
            
            result_parts.push_back(result);
          }
        }
      }
      
      if (minkowski_ch_it != boost::next(children.begin())) delete operands[0];
      
      if (result_parts.size() == 1) {
        PolySet *ps = new PolySet(3,true);
        createPolySetFromPolyhedron(*result_parts.begin(), *ps);
        operands[0] = ps;
      } else if (!result_parts.empty()) {
        t.start();
        PRINTDB("Minkowski: Computing union of %d parts",result_parts.size());
        Geometry::Geometries fake_children;
        for (const auto &polyhedron : result_parts) {
          PolySet ps(3,true);
          createPolySetFromPolyhedron(polyhedron, ps);
          fake_children.push_back(std::make_pair((const AbstractNode*)NULL,
                                                 shared_ptr<const Geometry>(createNefPolyhedronFromGeometry(ps))));
        }
        CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(fake_children, OPENSCAD_UNION);
        t.stop();
        if (N) PRINTDB("Minkowski: Union done: %f s",t.time());
        else PRINTDB("Minkowski: Union failed: %f s",t.time());
        t.reset();
        operands[0] = N;
      } else {
        operands[0] = new CGAL_Nef_polyhedron();
      }
    }
    
    t_tot.stop();
    PRINTDB("Minkowski: Total execution time %f s", t_tot.time());
    t_tot.reset();
    return operands[0];
  }
  catch (...) {
    // If anything throws we simply fall back to Nef Minkowski
    PRINTD("Minkowski: Falling back to Nef Minkowski");
    
    CGAL_Nef_polyhedron *N = applyOperator(children, OPENSCAD_MINKOWSKI);
    return N;
  }
}

#define STL_FACET_NUMBYTES 4*3*4+2
// as there is no 'float32_t' standard, we assume the systems 'float'
// is a 'binary32' aka 'single' standard IEEE 32-bit floating point type
union stl_facet {
	uint8_t data8[ STL_FACET_NUMBYTES ];
	uint32_t data32[4*3];
	struct facet_data {
	  float i, j, k;
	  float x1, y1, z1;
	  float x2, y2, z2;
	  float x3, y3, z3;
	  uint16_t attribute_byte_count;
	} data;
};

void uint32_byte_swap( uint32_t &x )
{
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
	x = __builtin_bswap32( x );
#elif defined(__clang__)
	x = __builtin_bswap32( x );
#elif defined(_MSC_VER)
	x = _byteswap_ulong( x );
#else
	uint32_t b1 = ( 0x000000FF & x ) << 24;
	uint32_t b2 = ( 0x0000FF00 & x ) << 8;
	uint32_t b3 = ( 0x00FF0000 & x ) >> 8;
	uint32_t b4 = ( 0xFF000000 & x ) >> 24;
	x = b1 | b2 | b3 | b4;
#endif
}

void read_stl_facet( std::ifstream &f, stl_facet &facet )
{
	f.read( (char*)facet.data8, STL_FACET_NUMBYTES );
#ifdef BOOST_BIG_ENDIAN
	for ( int i = 0; i < 12; i++ ) {
		uint32_byte_swap( facet.data32[ i ] );
	}
	// we ignore attribute byte count
#endif
}

PolySet *import_stl(const std::string &filename)
{
  PolySet *p = new PolySet(3);

  // Open file and position at the end
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
  if (!f.good()) {
    PRINTB("WARNING: Can't open import file '%s'.", filename);
    return NULL;
  }

  boost::regex ex_sfe("solid|facet|endloop");
  boost::regex ex_outer("outer loop");
  boost::regex ex_vertex("vertex");
  boost::regex ex_vertices("\\s*vertex\\s+([^\\s]+)\\s+([^\\s]+)\\s+([^\\s]+)");

  bool binary = false;
  std::streampos file_size = f.tellg();
  f.seekg(80);
  if (f.good() && !f.eof()) {
    uint32_t facenum = 0;
    f.read((char *)&facenum, sizeof(uint32_t));
#ifdef BOOST_BIG_ENDIAN
    uint32_byte_swap( facenum );
#endif
    if (file_size ==  static_cast<std::streamoff>(80 + 4 + 50*facenum)) {
      binary = true;
    }
  }
  f.seekg(0);

  char data[5];
  f.read(data, 5);
  if (!binary && !f.eof() && f.good() && !memcmp(data, "solid", 5)) {
    int i = 0;
    double vdata[3][3];
    std::string line;
    std::getline(f, line);
    while (!f.eof()) {
      std::getline(f, line);
      boost::trim(line);
      if (boost::regex_search(line, ex_sfe)) {
        continue;
      }
      if (boost::regex_search(line, ex_outer)) {
        i = 0;
        continue;
      }
      boost::smatch results;
      if (boost::regex_search(line, results, ex_vertices)) {
        try {
          for (int v=0;v<3;v++) {
            vdata[i][v] = boost::lexical_cast<double>(results[v+1]);
          }
        }
        catch (const boost::bad_lexical_cast &blc) {
          PRINTB("WARNING: Can't parse vertex line '%s'.", line);
          i = 10;
          continue;
        }
        if (++i == 3) {
          p->append_poly();
          p->append_vertex(vdata[0][0], vdata[0][1], vdata[0][2]);
          p->append_vertex(vdata[1][0], vdata[1][1], vdata[1][2]);
          p->append_vertex(vdata[2][0], vdata[2][1], vdata[2][2]);
        }
      }
    }
  }
  else if (binary && !f.eof() && f.good())
    {
      f.ignore(80-5+4);
      while (1) {
        stl_facet facet;
        read_stl_facet( f, facet );
        if (f.eof()) break;
        p->append_poly();
        p->append_vertex(facet.data.x1, facet.data.y1, facet.data.z1);
        p->append_vertex(facet.data.x2, facet.data.y2, facet.data.z2);
        p->append_vertex(facet.data.x3, facet.data.y3, facet.data.z3);
      }
    }
  return p;
}

/*!
  file format: 
  1. polygon coordinates (x,y,z) are comma separated (+/- spaces) and 
  each coordinate is on a separate line
  2. each polygon is separated by one or more blank lines
*/
bool import_polygon(PolyholeK &polyhole, const std::string &filename)
{
  std::ifstream ifs(filename.c_str());
  if (!ifs) return false;

  std::string line;
  PolygonK polygon;
  while (std::getline(ifs, line)) {
    std::stringstream ss(line);
    double X = 0.0, Y = 0.0, Z = 0.0;
    if (!(ss >> X)) {
      //ie blank lines => flag start of next polygon 
      if (polygon.size() > 0) polyhole.push_back(polygon);
      polygon.clear();
      continue;
    }
    char c = ss.peek();  
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces before comma
    if (c == ',') {ss.read(&c, 1); c = ss.peek();} //gobble comma
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces after comma
    if (!(ss >> Y)) {
      std::cerr << "Y error\n";
      return false;
    }
    c = ss.peek();
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces before comma
    if (c == ',') {ss.read(&c, 1); c = ss.peek();} //gobble comma
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces after comma
    if (!(ss >> Z)) {
      std::cerr << "Z error\n";
      return false;
    }
    polygon.push_back(Vertex3K(X, Y, Z));
  }
  if (polygon.size() > 0) polyhole.push_back(polygon);
  ifs.close();
  return true;
}
//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{

  OpenSCAD::debug = "decompose";

  PolySet *ps = NULL;
  CGAL_Nef_polyhedron *N = NULL;
  if (argc == 2) {
    std::string filename(argv[1]);
    std::string suffix = fs::path(filename).extension().generic_string();
    if (suffix == ".stl") {
      if (!(ps = import_stl(filename))) {
        std::cerr << "Error importing STL " << filename << std::endl;
        exit(1);
      }
      std::cerr << "Imported " << ps->numPolygons() << " polygons" << std::endl;
    }
    else if (suffix == ".nef3") {
      N = new CGAL_Nef_polyhedron(new CGAL_Nef_polyhedron3);
      std::ifstream stream(filename.c_str());
      stream >> *N->p3;
      std::cerr << "Imported Nef polyhedron" << std::endl;
    }
  }
  else {
    std::cerr << "Usage: " << argv[0] << " <file.stl> <file.stl>" << std::endl;
    exit(1);
  }

  if (ps && !N) N = createNefPolyhedronFromGeometry(*ps);

  std::vector<PolyhedronK> result;
  decompose(N->p3.get(), std::back_inserter(result));

  std::cerr << "Decomposed into " << result.size() << " convex parts" << std::endl;

  int idx = 0;
  BOOST_FOREACH(const PolyhedronK &P, result) {
    PolySet *result_ps = new PolySet(3);
    if (CGALUtils::createPolySetFromPolyhedron(P, *result_ps)) {
      std::cerr << "Error converting to PolySet\n";
    }
    else {
      std::stringstream ss;
      ss << "out" << idx++ << ".stl";
      exportFileByName(shared_ptr<const Geometry>(result_ps), OPENSCAD_STL, ss.str().c_str(), ss.str().c_str());
      std::cout << "color([" << colors[idx%147][0] << "," << colors[idx%147][1] << "," << colors[idx%147][2] << "]) " << "import(\"" << ss.str() << "\");\n";
    }
  }
  std::cerr << "Done." << std::endl;
}
