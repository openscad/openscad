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
		LOG("",loc.firstLine(),getFormatted("Can't open import file '%1$s', import()",filename),message_group::Warning);
	}
	else {
		file >> poly;
		file.close();
		CGALUtils::createPolySetFromPolyhedron(poly, *p);
	}
#else
	LOG("",loc.firstLine(),getFormatted("OFF import requires CGAL, import()",filename),message_group::Warning);
#endif
	return p;
}
	
