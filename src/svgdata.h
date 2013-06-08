/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2013 Felipe Sanches <fsanches@metamaquina.com.br>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SVGDATA_H_
#define SVGDATA_H_

#include <string>
#include <math.h>
#include <polyset.h>
#include <dxfdata.h>
#include <libxml++/libxml++.h>

class TransformMatrix {
public:
  TransformMatrix(){
    setIdentity();
  };

  void setIdentity(){
    //initialize values with identity matrix;
    // [a c e]   [1 0 0]
    // [b d f] = [0 1 0]
    // [0 0 1]   [0 0 1]
    a=1;
    b=0;
    c=0;
    d=1;
    e=0;
    f=0;
  };

  TransformMatrix(float a, float b, float c, float d, float e, float f) : a(a), b(b), c(c), d(d), e(e), f(f) {};

  TransformMatrix operator*(const TransformMatrix & tm) const {
    TransformMatrix result;
    result.a = a * tm.a + c * tm.b;
    result.b = b * tm.a + d * tm.b;
    result.c = a * tm.c + c * tm.d;
    result.d = b * tm.c + d * tm.d;
    result.e = a * tm.e + c * tm.f + e;
    result.f = b * tm.e + d * tm.f + f;
    return result;
  }

  void applyTransform(float *x, float *y){
    float tmpx = *x;
    float tmpy = *y;

    *x = a*tmpx + c*tmpy + e;
    *y = b*tmpx + d*tmpy + f;
  }

  void translate(float tx, float ty){
    TransformMatrix translation_matrix(1,0,0,1,tx,ty);
    *this = *this * translation_matrix;
  }

  void scale(float sx, float sy){
    TransformMatrix scaling_matrix(sx,0,0,sy,0,0);
    *this = *this * scaling_matrix;
  }

  void rotate(float angle, float cx, float cy){
    TransformMatrix rotation_matrix(cos(angle), sin(angle), -sin(angle), cos(angle), 0, 0);

    translate(cx, cy);
    *this = *this * rotation_matrix;
    translate(-cx, -cy);
  }

  void skewX(float angle){
    TransformMatrix skewX_matrix(1, 0, tan(angle), 1, 0, 0);
    *this = *this * skewX_matrix;
  }

  void skewY(float angle){
    TransformMatrix skewY_matrix(1, tan(angle), 0, 1, 0, 0);
    *this = *this * skewY_matrix;
  }

private:
  float a, b, c, d, e, f;
};

class SVGData{
public:
  SVGData(double fn, double fs, double fa, std::string filename);
  ~SVGData();

  class PolySet* convertToPolyset();
  void traverse_subtree(TransformMatrix parent_matrix, const xmlpp::Node* node);

private:
  void start_path();
  void close_path();
  void add_point(float x, float y);
  void add_arc_points(float xc, float yc, float rx, float ry, float start, float end);

  void parse_path_description(Glib::ustring d);
  std::vector<float> get_params(std::string str);
  void render_rect(float x, float y, float width, float height, float rx, float ry);
  void render_line_to(float x0, float y0, float x1, float y1);
  void render_quadratic_curve_to(float x0, float y0, float x1, float y1, float x2, float y2);
  void render_cubic_curve_to(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
  void render_elliptical_arc(float x0, float y0, float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y);

  float quadratic_curve_length(float x0, float y0, float x1, float y1, float x2, float y2);

  TransformMatrix parse_transform(std::string transform);
  void setCurrentTransformMatrix(TransformMatrix tm){
    ctm = tm;
  }

  std::string filename;
  xmlpp::DomParser* parser;
  DxfData *dxfdata;
  PolySet *p;
  Grid2d<int>* grid;
  float document_height;
  int first_point, last_point;
  double fn, fs, fa;
  TransformMatrix ctm;
};

#endif
