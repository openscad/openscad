#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "printutils.h"
#include "version.h"
#include "version_helper.h"

#include <string>
#include <cmath>

#ifdef ENABLE_CAIRO

#include <cairo.h>
#include <cairo-pdf.h>


// A4 Size Paper
#define WPOINTS 595.
#define HPOINTS 842.
#define MARGIN 30.

const std::string get_cairo_version() {
	return OpenSCAD::get_version(CAIRO_VERSION_STRING, cairo_version_string());
}

enum class OriginPosition{
  BUTTOMLEFT,
  CENTER
};

void draw_name(const char *name2display, cairo_t *cr,double x,double y){

    cairo_set_font_size(cr, 10.);
    cairo_move_to(cr,x,y);
    cairo_show_text(cr,name2display);

}

double mm_to_points(double mm){
    return mm*2.8346;
}

void draw_axis(cairo_t *cr, OriginPosition pos){
    cairo_set_font_size(cr, 6.);
    cairo_set_line_width(cr, 0.4);
    double offset = mm_to_points(10.);

    if(pos == OriginPosition::CENTER){

        for(int i=1;i<3;i++){
            cairo_move_to(cr, i*offset, (HPOINTS/2.));
            cairo_line_to(cr, i*offset, (HPOINTS/2.)-mm_to_points(12));
            cairo_stroke(cr);
            cairo_move_to(cr,(i*offset)+1.5, (HPOINTS/2.)-mm_to_points(8));
            if(i%2==0){
                std::string num = std::to_string(i*10);
                cairo_show_text(cr, num.c_str());
            }
        }
        for(int i=1;i<3;i++){
            cairo_move_to(cr, -i*offset, (HPOINTS/2.));
            cairo_line_to(cr, -i*offset, (HPOINTS/2.)-mm_to_points(12));
            cairo_stroke(cr);
            cairo_move_to(cr,(-i*offset)+1.5, (HPOINTS/2.)-mm_to_points(8));
            if(i%2==0){
                std::string num = std::to_string(-i*10);
                cairo_show_text(cr, num.c_str());
            }
        }

        for(int i=1;i<3;i++){
            cairo_move_to(cr, -(WPOINTS/2.) , -i*offset);
            cairo_line_to(cr, -(WPOINTS/2.)+mm_to_points(12), -i*offset);
            cairo_stroke(cr);
            cairo_move_to(cr, -(WPOINTS/2.)+mm_to_points(8), (-i*offset)-1.5);
            if(i%2==0){
                std::string num = std::to_string(i*10);
                cairo_show_text(cr, num.c_str());
            }
        }
        for(int i=1;i<3;i++){
            cairo_move_to(cr, -(WPOINTS/2.), i*offset);
            cairo_line_to(cr, -(WPOINTS/2.)+mm_to_points(12), i*offset);
            cairo_stroke(cr);
            cairo_move_to(cr, -(WPOINTS/2.)+mm_to_points(8), (i*offset)-1.5);
            if(i%2==0){
                std::string num = std::to_string(-i*10);
                cairo_show_text(cr, num.c_str());
            }
        }
        cairo_set_source_rgba(cr, 0., 0., 0., 1.0);
        cairo_move_to(cr, 0., (HPOINTS/2.));
        cairo_line_to(cr, 0., (HPOINTS/2.)-mm_to_points(12));
        cairo_move_to(cr, -(WPOINTS/2.), 0.);
        cairo_line_to(cr, -(WPOINTS/2.)+mm_to_points(12), 0.);
        cairo_stroke(cr);

    }else{

        for(int i=1;i<6;i++){
            cairo_move_to(cr, i*offset, 0.0);
            cairo_line_to(cr, i*offset, -mm_to_points(12));
            cairo_stroke(cr);
            cairo_move_to(cr,(i*offset)+1.5, -mm_to_points(8));
            if(i%2==0){
                std::string num = std::to_string(i*10);
                cairo_show_text(cr, num.c_str());
            }
        }
        for(int i=1;i<6;i++){
            cairo_move_to(cr, 0., -i*offset);
            cairo_line_to(cr, mm_to_points(12), -i*offset);
            cairo_stroke(cr);
            cairo_move_to(cr, mm_to_points(8), (-i*offset)-1.5);
            if(i%2==0){
                std::string num = std::to_string(i*10);
                cairo_show_text(cr, num.c_str());
            }
        }
    }
}

void draw_geom(const Polygon2d &poly, cairo_t *cr, bool &inpaper, OriginPosition pos){
    for(const auto &o : poly.outlines()){
        if (o.vertices.empty()) {
            continue;
        }
        const Eigen::Vector2d& p0 = o.vertices[0];
        cairo_move_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));
        for (unsigned int idx = 1;idx < o.vertices.size();idx++) {
            const Eigen::Vector2d& p = o.vertices[idx];
            cairo_line_to(cr, mm_to_points(p.x()), mm_to_points(-p.y()));
            if(pos == OriginPosition::CENTER){
                if( abs((int)mm_to_points(p.x()))>(WPOINTS/2) || abs((int)mm_to_points(p.y()))>(HPOINTS/2)) {
                    inpaper = false;
                }
            }else {
                if( abs((int)mm_to_points(p.x()))>WPOINTS || abs((int)mm_to_points(p.y()))>HPOINTS) {
                    inpaper = false;
                }
            }
        }
        cairo_line_to(cr, mm_to_points(p0.x()), mm_to_points(-p0.y()));

    }
}

void draw_geom(const shared_ptr<const Geometry> &geom, cairo_t *cr, bool &inpaper, OriginPosition pos){
    if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
        for (const auto &item : geomlist->getChildren()) {
            draw_geom(item.second, cr, inpaper, pos);
        }
    }
    else if (dynamic_pointer_cast<const PolySet>(geom)) {
        assert(false && "Unsupported file format");
    }
    else if (const auto poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
        draw_geom(*poly, cr, inpaper, pos);
    } else {
        assert(false && "Export as PDF for this geometry type is not supported");
    }
}

void export_pdf(const shared_ptr<const Geometry> &geom, ExportInfo exportInfo, bool &onerror){

    cairo_surface_t *surface = cairo_pdf_surface_create(exportInfo.name2open.c_str(), WPOINTS, HPOINTS);
    if(cairo_surface_status(surface)==cairo_status_t::CAIRO_STATUS_NULL_POINTER){
        onerror=true;
        cairo_surface_destroy(surface);
        return;
    }

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 16, 0)
	cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_TITLE, exportInfo.sourceFileName.c_str());
	cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, "OpenSCAD (https://www.openscad.org/)");
	cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_CREATE_DATE, "");
	cairo_pdf_surface_set_metadata(surface, CAIRO_PDF_METADATA_MOD_DATE, "");
#endif

    cairo_t *cr = cairo_create(surface);

    cairo_set_source_rgba(cr, 0., 0., 0., 1.0);
    cairo_set_line_width(cr, 1);

    BoundingBox bbox = geom->getBoundingBox();
    int minx = (int)floor(bbox.min().x());
    int maxy = (int)floor(bbox.max().y());
    int maxx = (int)ceil(bbox.max().x());
    int miny = (int)ceil(bbox.min().y());

    bool inpaper = true;

    if(minx>=0 && miny>=0 && maxx>=0 && maxy>=0){

        cairo_translate(cr, MARGIN, HPOINTS-MARGIN);
        draw_geom(geom, cr, inpaper, OriginPosition::BUTTOMLEFT);
        cairo_stroke(cr);
        cairo_set_source_rgba(cr, 0., 0., 0., 0.4);
        draw_name(exportInfo.sourceFilePath.c_str(),cr, 0., -HPOINTS+(2.*MARGIN));
        cairo_translate(cr, -MARGIN, MARGIN);
        draw_axis(cr, OriginPosition::BUTTOMLEFT);

    }else {

        cairo_translate(cr, WPOINTS/2., HPOINTS/2.);
        draw_geom(geom, cr, inpaper, OriginPosition::CENTER);
        cairo_stroke(cr);
        cairo_set_source_rgba(cr, 0., 0., 0., 0.4);
        draw_name(exportInfo.sourceFilePath.c_str(),cr, -(WPOINTS/2.)+MARGIN, -(HPOINTS/2.)+MARGIN);
        draw_axis(cr, OriginPosition::CENTER);
    }

    if(!inpaper){
        PRINT("WARNING: This Geometry cannot be accomodated in A4 size paper. Scale it down.");
    }

    cairo_show_page(cr);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

}
#else //ENABLE_CAIRO

const std::string get_cairo_version() {
	const std::string cairo_version = "(not enabled)";
	return cairo_version;
}

void export_pdf(const shared_ptr<const Geometry> &, ExportInfo , bool &){

    PRINT("Export to PDF format was not enabled when building the application.");

}

#endif //ENABLE_CAIRO
