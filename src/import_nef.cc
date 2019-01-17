#include "import.h"
#include "printutils.h"
#include "AST.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

CGAL_Nef_polyhedron *import_nef3(const std::string &filename, const Location &loc)
{
	CGAL_Nef_polyhedron *N = new CGAL_Nef_polyhedron;

	// Open file and position at the end
	std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);
	if (!f.good()) {
		PRINTB("WARNING: Can't open import file '%s', import() at line %d", filename % loc.firstLine());
		return N;
	}
	
	bool succes{true};
	std::string msg="";
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	try{
		N->p3.reset(new CGAL_Nef_polyhedron3);
		f >> *(N->p3);
	} catch (const CGAL::Failure_exception &e) {
		PRINTB("WARNING: Failure trying to import '%s', import() at line %d", filename % loc.firstLine());
		PRINT(e.what());
		N = new CGAL_Nef_polyhedron;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return N;
}
#endif
