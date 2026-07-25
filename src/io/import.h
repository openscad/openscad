#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

#include "core/AST.h"
#include "core/CurveDiscretizer.h"
#include "geometry/linalg.h"

std::unique_ptr<class PolySet> import_stl(const std::string& filename, const Location& loc);
std::unique_ptr<class PolySet> import_step(const std::string& filename, const Location& loc);
std::unique_ptr<class PolySet> import_obj(const std::string& filename, const Location& loc);
std::unique_ptr<class PolySet> import_off(const std::string& filename, const Location& loc);
std::unique_ptr<class PolySet> import_amf(const std::string&, const Location& loc);
std::unique_ptr<class PolySet> import_3mf(const std::string&, const Location& loc);

std::unique_ptr<class Polygon2d> import_svg(CurveDiscretizer discretizer, const std::string& filename,
                                            const boost::optional<std::string>& id,
                                            const boost::optional<std::string>& layer, const double dpi,
                                            const bool center, const Location& loc, bool stroke,
                                            const boost::optional<Color4f>& colorFilter = boost::none);

// Returns the distinct outline colors found in an SVG file, in first-appearance order,
// resolved through the same fill/stroke -> Color4f logic used by import_svg().
// Used to drive osimport(..., split_by_color=True).
std::vector<Color4f> import_svg_list_colors(CurveDiscretizer discretizer, const std::string& filename,
                                            const boost::optional<std::string>& id,
                                            const boost::optional<std::string>& layer, bool stroke,
                                            const Location& loc);
#ifdef ENABLE_CDR
std::unique_ptr<class Polygon2d> import_cdr(CurveDiscretizer discretizer, const std::string& filename,
                                            const Location& loc);
#endif

#ifdef ENABLE_CGAL
std::unique_ptr<class CGALNefGeometry> import_nef3(const std::string& filename, const Location& loc);
#endif

class Value import_json(const std::string& filename, class EvaluationSession *session,
                        const Location& loc);
