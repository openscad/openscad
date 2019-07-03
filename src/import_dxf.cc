#include "import.h"
#include "grid.h"
#include "printutils.h"
#include "calc.h"
#include "Polygon2d.h"
#include "degree_trig.h"
#include "dxf.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

Polygon2d *import_dxf(const std::string &filename, double fn, double fs, double fa)
{
	std::ifstream stream(filename.c_str());
    auto poly = new Polygon2d();
	if (!stream.good()) {
		PRINTB("WARNING: Can't open DXF file '%s'.", filename);
	}  
    else{
        
        read_dxf_file(filename, filename);

        std::vector<header_struct> header_vector;
        std::vector<table_struct> table_vector;
        std::vector<polyline_vertex_struct> polyline_vertex_vector;
        std::vector<polyline_struct> polyline_vector;
        std::vector<lwpolyline_struct> lwpolyline_vector;
        std::vector<circle_struct> circle_vector;
        std::vector<face3d_struct> face3d_vector;
        std::vector<line_struct> line_vector;
        std::vector<insert_struct> insert_vector;
        std::vector<point_struct> point_vector;
        std::vector<arc_struct> arc_vector;
        std::vector<text_struct> text_vector;
        std::vector<solid_struct> solid_vector;
        std::vector<mtext_struct> mtext_vector;
        std::vector<text_attrib_struct> text_attrib_vector;
        std::vector<ellipse_struct> ellipse_vector;
        std::vector<leader_struct> leader_vector;
        std::vector<spline_struct> spline_vector;
        std::vector<dimension_struct> dimension_vector;
        
        fn = 0; 
        fs = 2; 
        fa = 12;
        circle_vector = return_circle_entities_vector();
        if(!circle_vector.empty()){
            for(auto it : circle_vector){
                Outline2d outline;
                int n = Calc::get_fragments_from_r(it.radius, fn, fs, fa);
                Vector2d center(it.center[0], it.center[1]);
                for (int i = 0; i < n; i++) {
                    double a1 = (360.0 * i) / n;
                    double a2 = (360.0 *(i + 1)) / n;
                    outline.vertices.push_back(Vector2d(cos_degrees(a1)*it.radius + it.center[0], 
                                                        sin_degrees(a1)*it.radius + it.center[1]));
                    outline.vertices.push_back(Vector2d(cos_degrees(a2)*it.radius + it.center[0], 
                                                        sin_degrees(a2)*it.radius + it.center[1]));
                    outline.positive = true;
                }
                poly->addOutline(outline);
            }
        }        
    }



	// Grid2d<std::vector<int>> grid(GRID_COARSE);
    // std::vector<double> xverts = {-20, -20, 20, 20};
	// std::vector<double> yverts = {-20, 20, 20, -20};
    // size_t vertexNum = 4;
    // for(size_t i = 0; i < vertexNum; i++){
    //     grid.align(xverts.at(i), yverts.at(i));
    //     //what does the grid.align do? Just align the vertex on the grid?
    //     outline.vertices.push_back(Vector2d(xverts.at(i),yverts.at(i)));
    //     outline.positive = true; \\ Do we need this?
    // }

    return poly;
};


