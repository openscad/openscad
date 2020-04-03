#include <cairo.h>
#include <cairo-pdf.h>
#include <math.h>

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"

#include <QDebug>

static void draw_geom(const shared_ptr<const Geometry> &geom, cairo_t &cr){

}

void export_pdf(const shared_ptr<const Geometry> &geom, const char *name2open){
    //A4 Size in points
    const double WPOINTS = 595;
    const double HPOINTS = 842;

    cairo_surface_t *surface = cairo_pdf_surface_create(name2open, WPOINTS, HPOINTS);
    cairo_t *cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 0.9);

    BoundingBox bbox = geom->getBoundingBox();
    int minx = (int)floor(bbox.min().x());
    int maxy = (int)floor(bbox.max().y());
    int maxx = (int)ceil(bbox.max().x());
    int miny = (int)ceil(bbox.min().y());
    int width = maxx - minx;
    int height = maxy - miny;
    qDebug()<<minx<<" "<<miny<<" "<<maxx<<" "<<maxy<<" "<<width<<" "<<height<<"\n";

    if(minx>=0 && miny>=0 && maxx>=0 && maxy>=0){
        cairo_translate(cr, 10., HPOINTS-10.);
    }else {
        cairo_translate(cr, WPOINTS/2, HPOINTS/2);
    }
    cairo_rotate(cr,270.*(M_PI/180.));

    draw_geom(geom,cr);

    cairo_show_page(cr);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

}
