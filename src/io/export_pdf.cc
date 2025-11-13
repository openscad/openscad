#include "io/export.h"

#include <cassert>
#include <ostream>
#include <memory>
#include <string>
#include <cmath>

#include <Eigen/Core>

#include "core/ColorUtil.h"
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"
#include "io/export_enums.h"
#include "utils/printutils.h"
#include "utils/version_helper.h"

#ifdef ENABLE_CAIRO

#include <cairo.h>
#include <cairo-pdf.h>

constexpr inline auto FONT = "Liberation Sans";
constexpr double MARGIN = 30.0;
constexpr double PTS_IN_MM = 2.834645656693;

std::string get_cairo_version()
{
  return OpenSCAD::get_version(CAIRO_VERSION_STRING, cairo_version_string());
}

namespace {

// Dimensions in pts per PDF standard, used by ExportPDF
// See also: https://www.prepressure.com/library/paper-size
// rows map to paperSizes enums
// columns are Width, Height
const int paperDimensions[7][2] = {
  {298, 420},   // A6
  {420, 595},   // A5
  {595, 842},   // A4
  {842, 1190},  // A3
  {612, 792},   // Letter
  {612, 1008},  // Legal
  {792, 1224},  // Tabloid
};

void draw_text(const char *text, cairo_t *cr, double x, double y, double fontSize)
{
  cairo_select_font_face(cr, FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, fontSize);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, text);
}

double mm_to_points(double mm) { return mm * PTS_IN_MM; }

double points_to_mm(double pts) { return pts / PTS_IN_MM; }

void draw_grid(cairo_t *cr, double left, double right, double bottom, double top, double gridSize)
{
  if (gridSize < 1.) gridSize = 2.0;
  const double darkerLine = 0.36;
  const double lightLine = 0.24;
  const int major = (gridSize > 10.0 ? gridSize : int(10.0 / gridSize));

  double pts = 0.0;  // for iteration across page

  // Bounds are margins in points.
  // Compute Xrange in units of gridSize
  const int Xstart = ceil(points_to_mm(left) / gridSize);
  const int Xstop = floor(points_to_mm(right) / gridSize);
  // Draw Horizontal lines
  for (int i = Xstart; i < Xstop + 1; i++) {
    if (i % major) {
      cairo_set_line_width(cr, lightLine);
      cairo_set_source_rgba(cr, 0., 0., 0., 0.48);
    } else {
      cairo_set_line_width(cr, darkerLine);
      cairo_set_source_rgba(cr, 0., 0., 0., 0.6);
    }
    pts = mm_to_points(i * gridSize);
    cairo_move_to(cr, pts, top);
    cairo_line_to(cr, pts, bottom);
    cairo_stroke(cr);
  }
  // Compute Yrange in units of gridSize
  const int Ystart = ceil(points_to_mm(top) / gridSize);
  const int Ystop = floor(points_to_mm(bottom) / gridSize);
  // Draw vertical lines
  for (int i = Ystart; i < Ystop + 1; i++) {
    if (i % major) {
      cairo_set_line_width(cr, lightLine);
      cairo_set_source_rgba(cr, 0., 0., 0., 0.4);
    } else {
      cairo_set_line_width(cr, darkerLine);
      cairo_set_source_rgba(cr, 0., 0., 0., 0.6);
    }
    pts = mm_to_points(i * gridSize);
    cairo_move_to(cr, left, pts);
    cairo_line_to(cr, right, pts);
    cairo_stroke(cr);
  }
}

// New draw_axes (renamed from axis since it draws both).
void draw_axes(cairo_t *cr, double left, double right, double bottom, double top)
{
  const double darkerLine = 0.36;
  const double offset = mm_to_points(5.);
  double pts = 0.;  // for iteration across page

  cairo_set_line_width(cr, darkerLine);
  cairo_set_source_rgba(cr, 0., 0., 0., 0.6);

  // Axes proper
  // Left axis
  cairo_move_to(cr, left, top);
  cairo_line_to(cr, left, bottom);
  cairo_stroke(cr);
  // Bottom axis
  cairo_move_to(cr, left, bottom);
  cairo_line_to(cr, right, bottom);
  cairo_stroke(cr);

  // tics and labels
  // bounds are margins in points.
  // compute Xrange in 10mm
  const int Xstart = ceil(points_to_mm(left) / 10.0);
  const int Xstop = floor(points_to_mm(right) / 10.0);
  for (int i = Xstart; i < Xstop + 1; i++) {
    pts = mm_to_points(i * 10.0);
    cairo_move_to(cr, pts, bottom);
    cairo_line_to(cr, pts, bottom + offset);
    cairo_stroke(cr);
    if (i % 2 == 0) {
      const std::string num = std::to_string(i * 10);
      draw_text(num.c_str(), cr, pts + 1, bottom + offset - 2, 6.0);
    }
  }
  // compute Yrange in 10mm
  const int Ystart = ceil(points_to_mm(top) / 10.0);
  const int Ystop = floor(points_to_mm(bottom) / 10.0);
  for (int i = Ystart; i < Ystop + 1; i++) {
    pts = mm_to_points(i * 10.0);
    cairo_move_to(cr, left, pts);
    cairo_line_to(cr, left - offset, pts);
    cairo_stroke(cr);
    if (i % 2 == 0) {
      const std::string num = std::to_string(-i * 10);
      draw_text(num.c_str(), cr, left - offset, pts - 3, 6.0);
    }
  }
}

// Draws a single 2D polygon.
void draw_geom(const Polygon2d& poly, cairo_t *cr)
{
  for (const auto& o : poly.outlines()) {
    if (o.vertices.empty()) {
      continue;
    }
    const Eigen::Vector2d& p0 = o.vertices[0];
    // Move to the first vertice.  Note Y is inverted in Cairo.
    cairo_move_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));
    for (unsigned int idx = 1; idx < o.vertices.size(); idx++) {
      const Eigen::Vector2d& p = o.vertices[idx];
      cairo_line_to(cr, mm_to_points(p.x()), mm_to_points(-p.y()));
    }
    // Draw a line from the last vertice to the first vertice.
    cairo_line_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));
  }
}

// Main entry:  draw geometry that consists of 2D polygons.  Walks the tree...
void draw_geom(const std::shared_ptr<const Geometry>& geom, cairo_t *cr)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      draw_geom(item.second, cr);
    }
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(false && "Unsupported file format");
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    draw_geom(*poly, cr);
  } else {
    assert(false && "Export as PDF for this geometry type is not supported");
  }
}

cairo_status_t export_pdf_write(void *closure, const unsigned char *data, unsigned int length)
{
  auto *stream = static_cast<std::ostream *>(closure);
  stream->write(reinterpret_cast<const char *>(data), length);
  return !(*stream) ? CAIRO_STATUS_WRITE_ERROR : CAIRO_STATUS_SUCCESS;
}

void add_meta_data(cairo_surface_t *surface, const cairo_pdf_metadata_t metadata,
                   const std::string& value, const std::string& value2 = "")
{
  const std::string v = value.empty() ? value2 : value;
  if (v.empty()) {
    return;
  }

  cairo_pdf_surface_set_metadata(surface, metadata, v.c_str());
}

}  // namespace

void export_pdf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                const ExportInfo& exportInfo)
{
  // Extract the options.  This will change when options becomes a variant.
  const ExportPdfOptions *options;
  const ExportPdfOptions defaultPdfOptions;

  // Could use short-circuit short-form, but will need to grow.
  if (exportInfo.optionsPdf) {
    options = exportInfo.optionsPdf.get();
  } else {
    options = &defaultPdfOptions;
  }

  // Selected paper size for export.
  int pdfX, pdfY;

  // Fit geometry to page, get dims in mm.
  BoundingBox bbox = geom->getBoundingBox();
  const int minx = (int)floor(bbox.min().x());
  const int maxy = (int)floor(bbox.max().y());
  const int maxx = (int)ceil(bbox.max().x());
  const int miny = (int)ceil(bbox.min().y());

  // Compute page attributes in points.
  const int spanX = mm_to_points(maxx - minx);
  const int spanY = mm_to_points(maxy - miny);
  const int centerX = mm_to_points(minx) + spanX / 2.0;
  const int centerY = mm_to_points(miny) + spanY / 2.0;

  // Set orientation and paper size.
  if ((options->orientation == ExportPdfPaperOrientation::AUTO && spanX > spanY) ||
      (options->orientation == ExportPdfPaperOrientation::LANDSCAPE)) {
    pdfX = paperDimensions[static_cast<int>(options->paperSize)][1];
    pdfY = paperDimensions[static_cast<int>(options->paperSize)][0];
  } else {
    pdfX = paperDimensions[static_cast<int>(options->paperSize)][0];
    pdfY = paperDimensions[static_cast<int>(options->paperSize)][1];
  }

  // Does it fit? (in points)
  const bool inpaper = (spanX <= pdfX - MARGIN) && (spanY <= pdfY - MARGIN);
  if (!inpaper) {
    LOG(message_group::Export_Warning, "Geometry is too large to fit into selected size.");
  }

  //  Center on page.  Still in points.
  // Note Cairo inverts the Y axis, with zero at the top, positive going down.
  // Compute translation and auxiliary numbers in lieu of transform matrices.
  const double tcX = pdfX / 2.0 - centerX;
  const double tcY = (pdfY / 2.0 + centerY);  // Note Geometry Y will still need to be inverted.
  // Shifted exact margins
  const double Mlx = centerX - pdfX / 2.0 + MARGIN;     // Left margin, X axis
  const double Mrx = centerX + pdfX / 2.0 - MARGIN;     // Right margin, X axis
  const double Mty = -(centerY - pdfY / 2.0 + MARGIN);  // INVERTED Top margin, Y axis
  const double Mby = -(centerY + pdfY / 2.0 - MARGIN);  // INVERTED Bottom margin, Y axis

  // Initialize Cairo Surface and PDF
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(export_pdf_write, &output, pdfX, pdfY);
  if (cairo_surface_status(surface) == cairo_status_t::CAIRO_STATUS_NULL_POINTER) {
    cairo_surface_destroy(surface);
    return;
  }

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 16, 0)
  if (options->addMetaData) {
    add_meta_data(surface, CAIRO_PDF_METADATA_TITLE, options->metaDataTitle, exportInfo.title);
    add_meta_data(surface, CAIRO_PDF_METADATA_CREATOR, EXPORT_CREATOR);
    add_meta_data(surface, CAIRO_PDF_METADATA_CREATE_DATE, get_current_iso8601_date_time_utc());
    add_meta_data(surface, CAIRO_PDF_METADATA_MOD_DATE, "");
    add_meta_data(surface, CAIRO_PDF_METADATA_AUTHOR, options->metaDataAuthor);
    add_meta_data(surface, CAIRO_PDF_METADATA_SUBJECT, options->metaDataSubject);
    add_meta_data(surface, CAIRO_PDF_METADATA_KEYWORDS, options->metaDataKeywords);
  }
#endif

  cairo_t *cr = cairo_create(surface);

  // Note Y axis + is DOWN.  Drawings have to invert Y, but these translations account for that.
  cairo_translate(cr, tcX, tcY);  // Center page on geometry;

  const Color4f black = Color4f(0.0f, 0.0f, 0.0f);

  // create path
  draw_geom(geom, cr);

  if (options->fill) {
    Color4f fillColor = OpenSCAD::getColor(options->fillColor, black);
    cairo_set_source_rgba(cr, fillColor.r(), fillColor.g(), fillColor.b(), fillColor.a());
    cairo_fill_preserve(cr);
  }

  if (options->stroke) {
    Color4f strokeColor = OpenSCAD::getColor(options->strokeColor, black);
    cairo_set_source_rgba(cr, strokeColor.r(), strokeColor.g(), strokeColor.b(), strokeColor.a());
    cairo_set_line_width(cr, mm_to_points(options->strokeWidth));
    cairo_stroke_preserve(cr);
  }

  // clear path
  cairo_new_path(cr);

  // Set Annotations
  const std::string about =
    "Scale is to calibrate actual printed dimension. Check both X and Y. "
    "Measure between tick 0 and last tick";
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.48);
  // Design Filename
  if (options->showDesignFilename) draw_text(exportInfo.sourceFilePath.c_str(), cr, Mlx, Mby, 10.0);
  // Scale
  if (options->showScale) {
    draw_axes(cr, Mlx, Mrx, Mty, Mby);
    // Scale Message
    if (options->showScaleMsg) draw_text(about.c_str(), cr, Mlx + 1, Mty - 1, 5.0);
    // Grid
    if (options->showGrid) draw_grid(cr, Mlx, Mrx, Mty, Mby, options->gridSize);
  }

  cairo_show_page(cr);
  cairo_surface_destroy(surface);
  cairo_destroy(cr);
}

#else  // ENABLE_CAIRO

const std::string get_cairo_version()
{
  const std::string cairo_version = "(not enabled)";
  return cairo_version;
}

void export_pdf(const std::shared_ptr<const Geometry>&, std::ostream&, const ExportInfo&)
{
  LOG(message_group::Error, "Export to PDF format was not enabled when building the application.");
}

#endif  // ENABLE_CAIRO
