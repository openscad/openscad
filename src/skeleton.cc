#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

#include <boost/shared_ptr.hpp>
#include <cmath>
#include <algorithm>

#include "GeometryUtils.h"
#include "clipper-utils.h"
#include "skeleton.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel         CGAL_KERNEL;
typedef CGAL_KERNEL::Point_2                                        CGAL_Point_2;
typedef CGAL::Polygon_2<CGAL_KERNEL>                                CGAL_Polygon_2;
typedef CGAL::Polygon_with_holes_2<CGAL_KERNEL>                     CGAL_Polygon_with_holes_2;
typedef CGAL::Straight_skeleton_2<CGAL_KERNEL>                      CGAL_Ss;
typedef boost::shared_ptr<CGAL_Ss>                                  CGAL_SsPtr;

typedef ClipperLib::PolyTree                                        PolyTree;
typedef ClipperLib::PolyNode                                        PolyNode;

using ClipperUtils::sanitize;
using ClipperUtils::fromPolygon2d;
using ClipperUtils::fromPath;
using ClipperUtils::CLIPPER_SCALE;

CGAL_Polygon_2 to_cgal_polygon_2(const VectorOfVector2d &points) {
    CGAL_Polygon_2 poly;
    for (auto v : points) {
        poly.push_back({v[0], v[1]});
    }
    return poly;
}

// break a list of outlines into polygons with holes
std::vector<CGAL_Polygon_with_holes_2> polygons_with_holes(const Polygon2d &poly) {

    std::vector<CGAL_Polygon_with_holes_2> ret;

    PolyTree polytree = sanitize(fromPolygon2d(poly));  // how do we check if this was successful?

    std::cout << "huj222\n";

    // lambda for recursive walk through of polytree
    std::function<void (PolyNode *)> walk = [&](PolyNode *c) {
        
        std::cout << "huj huj\n";

        // start with outer path
        CGAL_Polygon_with_holes_2 c_poly(to_cgal_polygon_2(fromPath(c->Contour)));
        for (auto cc : c->Childs) {
            c_poly.add_hole(to_cgal_polygon_2(fromPath(cc->Contour)));
            for (auto ccc : cc->Childs) {
                walk(ccc);
            }
        }
        ret.push_back(c_poly);

        // DEBUG
        std::cout << "HUJHUJHUJ HUJ\n";
        std::cout << c_poly << "\n";


        return;
    };

    for (auto root_node : polytree.Childs) {
        walk(root_node);
    }

    std::cout << "RRRRR : " << ret.size() << "\n";

    return ret;
}

PolySet *test9(const Polygon2d &poly) {
    
    typedef typename CGAL_Ss::Vertex_const_handle     Vertex_const_handle ;
    typedef typename CGAL_Ss::Vertex_const_iterator   Vertex_const_iterator ;
    typedef typename CGAL_Ss::Halfedge_const_handle   Halfedge_const_handle ;
    typedef typename CGAL_Ss::Halfedge_const_iterator Halfedge_const_iterator ;
    typedef typename CGAL_Ss::Face_const_handle       Face_const_handle ;
    typedef typename CGAL_Ss::Face_const_iterator     Face_const_iterator ;

    auto *ps = new PolySet(3);

    std::cout << "huj111\n";

    for (auto shape : polygons_with_holes(poly)) {

        std::cout << "PIZDA PIZDA\n";
        std::cout << shape << "\n";

        CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);

        for (Face_const_iterator i=ss->faces_begin(); i!=ss->faces_end(); ++i ) {
            Halfedge_const_handle h0 = i->halfedge();
            Halfedge_const_handle h = h0;

            // roof and floor
            Polygon r, f;
            do {
                r.push_back({h->vertex()->point().x(), h->vertex()->point().y(), h->vertex()->time()});
                f.push_back({h->vertex()->point().x(), h->vertex()->point().y(), 0});
                h = h->next();
            } while (h != h0);

            ps->append_poly(r);
            std::reverse(f.begin(), f.end());
            ps->append_poly(f);
        }
    }

    return ps;
}

