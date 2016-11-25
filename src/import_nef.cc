#include "import.h"
#include "printutils.h"
#include "handle_dep.h" // handle_dep()

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

CGAL_Nef_polyhedron *import_nef3(const std::string &filename)
{
	CGAL_Nef_polyhedron *N = new CGAL_Nef_polyhedron;

	handle_dep(filename);
	// Open file and position at the end
	std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);
	if (!f.good()) {
		PRINTB("WARNING: Can't open import file '%s'.", filename);
		return N;
	}

	N->p3.reset(new CGAL_Nef_polyhedron3);
	f >> *(N->p3);
	return N;
}
#endif
