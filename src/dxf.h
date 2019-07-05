/*                           D X F . H
 * BRL-CAD
 *
 * Copyright (c) 2008-2019 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file dxf.h
 *
 * Structures and data common to the DXF importer and exporter.
 *
 */

#ifndef CONV_DXF_DXF_H
#define CONV_DXF_DXF_H

static unsigned char rgb[]={
    0, 0, 0,
    255, 0, 0,
    255, 255, 0,
    0, 255, 0,
    0, 255, 255,
    0, 0, 255,
    255, 0, 255,
    255, 255, 255,
    65, 65, 65,
    128, 128, 128,
    255, 0, 0,
    255, 128, 128,
    166, 0, 0,
    166, 83, 83,
    128, 0, 0,
    128, 64, 64,
    77, 0, 0,
    77, 38, 38,
    38, 0, 0,
    38, 19, 19,
    255, 64, 0,
    255, 159, 128,
    166, 41, 0,
    166, 104, 83,
    128, 32, 0,
    128, 80, 64,
    77, 19, 0,
    77, 48, 38,
    38, 10, 0,
    38, 24, 19,
    255, 128, 0,
    255, 191, 128,
    166, 83, 0,
    166, 124, 83,
    128, 64, 0,
    128, 96, 64,
    77, 38, 0,
    77, 57, 38,
    38, 19, 0,
    38, 29, 19,
    255, 191, 0,
    255, 223, 128,
    166, 124, 0,
    166, 145, 83,
    128, 96, 0,
    128, 112, 64,
    77, 57, 0,
    77, 67, 38,
    38, 29, 0,
    38, 33, 19,
    255, 255, 0,
    255, 255, 128,
    166, 166, 0,
    166, 166, 83,
    128, 128, 0,
    128, 128, 64,
    77, 77, 0,
    77, 77, 38,
    38, 38, 0,
    38, 38, 19,
    191, 255, 0,
    223, 255, 128,
    124, 166, 0,
    145, 166, 83,
    96, 128, 0,
    112, 128, 64,
    57, 77, 0,
    67, 77, 38,
    29, 38, 0,
    33, 38, 19,
    128, 255, 0,
    191, 255, 128,
    83, 166, 0,
    124, 166, 83,
    64, 128, 0,
    96, 128, 64,
    38, 77, 0,
    57, 77, 38,
    19, 38, 0,
    29, 38, 19,
    64, 255, 0,
    159, 255, 128,
    41, 166, 0,
    104, 166, 83,
    32, 128, 0,
    80, 128, 64,
    19, 77, 0,
    48, 77, 38,
    10, 38, 0,
    24, 38, 19,
    0, 255, 0,
    128, 255, 128,
    0, 166, 0,
    83, 166, 83,
    0, 128, 0,
    64, 128, 64,
    0, 77, 0,
    38, 77, 38,
    0, 38, 0,
    19, 38, 19,
    0, 255, 64,
    128, 255, 159,
    0, 166, 41,
    83, 166, 104,
    0, 128, 32,
    64, 128, 80,
    0, 77, 19,
    38, 77, 48,
    0, 38, 10,
    19, 38, 24,
    0, 255, 128,
    128, 255, 191,
    0, 166, 83,
    83, 166, 124,
    0, 128, 64,
    64, 128, 96,
    0, 77, 38,
    38, 77, 57,
    0, 38, 19,
    19, 38, 29,
    0, 255, 191,
    128, 255, 223,
    0, 166, 124,
    83, 166, 145,
    0, 128, 96,
    64, 128, 112,
    0, 77, 57,
    38, 77, 67,
    0, 38, 29,
    19, 38, 33,
    0, 255, 255,
    128, 255, 255,
    0, 166, 166,
    83, 166, 166,
    0, 128, 128,
    64, 128, 128,
    0, 77, 77,
    38, 77, 77,
    0, 38, 38,
    19, 38, 38,
    0, 191, 255,
    128, 223, 255,
    0, 124, 166,
    83, 145, 166,
    0, 96, 128,
    64, 112, 128,
    0, 57, 77,
    38, 67, 77,
    0, 29, 38,
    19, 33, 38,
    0, 128, 255,
    128, 191, 255,
    0, 83, 166,
    83, 124, 166,
    0, 64, 128,
    64, 96, 128,
    0, 38, 77,
    38, 57, 77,
    0, 19, 38,
    19, 29, 38,
    0, 64, 255,
    128, 159, 255,
    0, 41, 166,
    83, 104, 166,
    0, 32, 128,
    64, 80, 128,
    0, 19, 77,
    38, 48, 77,
    0, 10, 38,
    19, 24, 38,
    0, 0, 255,
    128, 128, 255,
    0, 0, 166,
    83, 83, 166,
    0, 0, 128,
    64, 64, 128,
    0, 0, 77,
    38, 38, 77,
    0, 0, 38,
    19, 19, 38,
    64, 0, 255,
    159, 128, 255,
    41, 0, 166,
    104, 83, 166,
    32, 0, 128,
    80, 64, 128,
    19, 0, 77,
    48, 38, 77,
    10, 0, 38,
    24, 19, 38,
    128, 0, 255,
    191, 128, 255,
    83, 0, 166,
    124, 83, 166,
    64, 0, 128,
    96, 64, 128,
    38, 0, 77,
    57, 38, 77,
    19, 0, 38,
    29, 19, 38,
    191, 0, 255,
    223, 128, 255,
    124, 0, 166,
    145, 83, 166,
    96, 0, 128,
    112, 64, 128,
    57, 0, 77,
    67, 38, 77,
    29, 0, 38,
    33, 19, 38,
    255, 0, 255,
    255, 128, 255,
    166, 0, 166,
    166, 83, 166,
    128, 0, 128,
    128, 64, 128,
    77, 0, 77,
    77, 38, 77,
    38, 0, 38,
    38, 19, 38,
    255, 0, 191,
    255, 128, 223,
    166, 0, 124,
    166, 83, 145,
    128, 0, 96,
    128, 64, 112,
    77, 0, 57,
    77, 38, 67,
    38, 0, 29,
    38, 19, 33,
    255, 0, 128,
    255, 128, 191,
    166, 0, 83,
    166, 83, 124,
    128, 0, 64,
    128, 64, 96,
    77, 0, 38,
    77, 38, 57,
    38, 0, 19,
    38, 19, 29,
    255, 0, 64,
    255, 128, 159,
    166, 0, 41,
    166, 83, 104,
    128, 0, 32,
    128, 64, 80,
    77, 0, 19,
    77, 38, 48,
    38, 0, 10,
    38, 19, 24,
    84, 84, 84,
    118, 118, 118,
    152, 152, 152,
    187, 187, 187,
    221, 221, 221,
    255, 255, 255
};

struct header_struct{
	int units;
	int color_by_layer;
	int splineSegs;
};

struct table_struct{
	std::string layer_name;
	int color;
};

struct polyline_vertex_struct{
	int face[4];
	int vertex_flage;
	double x, y, z;
	int color;
	std::string layer_name;
};

struct polyline_struct{
	int polyline_flag;
	int mesh_m_count;
	int mesh_n_count;
	int color;
	int invisible;
	std::string layer_name;
};

struct face3d_struct{
	double pts[4][3];
	int color;
	std::string layer_name;
};

struct line_struct{
	double line_pt[2][3];
	int color;
	std::string layer_name;
};

struct insert_data {
    double scale[3];
    double rotation;
    double insert_pt[3];
	double extrude_dir[4];
};


struct insert_struct{
	double scale[3];
    double rotation;
    double insert_pt[3];
	double extrude_dir[4];
	int color;
	std::string layer_name;
};

struct point_struct{
	double pt[3];
	std::string layer_name;
};

struct circle_struct{
	double center[3];
	double radius;
	int color;
	std::string layer_name;
};

struct arc_struct{
	double center[3];
	double radius;
	double start_angle, end_angle;
	int color;
	std::string layer_name;
};

struct text_struct{
	std::string theText;
	double firstAlignmentPoint[3];
	double secondAlignmentPoint[3];
	double textScale;
	double textHeight;
	double textRotation;
	int color;
	int textFlag;
	int vertAlignment;
	int horizAlignment;
	std::string layer_name;
};

struct solid_struct{
	double solid_pt[4][3];
	int color;
	std::string layer_name;
};

struct lwpolyline_struct{
    struct lw_pt{
        double x;
        double y;
        lw_pt(double x, double y): x(x), y(y){

        };
    };
    std::vector<lw_pt> lw_pt_vec;
	int color;
	int polyline_flag;
	std::string layer_name;
};

struct mtext_struct{
	std::string vls;
	double insertionPoint[3];
	double xAxisDirection[3];
	double textHeight;
	double entityHeight;
	double charWidth;
	double rectWidth;
	double rotationAngle;
	int attachPoint;
	int drawingDirection;
	int color;
	std::string layer_name;
};

struct text_attrib_struct{
	std::string theText;
	double firstAlignmentPoint[3];
	double secondAlignmentPoint[3];
	double textScale;
	double textHeight;
	double textRotation;
	int color;
	int textFlag;
	int vertAlignment;
	int horizAlignment;
	std::string layer_name;
};

struct ellipse_struct{
	double center[3];
	double majorAxis[3];
	double ratio;
	double start_angle, end_angle;
	int color;
	std::string layer_name;
};

struct leader_struct{
	double pt[3];
	int arrrowHeadFlag;
	int color;
	std::string layer_name;
};

struct spline_struct{
	struct spline_pts{
		double spoints[3];
		double weights;
	};
	int flag;
	int degree;
	int numKnots;
	int numCtlPts;
	int numFitPts;
	std::vector<double> knots;
	std::vector<spline_pts> ctlPts;
	std::vector<spline_pts> fitPts;
	int color;
	std::string layer_name;
};

struct dimension_struct{
	std::string block_name;
	std::string layer_name;
};

void read_dxf_file(std::string in_filename, std::string out_filename);

std::vector<polyline_vertex_struct> return_polyline_vertex_vector();
std::vector<polyline_struct> return_polyline_vector();
std::vector<lwpolyline_struct> return_lwpolyline_vector();
std::vector<circle_struct> return_circle_vector();
std::vector<face3d_struct> return_face3d_vector();
std::vector<line_struct> return_line_vector();
std::vector<insert_struct> return_insert_vector();
std::vector<point_struct> return_point_vector();
std::vector<arc_struct> return_arc_vector();
std::vector<text_struct> return_text_vector();
std::vector<solid_struct> return_solid_vector();
std::vector<mtext_struct> return_mtext_vector();
std::vector<text_attrib_struct> return_text_attrib_vector();
std::vector<ellipse_struct> return_ellipse_vector();
std::vector<leader_struct> return_leader_vector();
std::vector<spline_struct> return_spline_vector();
std::vector<dimension_struct> return_dimension_vector();

#endif /* CONV_DXF_DXF_H */


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
