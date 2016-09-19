#include "import.h"
#include "polyset.h"
#include "printutils.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

PolySet *import_off(const std::string &filename)
{
	PolySet *p = new PolySet(3);
#ifdef ENABLE_CGAL
	CGAL_Polyhedron poly;
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if (!file.good()) {
		PRINTB("WARNING: Can't open import file '%s'.", filename);
	}
	else {
		file >> poly;
		file.close();
		bool err = CGALUtils::createPolySetFromPolyhedron(poly, *p);
	}
#else
  PRINT("WARNING: OFF import requires CGAL.");
#endif
	return p;
}
	
