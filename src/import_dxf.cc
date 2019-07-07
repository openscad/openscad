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

#define ADD_LINE(_x1, _y1, _x2, _y2) do {										\
		double _p1x = _x1, _p1y = _y1, _p2x = _x2, _p2y = _y2;  \                                              \
		grid.align(_p1x, _p1y);                                 \
		grid.align(_p2x, _p2y);                                 \
		grid.data(_p1x, _p1y).push_back(lines.size());          \
		grid.data(_p2x, _p2y).push_back(lines.size());          \
		if (in_entities_section)                                \
			lines.emplace_back(                                   \
			  addPoint(_p1x, _p1y), addPoint(_p2x, _p2y));        \
		if (in_blocks_section && !current_block.empty())        \
			blockdata[current_block].emplace_back(	              \
				addPoint(_p1x, _p1y), addPoint(_p2x, _p2y));      	\
} while (0)

Polygon2d *import_dxf(const std::string &filename, double fn, double fs, double fa)
{
	std::ifstream stream(filename.c_str());
    auto poly = new Polygon2d();
	if (!stream.good()) {
		PRINTB("WARNING: Can't open DXF file '%s'.", filename);
        return poly;
	}

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

    circle_vector = return_circle_vector();
    if(!circle_vector.empty()){
        Outline2d outline;
        for(auto it : circle_vector){
            int n = Calc::get_fragments_from_r(it.radius, fn, fs, fa);
            Vector2d center(it.center[0], it.center[1]);
            for (int i = 0; i < n; i++) {
                double a1 = (360.0 * i) / n;
                double a2 = (360.0 *(i + 1)) / n;
                outline.vertices.push_back(Vector2d(cos_degrees(a1)*it.radius + it.center[0], 
                                                    sin_degrees(a1)*it.radius + it.center[1]));
                outline.vertices.push_back(Vector2d(cos_degrees(a2)*it.radius + it.center[0], 
                                                    sin_degrees(a2)*it.radius + it.center[1]));
            }
        }
        poly->addOutline(outline);
    }

    arc_vector = return_arc_vector();
    if(!arc_vector.empty()){
        Outline2d outline;
        for(auto it : arc_vector){
            Vector2d center(it.center[0], it.center[1]);
            int n = Calc::get_fragments_from_r(it.radius, fn, fs, fa);
            while (it.start_angle > it.end_angle) {
                it.end_angle += 360.0;
            }
            double arc_angle = it.end_angle - it.start_angle;
            n = static_cast<int>(ceil(n * arc_angle / 360));
            for (int i = 0; i < n; i++) {
                double a1 = it.start_angle + arc_angle * i / n;
                double a2 = it.start_angle + arc_angle * (i + 1) / n;
                outline.vertices.push_back(Vector2d(cos_degrees(a1)*it.radius + center[0], 
                                                    sin_degrees(a1)*it.radius + center[1]));
                outline.vertices.push_back(Vector2d(cos_degrees(a2)*it.radius + center[0], 
                                                    sin_degrees(a2)*it.radius + center[1]));
            }
        }
        poly->addOutline(outline);
    }
    
    ellipse_vector = return_ellipse_vector();
    if(!ellipse_vector.empty()){
        fprintf(stdout, "size of ellipse vector %d \n", ellipse_vector.size());
        Outline2d outline;
        for(auto it : ellipse_vector){
            // Commented code is meant as documentation of vector math
            fprintf(stdout, "you now run it one more time!\n");
            while (it.start_angle > it.end_angle) it.end_angle += 2 * M_PI;
    //				Vector2d center(xverts[0], yverts[0]);
            Vector2d center(it.center[0], it.center[1]);
    //				Vector2d ce(xverts[1], yverts[1]);
            Vector2d ce(it.majorAxis[0], it.majorAxis[1]);
    //				double r_major = ce.length();
            double r_major = sqrt(ce[0]*ce[0] + ce[1]*ce[1]);
    //				double rot_angle = ce.angle();
            double rot_angle;
            {
    //					double dot = ce.dot(Vector2d(1.0, 0.0));
                double dot = ce[0];
                double cosval = dot / r_major;
                if (cosval > 1.0) cosval = 1.0;
                if (cosval < -1.0) cosval = -1.0;
                rot_angle = acos(cosval);
                if (ce[1] < 0.0) rot_angle = 2 * M_PI - rot_angle;
            }

            // the ratio stored in 'radius; due to the parser code not checking entity type
            // edited : using brlcad code radius is now replaced by ratio
            double r_minor = r_major * it.ratio;
            double sweep_angle = it.end_angle-it.start_angle;
            int n = Calc::get_fragments_from_r(r_major, fn, fs, fa);
            n = static_cast<int>(ceil(n * sweep_angle / (2 * M_PI)));
    //				Vector2d p1;
            Vector2d p1{0.0, 0.0};
            for (int i=0;i<=n;i++) {
                double a = (it.start_angle + sweep_angle*i/n);
    //					Vector2d p2(cos(a)*r_major, sin(a)*r_minor);
                Vector2d p2(cos(a)*r_major, sin(a)*r_minor);
    //					p2.rotate(rot_angle);
                Vector2d p2_rot(cos(rot_angle)*p2[0] - sin(rot_angle)*p2[1],
                                sin(rot_angle)*p2[0] + cos(rot_angle)*p2[1]);
    //					p2 += center;
                p2_rot[0] += center[0];
                p2_rot[1] += center[1];
                if (i > 0) {
    // 						ADD_LINE(p1[0], p1[1], p2[0], p2[1]);
                    outline.vertices.push_back(Vector2d(p1[0], p1[1]));
                    outline.vertices.push_back(Vector2d(p2[0], p2[1]));
                }
    //					p1 = p2;
                p1[0] = p2_rot[0];
                p1[1] = p2_rot[1];            
            }
        }   
        poly->addOutline(outline);
    }

    spline_vector = return_spline_vector();
    if(!spline_vector.empty()){

    }

    lwpolyline_vector = return_lwpolyline_vector();
    if(!lwpolyline_vector.empty()){
        Outline2d outline;
        for(auto it : lwpolyline_vector){
            for(auto it_pt : it.lw_pt_vec){
                outline.vertices.push_back(Vector2d(it_pt.x, it_pt.y));
            }
            // if polyline_flag == 1, the last vertex connect to the first vertex
            if (it.polyline_flag & 0x01){
                outline.vertices.push_back(Vector2d(it.lw_pt_vec.back().x, it.lw_pt_vec.back().y));
                outline.vertices.push_back(Vector2d(it.lw_pt_vec.front().x, it.lw_pt_vec.front().y));
            }
        }            
        poly->addOutline(outline);
    }

    line_vector = return_line_vector();
    if(!line_vector.empty()){
        Outline2d outline;
        for(auto it : line_vector){
            outline.vertices.push_back(Vector2d(it.line_pt[0][0], it.line_pt[0][1]));
            outline.vertices.push_back(Vector2d(it.line_pt[1][0], it.line_pt[1][1]));
        }
        poly->addOutline(outline);
    }

    point_vector = return_point_vector();
    if(!point_vector.empty()){
        Outline2d outline;
        for(auto it : point_vector){
            outline.vertices.push_back(Vector2d(it.pt[0], it.pt[1]));
        }
        poly->addOutline(outline);
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


