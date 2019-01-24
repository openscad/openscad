#include "import.h"
#include "polyset.h"
#include "printutils.h"
#include "AST.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

PolySet *import_off(const std::string &filename, const Location &loc)
{
	PolySet *p = new PolySet(3);
#ifdef ENABLE_CGAL
	CGAL_Polyhedron poly;
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if (!file.good()) {
		PRINTB("WARNING: Can't open import file '%s', import() at line %d", filename % loc.firstLine());
	}
	else {
		file >> poly;
		file.close();
		CGALUtils::createPolySetFromPolyhedron(poly, *p);
	}
#else
	PRINTB("WARNING: OFF import requires CGAL, import() at line %d", loc.firstLine());
#endif
	return p;
}
	
