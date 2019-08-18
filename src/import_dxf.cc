#include "grid.h"
#include "printutils.h"
#include "calc.h"
#include "Polygon2d.h"
#include "degree_trig.h"
#include "dxf.h"
#include "linalg.h"
#include "import.h"
#include "boost-utils.h"

#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <assert.h>
#include <unordered_map>
#include <iostream>

#define RT_NURB_MAKE_PT_TYPE(n, t, h)	((n<<5) | (t<<1) | h)
#define RT_NURB_EXTRACT_COORDS(pt)	(pt>>5)
#define RT_NURB_EXTRACT_PT_TYPE(pt)		((pt>>1) & 0x0f)
#define POLYLINE_3D_POLYLINE 8
#define POLYLINE_3D_POLYMESH 16
#define POLYLINE_POLYFACE_MESH 64
#define POLYLINE_CURVE_FIT_VERTICES_ADDED 2
#define POLYLINE_SPLINE_FIT_VERTICES_ADDED 4
#define VTX_SPLINE_VERTEX_CREATED 8
#define VTX_SPLINE_FRAME_CONTROL_POINT 16

struct Line {
    int idx[2]; // indices into DData::points
    bool disabled;
    Line(int i1 = -1, int i2 = -1) : idx{i1, i2}, disabled(false) { }
};

class DData
{
public:
	struct Path {
		std::vector<int> indices; // indices into DData::points
		bool is_closed, is_inner;
		Path() : is_closed(false), is_inner(false) { }
	};
	struct Dim {
		unsigned int type;
		double coords[7][2];
		double angle;
		double length;
		std::string name;
		Dim() {
			for (int i = 0; i < 7; i++)
			for (int j = 0; j < 2; j++)
				coords[i][j] = 0;
			type = 0;
			angle = 0;
			length = 0;
		}
	};

    DData();

	double scale, xorigin, yorigin;
	Vector2d pen;
	Grid2d<std::vector<int>> grid;

	std::vector<Vector2d> points;
	std::vector<Path> paths;
	std::vector<Dim> dims;
    std::vector<Line> lines;

	int addPoint(double x, double y);
    void addLine(double x1, double y1, double x2, double y2);

	void fixup_path_direction();
    void process_path();
	std::string dump() const;
	class Polygon2d *toPolygon2d() const;
	void curve_to(double fn, const Vector2d &c1, const Vector2d &to);
	void curve_to(double fn, const Vector2d &c1, const Vector2d &c2, const Vector2d &to);
};

DData::DData(): scale(0), xorigin(0), yorigin(0), pen(Vector2d(0,0)), grid(GRID_COARSE) {

}

// Quadric Bezier curve
void DData::curve_to(double fn, const Vector2d &c1, const Vector2d &to)
{
	Vector2d prev(0,0);
	for (unsigned long idx = 1;idx <= fn;idx++) {
		const double a = idx * (1.0 / (double)fn);
		Vector2d temp = (pen * pow(1-a, 2) + 
							 c1 * 2 * pow(1-a, 1) * a + 
							 to * pow(a, 2));

		if( idx != 1){
			this->addLine(prev.x(), prev.y(), temp.x(), temp.y());
		}
		else{
			this->addLine(pen.x(), pen.y(), temp.x(), temp.y());
		}
		prev = temp;
	}
	pen = to;
}

// Cubic Bezier curve
void DData::curve_to(double fn, const Vector2d &c1, const Vector2d &c2, const Vector2d &to)
{
	Vector2d prev;
	for (unsigned long idx = 1;idx <= fn;idx++) {
		const double a = idx * (1.0 / (double)fn);
		Vector2d temp = (pen * pow(1-a, 3) + 
							 c1 * 3 * pow(1-a, 2) * a + 
							 c2 * 3 * pow(1-a, 1) * pow(a, 2) + 
							 to * pow(a, 3));
		if( idx != 1){
			this->addLine(prev.x(), prev.y(), temp.x(), temp.y());
		}
		else{
			this->addLine(pen.x(), pen.y(), temp.x(), temp.y());
		}
		prev = temp;
	}
	pen = to;
}

void DData::process_path(){
	// Extract paths from parsed data

	typedef std::map<int, int> LineMap;
	LineMap enabled_lines;
	for (size_t i = 0; i < lines.size(); i++) {
		enabled_lines[i] = i;
	}

	// extract all open paths
	while (enabled_lines.size() > 0) {
		int current_line, current_point;

		for (const auto &l : enabled_lines) {
			int idx = l.second;
			for (int j = 0; j < 2; j++) {
				auto lv = this->grid.data(this->points[lines[idx].idx[j]][0], this->points[lines[idx].idx[j]][1]);
				for (size_t ki = 0; ki < lv.size(); ki++) {
					int k = lv.at(ki);
					if (k == idx || lines[k].disabled) continue;
					goto next_open_path_j;
				}
				current_line = idx;
				current_point = j;
				goto create_open_path;
			next_open_path_j:;
			}
		}

		break;

	create_open_path:
		this->paths.push_back(Path());
		Path *this_path = &this->paths.back();
		this_path->indices.push_back(lines[current_line].idx[current_point]);
		while (1) {
			this_path->indices.push_back(lines[current_line].idx[!current_point]);
			const auto &ref_point = this->points[lines[current_line].idx[!current_point]];
			lines[current_line].disabled = true;
			enabled_lines.erase(current_line);
			auto lv = this->grid.data(ref_point[0], ref_point[1]);
			for (size_t ki = 0; ki < lv.size(); ki++) {
				int k = lv.at(ki);
				if (lines[k].disabled) continue;
				if (this->grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[0]][0], this->points[lines[k].idx[0]][1])) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_open_path;
				}
				if (this->grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[1]][0], this->points[lines[k].idx[1]][1])) {
					current_line = k;
					current_point = 1;
					goto found_next_line_in_open_path;
				}
			}
			break;
		found_next_line_in_open_path:;
		}
	}

    // extract all closed paths
	while (enabled_lines.size() > 0) {
		int current_line = enabled_lines.begin()->second;
		int current_point = 0;

		this->paths.push_back(Path());
		auto& this_path = this->paths.back();
		this_path.is_closed = true;
		this_path.indices.push_back(lines[current_line].idx[current_point]);
		while (1) {
			this_path.indices.push_back(lines[current_line].idx[!current_point]);
			const auto &ref_point = this->points[lines[current_line].idx[!current_point]];
			lines[current_line].disabled = true;
			enabled_lines.erase(current_line);
			auto lv = this->grid.data(ref_point[0], ref_point[1]);
			for (size_t ki = 0; ki < lv.size(); ki++) {
				int k = lv.at(ki);
				if (lines[k].disabled) continue;
				if (this->grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[0]][0], this->points[lines[k].idx[0]][1])) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_closed_path;
				}
					if (this->grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[1]][0], this->points[lines[k].idx[1]][1])) {
					current_line = k;
					current_point = 1;
					goto found_next_line_in_closed_path;
				}
			}
			break;
		found_next_line_in_closed_path:;
		}
	}

	fixup_path_direction();
#if 0
	printf("----- DXF Data -----\n");
	for (int i = 0; i < this->paths.size(); i++) {
		printf("Path %d (%s):\n", i, this->paths[i].is_closed ? "closed" : "open");
		for (int j = 0; j < this->paths[i].points.size(); j++)
			printf("  %f %f\n", (*this->paths[i].points[j])[0], (*this->paths[i].points[j])[1]);
	}
	printf("--------------------\n");
	fflush(stdout);
#endif
}

/*!
	Ensures that all paths have the same vertex ordering.
	FIXME: CW or CCW?
*/
void DData::fixup_path_direction()
{	
	for (size_t i = 0; i < this->paths.size(); i++) {
		if (!this->paths[i].is_closed) break;
		this->paths[i].is_inner = true;
		double min_x = this->points[this->paths[i].indices[0]][0];
		size_t min_x_point = 0;
		for (size_t j = 1; j < this->paths[i].indices.size(); j++) {
			if (this->points[this->paths[i].indices[j]][0] < min_x) {
				min_x = this->points[this->paths[i].indices[j]][0];
				min_x_point = j;
			}
		}
		// rotate points if the path is in non-standard rotation
		size_t b = min_x_point;
		size_t a = b == 0 ? this->paths[i].indices.size() - 2 : b - 1;
		size_t c = b == this->paths[i].indices.size() - 1 ? 1 : b + 1;
		double ax = this->points[this->paths[i].indices[a]][0] - this->points[this->paths[i].indices[b]][0];
		double ay = this->points[this->paths[i].indices[a]][1] - this->points[this->paths[i].indices[b]][1];
		double cx = this->points[this->paths[i].indices[c]][0] - this->points[this->paths[i].indices[b]][0];
		double cy = this->points[this->paths[i].indices[c]][1] - this->points[this->paths[i].indices[b]][1];
#if 0
		printf("Rotate check:\n");
		printf("  a/b/c indices = %d %d %d\n", a, b, c);
		printf("  b->a vector = %f %f (%f)\n", ax, ay, atan2(ax, ay));
		printf("  b->c vector = %f %f (%f)\n", cx, cy, atan2(cx, cy));
#endif
		// FIXME: atan2() usually takes y,x. This variant probably makes the path clockwise..
		if (atan2(ax, ay) < atan2(cx, cy)) {
			std::reverse(this->paths[i].indices.begin(), this->paths[i].indices.end());
		}
	}
}

/*!
	Adds a vertex and returns the index into DData::points
 */
int DData::addPoint(double x, double y)
{
	this->points.emplace_back(x, y);
	return this->points.size() - 1;
}

std::string DData::dump() const
{
	std::ostringstream out;
	out << "DData"
	  << "\n num points: " << points.size()
	  << "\n num paths: " << paths.size()
	  << "\n num dims: " << dims.size()
	  << "\n points: ";
	for (size_t k = 0; k < points.size(); k++ ) {
		out << "\n  x y: " << points[k].transpose();
	}
	out << "\n paths: ";
	for (size_t i = 0; i < paths.size(); i++) {
		out << "\n  path:" << i
		  << "\n  is_closed: " << paths[i].is_closed
		  << "\n  is_inner: " << paths[i].is_inner ;
		DData::Path path = paths[i];
		for (size_t j = 0; j < path.indices.size(); j++) {
			out << "\n  index[" << j << "]==" << path.indices[j];
		}
	}
	out << "\nDData end";
	return out.str();
}

/*
    May return an empty polygon, but will not return nullptr
 */
Polygon2d *DData::toPolygon2d() const
{
	auto poly = new Polygon2d();
	for (size_t i = 0; i < this->paths.size(); i++) {
		const auto &path = this->paths[i];
		Outline2d outline;
		size_t endidx = path.indices.size();
		// We don't support open paths; closing them to be compatible with existing behavior
		if (!path.is_closed) endidx++;
		for (size_t j = 1; j < endidx; j++) {
			outline.vertices.push_back(this->points[path.indices[path.indices.size()-j]]);
		}
		poly->addOutline(outline);
	}
	return poly;
}

void DData::addLine(double x1, double y1, double x2, double y2){
	if(this->xorigin != 0.0 || this->yorigin != 0.0){
		x1 = x1 - xorigin*scale;
		y1 = y1 - yorigin*scale;
		x2 = x2 - xorigin*scale;
		y2 = y2 - yorigin*scale;
	}
    this->grid.align(x1, y1);                                 
    this->grid.align(x2, y2);                                 
    this->grid.data(x1, y1).push_back(lines.size());         
    this->grid.data(x2, y2).push_back(lines.size());                                        
	lines.emplace_back(addPoint(x1, y1), addPoint(x2, y2)); 
}

Polygon2d *import_dxf( double fn, double fs, double fa, const std::string &filename, const std::string &layername,
						double xorigin, double yorigin, double scale)
{

    DData dxf;
	std::unordered_map<std::string, std::vector<Line>> blockdata; // Lines in blocks
	dxf.yorigin = yorigin;
	dxf.xorigin = xorigin;
	dxf.scale = scale;   
	
	std::ifstream stream(filename.c_str());
	if (!stream.good()) {
		PRINTB("WARNING: Can't open DXF file '%s'.", filename);  
        auto poly = new Polygon2d();
        return poly;
	}

    dxf_data dd = read_dxf_file(filename, "", scale);

    std::vector<header_struct> header_vector;
    std::vector<polyline_vertex_struct> polyline_vertex_vector;
    std::vector<polyline_struct> polyline_vector;
    std::vector<lwpolyline_struct> lwpolyline_vector;
    std::vector<circle_struct> circle_vector;
    std::vector<face3d_struct> face3d_vector;
    std::vector<line_struct> line_vector;
    std::vector<point_struct> point_vector;
    std::vector<arc_struct> arc_vector;
    std::vector<text_struct> text_vector;
    std::vector<solid_struct> solid_vector;
    std::vector<mtext_struct> mtext_vector;
    std::vector<text_attrib_struct> text_attrib_vector;
    std::vector<ellipse_struct> ellipse_vector;
    std::vector<leader_struct> leader_vector;
    std::vector<spline_struct> spline_vector;

    circle_vector =  dd.return_circle_vector();
	for(auto it : circle_vector){
		if(layername.empty() || it.layer_name == layername){
			int n = Calc::get_fragments_from_r(it.radius, fn, fs, fa);
			Vector2d center(it.center[0], it.center[1]);
			for (int i = 0; i < n; i++) {
				double a1 = (360.0 * i) / n;
				double a2 = (360.0 *(i + 1)) / n;
				dxf.addLine(cos_degrees(a1)*it.radius + it.center[0], sin_degrees(a1)*it.radius + it.center[1],
							cos_degrees(a2)*it.radius + it.center[0], sin_degrees(a2)*it.radius + it.center[1]);
			}				
		}
	}

    arc_vector = dd.return_arc_vector();
	for(auto it : arc_vector){
		if(layername.empty() || it.layer_name == layername){
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
				dxf.addLine(cos_degrees(a1)*it.radius + center[0], sin_degrees(a1)*it.radius + center[1],
							cos_degrees(a2)*it.radius + center[0], sin_degrees(a2)*it.radius + center[1]);
			}
		}
	}
    
    ellipse_vector = dd.return_ellipse_vector();
	for(auto it : ellipse_vector){
		if(layername.empty() || it.layer_name == layername){
			// Commented code is meant as documentation of vector math
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
					dxf.addLine(p1[0], p1[1],p2_rot[0], p2_rot[1]);
				}
	//					p1 = p2;
				p1[0] = p2_rot[0];
				p1[1] = p2_rot[1];            
			}
		}
	}   

	// for spline knots, weights, fit points are ignored 
    spline_vector = dd.return_spline_vector();
	for(auto it : spline_vector){
		if(layername.empty() || it.layer_name == layername){
			if(it.degree == 1){
				// polyline
				for(unsigned int i = 1; i < it.numCtlPts ; i++){
					dxf.addLine(it.ctlPts.at(i-1).spoints[0], it.ctlPts.at(i-1).spoints[1], 
								it.ctlPts.at(i).spoints[0], it.ctlPts.at(i).spoints[1]);
					if(it.flag == 1){
						dxf.addLine(it.ctlPts.front().spoints[0], it.ctlPts.front().spoints[1],
									it.ctlPts.back().spoints[0], it.ctlPts.back().spoints[1]);
					}						
				}
			}
			else if(it.degree == 2){
				// Quadratic Bezier curve
				dxf.pen = Vector2d(it.ctlPts.at(0).spoints[0], it.ctlPts.at(0).spoints[1]);
				int step = 2;
				int remain = it.numCtlPts % 3;
				if(remain != 0){
					PRINTD("Degree 2 spline with number of control points can not be divisible by 3 may cause incorrect geometry!");
				}
				for(unsigned int i = 2; i < it.numCtlPts; (i += step)){
					if(i >= it.numCtlPts-remain){
						step = 1;
					}
					dxf.curve_to(it.splineSegs, Vector2d(it.ctlPts.at(i-1).spoints[0], it.ctlPts.at(i-1).spoints[1]),
										Vector2d(it.ctlPts.at(i).spoints[0], it.ctlPts.at(i).spoints[1]));
				}
				if(it.flag == 1){
					dxf.curve_to(it.splineSegs, Vector2d(it.ctlPts.back().spoints[0], it.ctlPts.back().spoints[1]),
								Vector2d(it.ctlPts.at(0).spoints[0], it.ctlPts.at(0).spoints[1]));
				}
			}
			else if(it.degree == 3){
				// Cubic Bezier curve
				dxf.pen = Vector2d(it.ctlPts.at(0).spoints[0], it.ctlPts.at(0).spoints[1]);

				int step = 3;
				int remain = it.numCtlPts % 4;
				if(remain != 0){
					PRINTD("Degree 3 spline with number of control points can not be divisible by 4 may cause incorrect geometry!");
				}
				for(unsigned int i = 3; i < it.numCtlPts; (i += step)){
					if(i >= it.numCtlPts-remain){
						step = 1;
					}
					dxf.curve_to(it.splineSegs, Vector2d(it.ctlPts.at(i-2).spoints[0], it.ctlPts.at(i-2).spoints[1]),
								Vector2d(it.ctlPts.at(i-1).spoints[0], it.ctlPts.at(i-1).spoints[1]),
								Vector2d(it.ctlPts.at(i).spoints[0], it.ctlPts.at(i).spoints[1]));
				}
				if(it.flag == 1){
					dxf.curve_to(it.splineSegs, Vector2d(it.ctlPts.at(it.numCtlPts-2).spoints[0], it.ctlPts.at(it.numCtlPts-2).spoints[1]),
								Vector2d(it.ctlPts.at(it.numCtlPts-1).spoints[0], it.ctlPts.at(it.numCtlPts-1).spoints[1]),
								Vector2d(it.ctlPts.at(0).spoints[0], it.ctlPts.at(0).spoints[1]));
				}
			}
			else{
				// degree > 3 does not support
				PRINTD("Spline entity only supports degree 1 to degree 3!");
			}
		}
	}

    lwpolyline_vector = dd.return_lwpolyline_vector();
	for(auto it : lwpolyline_vector){
		if(layername.empty() || it.layer_name == layername){
			for(auto it_pt = it.lw_pt_vec.begin(); it_pt != it.lw_pt_vec.end(); it_pt++){
				if(std::next(it_pt) != it.lw_pt_vec.end()){
					dxf.addLine(it_pt->x, it_pt->y, std::next(it_pt)->x, std::next(it_pt)->y);
				}
			
			}
			// if polyline_flag == 1, the last vertex connect to the first vertex
			// closed lwpolyline flag
			if (it.polyline_flag & 0x01){
				dxf.addLine(it.lw_pt_vec.back().x, it.lw_pt_vec.back().y,
							it.lw_pt_vec.front().x, it.lw_pt_vec.front().y);
			}
		}
	}

	polyline_vector = dd.return_polyline_vector();
	for(auto it : polyline_vector){
		if(layername.empty() || it.layer_name == layername){
			// All 3D feature of polyline are not supported
			if(it.polyline_flag == POLYLINE_POLYFACE_MESH ||
			   it.polyline_flag == POLYLINE_3D_POLYLINE ||
			   it.polyline_flag == POLYLINE_3D_POLYMESH){
				PRINTD("All 3D polyline features are ignored, only 2D polyline is imported!");
			}
			if(it.polyline_flag == POLYLINE_CURVE_FIT_VERTICES_ADDED){
				PRINTD("Polyline with curve fit is ignored!");
			}
			if(it.polyline_flag == POLYLINE_SPLINE_FIT_VERTICES_ADDED){
				
				bool spline_vertex_found = false;
				// polyline created by spline vertex
				polyline_vertex_struct tmp_vert = it.vertex_vec.front();
				for(auto it_pt : it.vertex_vec){
					if(it_pt.vertex_flage == VTX_SPLINE_VERTEX_CREATED){
						dxf.addLine(tmp_vert.x, tmp_vert.y, it_pt.x, it_pt.y);
						tmp_vert = it_pt;
						spline_vertex_found = true;
					}
				}

				// if spline vertex not found, polyline created by control points
				// Cubic Bezier curve is used for polyline spline-fit
				if(!spline_vertex_found){
					dxf.pen = Vector2d(it.vertex_vec.at(0).x, it.vertex_vec.at(0).y);
					int step = 3;
					int remain = it.vertex_vec.size() % 4;
					if(remain != 0){
						PRINTD("Spline-fit polyline with number of control points vertex can not be divisible by 4 may cause incorrect geometry!");
					}
					for(unsigned int i = 3; i < it.vertex_vec.size(); i += step){
						if(i >= it.vertex_vec.size()-remain){
							step = 1;
						}
						dxf.curve_to(it.splineSegs, Vector2d(it.vertex_vec.at(i-2).x, it.vertex_vec.at(i-2).y),
										Vector2d(it.vertex_vec.at(i-1).x, it.vertex_vec.at(i-1).y),
										Vector2d(it.vertex_vec.at(i).x, it.vertex_vec.at(i).y));
					}
				}				
			}
			else{
				for(auto it_pt = it.vertex_vec.begin(); it_pt != it.vertex_vec.end(); it_pt++){
					if(std::next(it_pt) != it.vertex_vec.end()){
						dxf.addLine(it_pt->x, it_pt->y, std::next(it_pt)->x, std::next(it_pt)->y);
					}
				}				
				//polyline is closed
				if(it.polyline_flag & 0x01){
					dxf.addLine(it.vertex_vec.front().x, it.vertex_vec.front().y,
								it.vertex_vec.back().x, it.vertex_vec.back().y);
				}
			}
		}
	}

    line_vector = dd.return_line_vector();
	for(auto it : line_vector){
		if(layername.empty() || it.layer_name == layername){
			dxf.addLine(it.line_pt[0][0], it.line_pt[0][1],
						it.line_pt[1][0], it.line_pt[1][1]);
		}
	}

    point_vector = dd.return_point_vector();
    if(!point_vector.empty()){
        PRINTD("Point Entity is ignored!");
    }

    dxf.process_path();
    return dxf.toPolygon2d();
};
