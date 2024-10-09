#include "io/import.h"

#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include "utils/printutils.h"
#include "core/AST.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

std::unique_ptr<CGAL_Nef_polyhedron> import_nef3(const std::string& filename, const Location& loc)
{
  // Open file and position at the end
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);
  if (!f.good()) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename, loc.firstLine());
    return std::make_unique<CGAL_Nef_polyhedron>();
  }

  try {
    auto nef = std::make_shared<CGAL_Nef_polyhedron3>();
    f >> *nef;
    return std::make_unique<CGAL_Nef_polyhedron>(nef);
  } catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Warning, "Failure trying to import '%1$s', import() at line %2$d", filename, loc.firstLine());
    LOG(e.what());
    return std::make_unique<CGAL_Nef_polyhedron>();
  }
}

#endif // ENABLE_CGAL
