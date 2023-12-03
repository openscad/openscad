#include "io/import.h"
#include "PolySet.h"
#include "printutils.h"
#include "AST.h"
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

std::unique_ptr<PolySet> import_off(const std::string& filename, const Location& loc)
{
#ifdef ENABLE_CGAL
  CGAL_Polyhedron poly;
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (!file.good()) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename, loc.firstLine());
  } else {
    file >> poly;
    file.close();
    return CGALUtils::createPolySetFromPolyhedron(poly);
  }
#else
  LOG(message_group::Warning, "OFF import requires CGAL, import() at line %2$d", filename, loc.firstLine());
#endif // ifdef ENABLE_CGAL
  return std::make_unique<PolySet>(3);
}
