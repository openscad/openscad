
// g++ -o viewoff -lCGAL -lCGAL_Qt3 -I/opt/qt3/include/ -L/opt/qt3/lib/ -lqt viewoff.cc

#include <CGAL/Cartesian.h>
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>
#include <CGAL/IO/Polyhedron_VRML_1_ostream.h> 
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Simple_cartesian.h>
#include <fstream>
#include <iostream>
#include <CGAL/IO/Qt_widget_Nef_3.h>
#include <qapplication.h>

#define VERBOSE 0

typedef CGAL::Cartesian<CGAL::Gmpq> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef CGAL::Nef_polyhedron_3<Kernel> Nef_polyhedron;

int main(int argc, char **argv)
{
	QApplication a(argc, argv);

	for (int i=1; i<argc; i++) {
		std::ifstream inFile(argv[i]);
		if (inFile.fail()) {
			std::cerr << "unable to open file " << argv[i] << " for reading!" << std::endl;
			exit(1);
		}
#if VERBOSE
		printf("--- %s ---\n", argv[i]);
#endif
		Polyhedron P;
		CGAL::scan_OFF(inFile, P, true);
		if (inFile.bad()) {
			std::cerr << "failed reading OFF file " << argv[i] << "!" << std::endl;
			exit(1);
		}
#if VERBOSE
		printf("P: Closed:      %6s\n", P.is_closed() ? "yes" : "no");
		printf("P: Vertices:    %6d\n", (int)P.size_of_vertices());
		printf("P: Halfedges:   %6d\n", (int)P.size_of_halfedges());
		printf("P: Facets:      %6d\n", (int)P.size_of_facets());
#endif
		Nef_polyhedron NP(P);
#if VERBOSE
		printf("NP: Simple:     %6s\n", NP.is_simple() ? "yes" : "no");
		printf("NP: Valid:      %6s\n", NP.is_valid() ? "yes" : "no");
		printf("NP: Vertices:   %6d\n", (int)NP.number_of_vertices());
		printf("NP: Halfedges:  %6d\n", (int)NP.number_of_halfedges());
		printf("NP: Edges:      %6d\n", (int)NP.number_of_edges());
		printf("NP: Halffacets: %6d\n", (int)NP.number_of_halffacets());
		printf("NP: Facets:     %6d\n", (int)NP.number_of_facets());
		printf("NP: Volumes:    %6d\n", (int)NP.number_of_volumes());
#endif
		CGAL::Qt_widget_Nef_3<Nef_polyhedron>* w = new CGAL::Qt_widget_Nef_3<Nef_polyhedron>(NP);
		if (i == 1)
			a.setMainWidget(w);
		w->show();
	}

	return a.exec();
}

