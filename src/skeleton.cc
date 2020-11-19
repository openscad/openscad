#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

#include <boost/shared_ptr.hpp>
#include <cmath>
#include <algorithm>

#include "GeometryUtils.h"
#include "skeleton.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel         CGAL_KERNEL;
typedef CGAL_KERNEL::Point_2                                        CGAL_Point_2;
typedef CGAL::Polygon_2<CGAL_KERNEL>                                CGAL_Polygon_2;
typedef CGAL::Polygon_with_holes_2<CGAL_KERNEL>                     CGAL_Polygon_with_holes_2;
typedef CGAL::Straight_skeleton_2<CGAL_KERNEL>                      CGAL_Ss;
typedef boost::shared_ptr<CGAL_Ss>                                  CGAL_SsPtr;

PolySet *test9(Polygon2d::Outlines2d outlines) {
    
    typedef typename CGAL_Ss::Vertex_const_handle     Vertex_const_handle ;
    typedef typename CGAL_Ss::Vertex_const_iterator   Vertex_const_iterator ;
    typedef typename CGAL_Ss::Halfedge_const_handle   Halfedge_const_handle ;
    typedef typename CGAL_Ss::Halfedge_const_iterator Halfedge_const_iterator ;
    typedef typename CGAL_Ss::Face_const_handle       Face_const_handle ;
    typedef typename CGAL_Ss::Face_const_iterator     Face_const_iterator ;

    auto *ps = new PolySet(3);

    CGAL_Polygon_with_holes_2 shape;
    // assume that the first polygon is the outline and the rest are holes
    for (size_t oo=0; oo<outlines.size(); oo++) {
        CGAL_Polygon_2 poly;
        std::cout << "Polygon " << oo << "\n";
        for (auto p : outlines[oo].vertices) {
            poly.push_back(CGAL_Point_2(p[0], p[1]));
            std::cout << "Pushed point (" << p[0] << ", " << p[1] << ")\n";
        }
        if (oo==0) {
            shape = CGAL_Polygon_with_holes_2(poly);
        } else {
            shape.add_hole(poly);
        }
    }

    
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

    return ps;
}
