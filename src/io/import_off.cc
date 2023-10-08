#include "import.h"
#include "PolySet.h"
#include "printutils.h"
#include "AST.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

PolySet *import_off(const std::string& filename, const Location& loc)
{
  auto *p = new PolySet(3);
#ifdef ENABLE_CGAL
  CGAL_Polyhedron poly;
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (!file.good()) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename, loc.firstLine());
  } else {
    file >> poly;
    file.close();
    CGALUtils::createPolySetFromPolyhedron(poly, *p);
  }
#else
  LOG(message_group::Warning, "OFF import requires CGAL, import() at line %2$d", filename, loc.firstLine());
#endif // ifdef ENABLE_CGAL
  return p;
}

