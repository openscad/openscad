#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

#include "core/AST.h"
#include "core/CurveDiscretizer.h"

std::unique_ptr<class PolySet> import_stl(const fs::path& filename, const Location& loc);
std::unique_ptr<class PolySet> import_obj(const fs::path& filename, const Location& loc);
std::unique_ptr<class PolySet> import_off(const fs::path& filename, const Location& loc);
std::unique_ptr<class PolySet> import_3mf(const fs::path&, const Location& loc);

std::unique_ptr<class Polygon2d> import_svg(CurveDiscretizer discretizer, const fs::path& filename,
                                            const boost::optional<std::string>& id,
                                            const boost::optional<std::string>& layer, const double dpi,
                                            const bool center, const Location& loc);

#ifdef ENABLE_CGAL
std::unique_ptr<class CGALNefGeometry> import_nef3(const fs::path& filename, const Location& loc);
#endif

class Value import_json(const fs::path& filename, class EvaluationSession *session, const Location& loc);
