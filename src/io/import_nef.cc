#include "io/import.h"

#include <fstream>
#include <ios>
#include <memory>
#include <string>

#include "core/AST.h"
#include "utils/printutils.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/CGALNefGeometry.h"
#include <CGAL/IO/Nef_polyhedron_iostream_3.h>

std::unique_ptr<CGALNefGeometry> import_nef3(const std::string& filename, const Location& loc)
{
  // Open file and position at the end
  std::ifstream f(std::filesystem::u8path(filename), std::ios::in | std::ios::binary);
  if (!f.good()) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename,
        loc.firstLine());
    return std::make_unique<CGALNefGeometry>();
  }

  try {
    auto nef = std::make_shared<CGAL_Nef_polyhedron3>();
    f >> *nef;
    return std::make_unique<CGALNefGeometry>(nef);
  } catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Warning, "Failure trying to import '%1$s', import() at line %2$d", filename,
        loc.firstLine());
    LOG(e.what());
    return std::make_unique<CGALNefGeometry>();
  }
}

#endif  // ENABLE_CGAL
