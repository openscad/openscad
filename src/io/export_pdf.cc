#include "io/export.h"
#include "geometry/PolySet.h"
// #include "geometry/PolySetUtils.h"
#include "utils/printutils.h"
// #include "version.h"
#include "utils/version_helper.h"

#include <cassert>
#include <ostream>
#include <memory>
#include <string>
#include <cmath>

#ifdef ENABLE_CAIRO

#include <cairo.h>
#include <cairo-pdf.h>


#define FONT "Liberation Sans"


#define MARGIN 30.

// void export_pdf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo, const ExportPdfOptions  exportPdfOptions);
 
const std::string get_cairo_version() {
  return OpenSCAD::get_version(CAIRO_VERSION_STRING, cairo_version_string());
}

void draw_text(const char *text, cairo_t *cr, double x, double y, double fontSize){

  cairo_select_font_face(cr, FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, fontSize);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, text);

}

#define PTS_IN_MM 2.834645656693;

double mm_to_points(double mm){
  return mm * PTS_IN_MM;
}

double points_to_mm(double pts){
  return pts / PTS_IN_MM;
}

void draw_grid(cairo_t *cr, double left, double right, double bottom, double top, double gridSize ){
  // gridSize>1.
  if (gridSize<1.) gridSize=2.;
  double darkerLine=0.36;
  double lightLine=0.24;
  int major = (gridSize>10.? gridSize: int(10./gridSize));

  double pts=0.;  // for iteration across page
  
  // bounds are margins in points.
  // compute Xrange in units of gridSize
  int Xstart=ceil(points_to_mm(left)/gridSize);
  int Xstop=floor(points_to_mm(right)/gridSize);
  // draw Horizontal lines
   for (int i = Xstart; i < Xstop+1; i++) {
      if (i%major)  { 
      	cairo_set_line_width(cr, lightLine);
      	cairo_set_source_rgba(cr, 0., 0., 0., 0.48);
      	}
      else  { 
      	cairo_set_line_width(cr, darkerLine);
	cairo_set_source_rgba(cr, 0., 0., 0., 0.6);
	}
      pts=mm_to_points(i*gridSize);
      cairo_move_to(cr, pts, top);
      cairo_line_to(cr, pts, bottom);
      cairo_stroke(cr);
  };
  // compute Yrange in units of gridSize
  int Ystart=ceil(points_to_mm(top)/gridSize);
  int Ystop=floor(points_to_mm(bottom)/gridSize);
  // draw vertical lines
   for (int i = Ystart; i < Ystop+1; i++) {
         if (i%major)  { 
      	cairo_set_line_width(cr, lightLine);
      	cairo_set_source_rgba(cr, 0., 0., 0., 0.4);
      	}
      else  { 
      	cairo_set_line_width(cr, darkerLine);
	cairo_set_source_rgba(cr, 0., 0., 0., 0.6);
	}
      pts=mm_to_points(i*gridSize);
      cairo_move_to(cr, left, pts);
      cairo_line_to(cr, right, pts);
      cairo_stroke(cr);
  };
}  

// New draw_axes (renamed from axis since it draws both).  
void draw_axes(cairo_t *cr, double left, double right, double bottom, double top){
  double darkerLine=0.36;
  double offset = mm_to_points(5.);
  double pts=0.;  // for iteration across page
  
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
  int Xstart=ceil(points_to_mm(left)/10.);
  int Xstop=floor(points_to_mm(right)/10.);
   for (int i = Xstart; i < Xstop+1; i++) {
      pts=mm_to_points(i*10.);
      cairo_move_to(cr, pts, bottom);
      cairo_line_to(cr, pts, bottom + offset);
      cairo_stroke(cr);
      if (i % 2 == 0) {
            std::string num = std::to_string(i * 10);
            draw_text(num.c_str(), cr, pts+1, bottom+offset-2, 6.);
      }
  };
  	// compute Yrange in 10mm
  int Ystart=ceil(points_to_mm(top)/10.);
  int Ystop=floor(points_to_mm(bottom)/10.);
   for (int i = Ystart; i < Ystop+1; i++) {
      pts=mm_to_points(i*10.);
      cairo_move_to(cr, left, pts);
      cairo_line_to(cr, left-offset, pts);
      cairo_stroke(cr);
      if (i % 2 == 0) {
            std::string num = std::to_string(-i * 10);
            draw_text(num.c_str(), cr, left-offset, pts - 3, 6.);
      }
  };
}  
  


// Draws a single 2D polygon.
void draw_geom(const Polygon2d& poly, cairo_t *cr ){
  for (const auto& o : poly.outlines()) {
    if (o.vertices.empty()) {
      continue;
    }
    const Eigen::Vector2d& p0 = o.vertices[0];
    // Move to the first vertice.  Note Y is inverted in Cairo.
    cairo_move_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));
    // LOG(message_group::Export_Warning, "DRAW VERTICE %1$8.4f %2$8.4f", mm_to_points(p0.x()),mm_to_points(p0.y()));
    // iterate across the remaining vertices, drawing a line to each.
    for (unsigned int idx = 1; idx < o.vertices.size(); idx++) {
      const Eigen::Vector2d& p = o.vertices[idx];
      cairo_line_to(cr, mm_to_points(p.x()), mm_to_points(-p.y()));
      // LOG(message_group::Export_Warning, "DRAW VERTICE %1$8.4f %2$8.4f", mm_to_points(p.x()),mm_to_points(p.y()));
    }
    // Draw a line from the last vertice to the first vertice.
    cairo_line_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));

  }
}


// Main entry:  draw geometry that consists of 2D polygons.  Walks the tree...
void draw_geom(const std::shared_ptr<const Geometry>& geom, cairo_t *cr){
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) { // iterate
    for (const auto& item : geomlist->getChildren()) {
      draw_geom(item.second, cr);
    }
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(false && "Unsupported file format");
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) { // geometry that can be drawn.
    draw_geom(*poly, cr);
  } else {
    assert(false && "Export as PDF for this geometry type is not supported");
  }
}

static cairo_status_t export_pdf_write(void *closure, const unsigned char *data, unsigned int length)
{
  std::ostream *stream = static_cast<std::ostream *>(closure);
  stream->write(reinterpret_cast<const char *>(data), length);
  return !(*stream) ? CAIRO_STATUS_WRITE_ERROR : CAIRO_STATUS_SUCCESS;
}


void export_pdf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo)
{
// Extract the options.  This will change when options becomes a variant.
ExportPdfOptions *exportPdfOptions;
ExportPdfOptions defaultPdfOptions;
// could use short-circuit short-form, but will need to grow.
if (exportInfo.options==nullptr) {
	exportPdfOptions=&defaultPdfOptions;
} else {
	exportPdfOptions=exportInfo.options;
};

  int pdfX,pdfY;  // selected paper size for export.
  // Fit geometry to page
  // Get dims in mm.
  BoundingBox bbox = geom->getBoundingBox();
  int minx = (int)floor(bbox.min().x());
  int maxy = (int)floor(bbox.max().y());
  int maxx = (int)ceil(bbox.max().x());
  int miny = (int)ceil(bbox.min().y());
  // compute page attributes in points
  int spanX = mm_to_points(maxx-minx);
  int spanY = mm_to_points(maxy-miny);
  int centerX = mm_to_points(minx)+spanX/2;
  int centerY = mm_to_points(miny)+spanY/2;
  // Temporary Log
//  LOG(message_group::Export_Warning, "min( %1$6d , %2$6d ), max( %3$6d , %4$6d )", minx, miny, maxx, maxy);
//  LOG(message_group::Export_Warning, "span( %1$6d , %2$6d ), center ( %3$6d , %4$6d )", spanX, spanY,  centerX, centerY);
  
  // Set orientation and paper size
  if ((exportPdfOptions->Orientation==paperOrientations::AUTO && spanX>spanY)||(exportPdfOptions->Orientation==paperOrientations::LANDSCAPE)) {
  	pdfX=paperDimensions[static_cast<int>(exportPdfOptions->paperSize)][1];
  	pdfY=paperDimensions[static_cast<int>(exportPdfOptions->paperSize)][0];
  } else {
    	pdfX=paperDimensions[static_cast<int>(exportPdfOptions->paperSize)][0];
    	pdfY=paperDimensions[static_cast<int>(exportPdfOptions->paperSize)][1];
  }; 
  
  // Does it fit? (in points)	
  bool inpaper = (spanX<=pdfX-MARGIN)&&(spanY<=pdfY-MARGIN);
  if (!inpaper) {
    LOG(message_group::Export_Warning, "Geometry is too large to fit into selected size.");
  }
  //      LOG(message_group::Export_Warning, "pdfX, pdfY %1$6d %2$6d ", pdfX, pdfY);
        
  //  Center on page.  Still in points.
  // Note Cairo inverts the Y axis, with zero at the top, positive going down.
  // Compute translation and auxiliary numbers in lieu of transform matrices.
  double tcX=pdfX/2-centerX;
  double tcY=(pdfY/2+centerY); // Note Geometry Y will still need to be inverted.
  // Shifted exact margins
  double Mlx=centerX-pdfX/2+MARGIN;  // Left margin, X axis
  double Mrx=centerX+pdfX/2-MARGIN;  // Right margin, X axis
  double Mty=-(centerY-pdfY/2+MARGIN);  // INVERTED Top margin, Y axis
  double Mby=-(centerY+pdfY/2-MARGIN);  // INVERTED Bottom margin, Y axis
    // Temporary Log
    // LOG(message_group::Export_Warning, "tcX, tcY %1$6d , %2$6d", tcX, tcY);
    // LOG(message_group::Export_Warning, "Mlx, Mry %1$6d %2$6d Mtx, Mty %3$6d %4$6d", Mlx, Mrx, Mty, Mby);
  
  // Initialize Cairo Surface and PDF
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(export_pdf_write, &output, pdfX, pdfY);
  if (cairo_surface_status(surface) == cairo_status_t::CAIRO_STATUS_NULL_POINTER) {
    cairo_surface_destroy(surface);
    return;
  }


#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 16, 0)
  cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_TITLE, std::filesystem::path(exportInfo.sourceFilePath).filename().string().c_str());
  cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, "OpenSCAD (https://www.openscad.org/)");
  cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATE_DATE, "");
  cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_MOD_DATE, "");
#endif

  cairo_t *cr = cairo_create(surface);
  // Note Y axis + is DOWN.  Drawings have to invert Y, but these translations account for that.
  cairo_translate(cr, tcX, tcY);  // Center page on geometry;

  cairo_set_source_rgba(cr, 0., 0., 0., 1.0); // Set black line, opaque
  cairo_set_line_width(cr, 1);  // 1 point width.
  draw_geom(geom, cr);
  cairo_stroke(cr);
    
    // Set Annotations
      std::string about = "Scale is to calibrate actual printed dimension. Check both X and Y. Measure between tick 0 and last tick";
    cairo_set_source_rgba(cr, 0., 0., 0., 0.48);
    // Design Filename
    if (exportPdfOptions->showDesignFilename) draw_text(exportInfo.sourceFilePath.c_str(), cr, Mlx, Mby, 10.);
    // Scale
    if (exportPdfOptions->showScale) {
    	draw_axes(cr, Mlx,Mrx,Mty,Mby);
    	// Scale Message
    	if (exportPdfOptions->showScaleMsg) draw_text(about.c_str(), cr, Mlx+1, Mty-1, 5.);
    }
    // Grid
    if (exportPdfOptions->showGrid) draw_grid(cr, Mlx,Mrx,Mty,Mby, exportPdfOptions->gridSize);

  cairo_show_page(cr);
  cairo_surface_destroy(surface);
  cairo_destroy(cr);

}
#else //ENABLE_CAIRO

const std::string get_cairo_version() {
  const std::string cairo_version = "(not enabled)";
  return cairo_version;
}

void export_pdf(const std::shared_ptr<const Geometry>&, std::ostream&, const ExportInfo&) {

  LOG(message_group::Error, "Export to PDF format was not enabled when building the application.");

}

#endif //ENABLE_CAIRO
