#include <CGAL/Gmpq.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Bounded_kernel.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

typedef CGAL::Lazy_exact_nt<CGAL::Gmpq> FT;
typedef CGAL::Simple_cartesian<FT> Kernel;
typedef CGAL::Bounded_kernel<Kernel> Extended_kernel;

// typedef CGAL::Exact_predicates_inexact_constructions_kernel MyKernel;
// typedef CGAL::Bounded_kernel<CGAL::Extended_cartesian<CGAL::Gmpq> > MyKernel;
typedef CGAL::Extended_cartesian<CGAL::Gmpq> MyKernel;

typedef CGAL::Nef_polyhedron_2<MyKernel> Nef_polyhedron;
typedef Nef_polyhedron::Point Point;
typedef Nef_polyhedron::Explorer Explorer;
typedef Explorer::Vertex_const_iterator Vertex_const_iterator;
typedef Explorer::Face_const_iterator Face_const_iterator;
typedef Explorer::Hole_const_iterator Hole_const_iterator;
typedef Explorer::Halfedge_around_face_const_circulator Halfedge_around_face_const_circulator;
typedef Explorer::Vertex_const_handle Vertex_const_handle;
typedef Explorer::Vertex_handle Vertex_handle;
typedef Explorer::Halfedge_const_handle Halfedge_const_handle;


void print(const Nef_polyhedron &RST)
{
	//   CGAL::set_pretty_mode(std::cout);
	//   std::cout << RST << std::endl;

	Explorer explorer = RST.explorer();
	explorer.print_statistics();

	CGAL::Object_index<Vertex_const_handle> VI(explorer.vertices_begin(), explorer.vertices_end(), 'v');

	for (Vertex_const_iterator vit = explorer.vertices_begin(); vit!=explorer.vertices_end(); ++vit) {
		std::cout << VI(vit, true);
		if (explorer.is_standard(vit)) std::cout << " [ " << to_double(explorer.point(vit).x()) << ", " << to_double(explorer.point(vit).y()) << " ]";
		std::cout << "\n";
	}

	for (Face_const_iterator fit = explorer.faces_begin();
			 fit != explorer.faces_end();
			 fit++) {
		std::cout << "explorer.mark(explorer.faces_begin()) "  << ((explorer.mark(fit))? "is part of polygon" :  "is not part of polygon") << std::endl;

		if (fit->halfedge() == Halfedge_const_handle()) std::cout << "X\n";
		else {
			Halfedge_around_face_const_circulator hafc = explorer.face_cycle(fit), done(hafc);
			do {
				Vertex_const_handle vh = explorer.target(hafc);
				std::cout << VI(vh, true) << " ";
				if (explorer.is_standard(vh)) std::cout << "[" << to_double(explorer.point(vh).x()) << ", " << to_double(explorer.point(vh).y()) << "],  " ;
				hafc++;
			} while(hafc != done);
			std::cout << std::endl;
		}
		if (fit->fc_begin() == fit->fc_end()) {
			std::cout << "Y\n";
		}
		else {
			for (Hole_const_iterator hit = explorer.holes_begin(fit); hit != explorer.holes_end(fit); hit++){
				std::cout << "Hole: ";
				Halfedge_around_face_const_circulator hafc(hit), done(hit);
				do{
					Vertex_const_handle vh = explorer.target(hafc);
					std::cout << VI(vh, true) << " ";
					if (explorer.is_standard(vh)) std::cout << "[" << to_double(explorer.point(vh).x()) << ", " << to_double(explorer.point(vh).y()) << "],  " ;
					hafc++;
				}while(hafc != done);
				std::cout << std::endl;
			}
		}
	}

}

int main()
{
	Point tris[15] = {
		Point(45,100), Point(45,50), Point(60,80),
		Point(140,0), Point(45,50), Point(0,0),
		Point(0,140), Point(0,0), Point(50,140),
		Point(45,100), Point(50,140), Point(0,0), 
		Point(45,100), Point(0,0), Point(45,50), 
	};



	std::list<std::pair<Point*,Point*> > polylines;
	polylines.push_back(std::make_pair(tris+0, tris+3));
	polylines.push_back(std::make_pair(tris+3, tris+6));
	polylines.push_back(std::make_pair(tris+6, tris+9));
	polylines.push_back(std::make_pair(tris+9, tris+12));
	polylines.push_back(std::make_pair(tris+12, tris+15));

	Nef_polyhedron RST(polylines.begin(), polylines.end(), Nef_polyhedron::POLYGONS);
	print(RST);

	Nef_polyhedron N;
	for (std::list<std::pair<Point*,Point*> >::const_iterator iter = polylines.begin();
			 iter != polylines.end();
			 iter++) {
		N += Nef_polyhedron(iter->first, iter->second, Nef_polyhedron::INCLUDED);
	}
	print(N);

	return 0;
}
