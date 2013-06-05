/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include <math.h>
#include <svgdata.h>
#include <dxfdata.h>
#include <polyset.h>
#include "dxftess.h"
#include "handle_dep.h"
#include "printutils.h"
#include <fstream>
#include <boost/regex.hpp>

SVGData::SVGData(double fn, double fs, double fa, std::string filename) : filename(filename), fa(fa), fs(fs), fn(fn) {
	handle_dep(filename); // Register ourselves as a dependency

  parser = NULL;
  dxfdata = new DxfData();
  p = new PolySet();
  grid = new Grid2d<int>(GRID_COARSE);

  try{
    parser = new xmlpp::DomParser();
    parser->parse_file(filename);
  }
  catch(const std::exception& ex)
  {
    std::cout << "Exception caught: " << ex.what() << std::endl;
  }
}

SVGData::~SVGData(){
  free(dxfdata);
  free(grid);

  if (parser)
    free(parser);
}

void SVGData::add_point(float x, float y){
  ctm.applyTransform(&x, &y);
  double px = (double) x;
  double py = (double) (document_height - y);

  int this_point = -1;
  if (grid->has(x, y)) {
	  this_point = grid->align(px, py);
  } else {
	  this_point = grid->align(px, py) = dxfdata->points.size();
	  dxfdata->points.push_back(Vector2d(px, py));
  }
  if (first_point < 0) {
	  dxfdata->paths.push_back(DxfData::Path());
	  first_point = this_point;
  }
  if (this_point != last_point) {
	  dxfdata->paths.back().indices.push_back(this_point);
	  last_point = this_point;
  }
}

void SVGData::start_path(){
  first_point = -1;
  last_point = -1;
}

void SVGData::close_path(){
  if (first_point >= 0) {
	  dxfdata->paths.back().is_closed = 1;
	  dxfdata->paths.back().indices.push_back(first_point);
  }
}

//TODO: implement another method specifically
// for parsing the elliptical arc parameters
// because their sintax is slightly different and would
// probably make the generic regex get too much complicated.
std::vector<float> SVGData::get_params(std::string str){
  std::string pattern = "\\s*(-?[0-9]+(\\.[0-9]+)?(e-?[0-9]+)?),(-?[0-9]+(\\.[0-9]+)?(e-?[0-9]+)?)";
  boost::regex regexPattern(pattern);
  boost::match_results<std::string::const_iterator> result;

  std::string::const_iterator start, end;
  start = str.begin();
  end = str.end();

  std::vector<float> values;
  float value;

  while(boost::regex_search(start, end, result, regexPattern)){
    value = atof(((std::string) result[1]).c_str());
    values.push_back(value);
//    std::cout << "value1=" << value << std::endl;

    value = atof(((std::string) result[4]).c_str());
    values.push_back(value);
//    std::cout << "value2=" << value << std::endl;

    start = result[4].second;
  }

  return values;
}

void SVGData::render_rect(float x, float y, float width, float height){
  start_path();
  add_point(x,y);
  add_point(x+width,y);
  add_point(x+width, y+height);
  add_point(x, y+height);
  close_path();
}

void SVGData::render_line_to(float x0, float y0, float x1, float y1){
//  std::cout << "render line: x0:" << x0 << " y0:" << y0 << " x1:" << x1 << " y1:" << y1 << std::endl;
  add_point(x0,y0);
  add_point(x1,y1);
}

void SVGData::render_quadratic_curve_to(float x0, float y0, float x1, float y1, float x2, float y2){
  //TODO: This only deals with $fn. Add some logic to support $fa and $fs
  for (int i=0; i<=fn; i++){
    double t = i/fn;
    add_point((1-t)*(1-t)*x0 + 2*(1-t)*x1*t + x2*t*t,
              (1-t)*(1-t)*y0 + 2*(1-t)*y1*t + y2*t*t);
  }
}

void SVGData::render_cubic_curve_to(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3){
  //TODO: This only deals with $fn. Add some logic to support $fa and $fs
  for (int i=0; i<=fn; i++){
    double t = i/fn;
    add_point((1-t)*(1-t)*(1-t)*x0 + 3*(1-t)*(1-t)*x1*t + 3*(1-t)*x2*t*t + x3*t*t*t,
              (1-t)*(1-t)*(1-t)*y0 + 3*(1-t)*(1-t)*y1*t + 3*(1-t)*y2*t*t + y3*t*t*t);
  }
}

void SVGData::render_elliptical_arc(float x0, float y0, float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y){
  //TODO: This only deals with $fn. Add some logic to support $fa and $fs
  for (int i=0; i<=fn; i++){
    double t = i/fn;
    //TODO: implement-me!
  }
}

void SVGData::parse_path_description(Glib::ustring description){
  std::string d = (std::string) description;
  std::string pattern = "(([mMzZlLhHvVcCsSqQtTaA])([^mMzZlLhHvVcCsSqQtTaA]*))";
  boost::regex regexPattern(pattern);
  boost::match_results<std::string::const_iterator> result;

  std::string::const_iterator start, end;
  start = d.begin();
  end = d.end();

  float x=0, y=0;
  while(boost::regex_search(start, end, result, regexPattern)){
//    std::cout << "result: " << std::endl << result[1] << std::endl << std::endl;

    char instruction_code = ((std::string) result[2]).c_str()[0];
    std::vector<float> params = get_params(((std::string) result[3]).c_str());

//    std::cout << "instruction_code=" << instruction_code << std::endl;

    int idx=0;
    switch (instruction_code){
      case 'm':
        //"moveto" command - relative

        //Since we're doing a move, we must close the path we may have drawn so far
        close_path();

        //And prepare to start drawing something else from our new starting point after the move command
        start_path();

        while (params.size() - idx >= 2){
          if (idx>0)
            render_line_to(x, y, x+params[idx], y+params[idx+1]);
          x += params[idx];
          y += params[idx+1];
          //std::cout << "m: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'M':
        //"moveto" command - absolute

        //Since we're doing a move, we must close the path we may have drawn so far
        close_path();

        //And prepare to start drawing something else from our new starting point after the move command
        start_path();

        while (params.size() - idx >= 2){
          if (idx>0)
            render_line_to(x, y, params[idx], params[idx+1]);
          x = params[idx];
          y = params[idx+1];
          std::cout << "M: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'z':
      case 'Z':
        //"closepath" command
        //do nothing?
        break;
      case 'l':
        //"lineto" command - relative
        while (params.size() - idx >= 2){
          render_line_to(x, y, x+params[idx], y+params[idx+1]);
          x += params[idx];
          y += params[idx+1];
          //std::cout << "l: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'L':
        //"lineto" command - absolute
        while (params.size() - idx >= 2){
          render_line_to(x, y, params[idx], params[idx+1]);
          x = params[idx];
          y = params[idx+1];
          //std::cout << "L: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'h':
        //horizontal "lineto" command - relative
        while (params.size() - idx >= 1){
          render_line_to(x, y, x+params[idx], y);
          x += params[idx];
          //std::cout << "h: x=" << x << " y=" << y << std::endl;
          idx++;
        }
        break;
      case 'H':
        //horizontal "lineto" command - absolute
        while (params.size() - idx >= 1){
          render_line_to(x, y, params[idx], y);
          x = params[idx];
          //std::cout << "H: x=" << x << " y=" << y << std::endl;
          idx++;
        }
        break;
      case 'v':
        //vertical "lineto" command - relative
        while (params.size() - idx >= 1){
          render_line_to(x, y, x, y+params[idx]);
          y += params[idx];
          //std::cout << "v: x=" << x << " y=" << y << std::endl;
          idx++;
        }
        break;
      case 'V':
        //vertical "lineto" command - absolute
        while (params.size() - idx >= 1){
          render_line_to(x, y, x, params[idx]);
          y = params[idx];
          //std::cout << "V: x=" << x << " y=" << y << std::endl;
          idx++;
        }
        break;
      case 'c':
        //"curveto" cubic Bézier command - relative
        while (params.size() - idx >= 6){
          render_cubic_curve_to(x, y, x+params[idx], y+params[idx+1], x+params[idx+2], y+params[idx+3], x+params[idx+4], y+params[idx+5]);
          x += params[idx+4];
          y += params[idx+5];
          //std::cout << "c: x=" << x << " y=" << y << std::endl;
          idx+=6;
        }
        break;
      case 'C':
        //"curveto" cubic Bézier command - absolute
        while (params.size() - idx >= 6){
          render_cubic_curve_to(x, y, params[idx], params[idx+1], params[idx+2], params[idx+3], params[idx+4], params[idx+5]);
          x = params[idx+4];
          y = params[idx+5];
          //std::cout << "C: x=" << x << " y=" << y << std::endl;
          idx+=6;
        }
        break;
      case 's':
        //"shorthand/smooth curveto" cubic Bézier command - relative
        while (params.size() - idx >= 4){
          //this is wrong! TODO: implement reflection of previous control point as described in the SVG spec.
          render_cubic_curve_to(x, y, x, y, x+params[idx], y+params[idx+1], x+params[idx+2], y+params[idx+3]);
          x += params[idx+2];
          y += params[idx+3];
          //std::cout << "s: x=" << x << " y=" << y << std::endl;
          idx+=4;
        }
        break;
      case 'S':
        //"shorthand/smooth curveto" cubic Bézier command - absolute
        while (params.size() - idx >= 4){
          //this is wrong! TODO: implement reflection of previous control point as described in the SVG spec.
          render_cubic_curve_to(x, y, x, y, params[idx], params[idx+1], params[idx+2], params[idx+3]);
          x = params[idx+2];
          y = params[idx+3];
          //std::cout << "S: x=" << x << " y=" << y << std::endl;
          idx+=4;
        }
        break;
      case 'q':
        //"curveto" quadratic Bézier command - relative
        while (params.size() - idx >= 4){
          render_quadratic_curve_to(x, y, x+params[idx], y+params[idx+1], x+params[idx+2], y+params[idx+3]);
          x += params[idx+2];
          y += params[idx+3];
          //std::cout << "q: x=" << x << " y=" << y << std::endl;
          idx+=4;
        }
        break;
      case 'Q':
        //"curveto" quadratic Bézier command - absolute
        while (params.size() - idx >= 4){
          render_quadratic_curve_to(x, y, params[idx], params[idx+1], params[idx+2], params[idx+3]);
          x = params[idx+2];
          y = params[idx+3];
          //std::cout << "Q: x=" << x << " y=" << y << std::endl;
          idx+=4;
        }
        break;
      case 't':
        //"shorthand/smooth curveto" quadratic Bézier command - relative
        while (params.size() - idx >= 2){
          //this is wrong! TODO: implement reflection of previous control point as described in the SVG spec.
          render_quadratic_curve_to(x, y, x, y, x+params[idx], y+params[idx+1]);
          x += params[idx];
          y += params[idx+1];
          //std::cout << "t: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'T':
        //"shorthand/smooth curveto" quadratic Bézier command - absolute
        while (params.size() - idx >= 2){
          //this is wrong! TODO: implement reflection of previous control point as described in the SVG spec.
          render_quadratic_curve_to(x, y, x, y, params[idx], params[idx+1]);
          x = params[idx];
          y = params[idx+1];
          //std::cout << "T: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'a':
        //"elliptical arc" command - relative
        while (params.size() - idx >= 7){
          render_elliptical_arc(x, y, params[idx], params[idx+1], params[idx+2], params[idx+3], params[idx+4], x+params[idx+5], y+params[idx+6]);
          x += params[idx+5];
          y += params[idx+6];
          //std::cout << "a: x=" << x << " y=" << y << std::endl;
          idx+=7;
        }
        break;
      case 'A':
        //"elliptical arc" command - absolute
        while (params.size() - idx >= 7){
          render_elliptical_arc(x, y, params[idx], params[idx+1], params[idx+2], params[idx+3], params[idx+4], params[idx+5], params[idx+6]);
          x = params[idx+5];
          y = params[idx+6];
          //std::cout << "A: x=" << x << " y=" << y << std::endl;
          idx+=7;
        }
        break;
    }

    start = result[1].second;
  }
}

#define NUMBER_REGEX "-?[0-9]+(\\.[0-9]+)?(e-?[0-9]+)?"
TransformMatrix SVGData::parse_transform(std::string transform){
//  std::cout << "Parsing SVG 'transform' atrtibute = '" << transform << "'" << std::endl;

  TransformMatrix tm;
  tm.setIdentity();

  boost::regex matrix_regex("\\s*matrix\\(\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")\\s*\\)");

  boost::regex translate_regex("\\s*translate\\(\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")?\\s*\\)");

  boost::regex scale_regex("\\s*scale\\(\\s*(" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX ")?\\s*\\)");

  boost::regex rotate_regex("\\s*rotate\\(\\s*(" NUMBER_REGEX ")\\s*,\\s*((" NUMBER_REGEX ")\\s*,\\s*(" NUMBER_REGEX "))?\\s*\\)");

  boost::regex skewX_regex("\\s*skewX\\(\\s*(" NUMBER_REGEX ")\\s*\\)");

  boost::regex skewY_regex("\\s*skewY\\(\\s*(" NUMBER_REGEX ")\\s*\\)");

  boost::match_results<std::string::const_iterator> result;

  std::string::const_iterator start, end;
  start = transform.begin();
  end = transform.end();

  bool continue_parsing = true;
  while(continue_parsing){
    continue_parsing = false;

    if (boost::regex_search(start, end, result, matrix_regex)){
      float a = atof(((std::string) result[1]).c_str());
      float b = atof(((std::string) result[4]).c_str());
      float c = atof(((std::string) result[7]).c_str());
      float d = atof(((std::string) result[10]).c_str());
      float e = atof(((std::string) result[13]).c_str());
      float f = atof(((std::string) result[16]).c_str());
      TransformMatrix transform_matrix(a, b, c, d, e, f);
      //std::cout << "found a matrix transform!" << std::endl;
      tm = tm * transform_matrix;
      start = result[0].second;
      continue_parsing = true;
    }

    if (boost::regex_search(start, end, result, translate_regex)){
      float tx = atof(((std::string) result[1]).c_str());
      float ty = atof(((std::string) result[4]).c_str());
      //std::cout << "found a translate transform!" << std::endl;
      tm.translate(tx, ty);
      start = result[0].second;
      continue_parsing = true;
    }

    if (boost::regex_search(start, end, result, scale_regex)){
      float sx = atof(((std::string) result[1]).c_str());
      float sy = atof(((std::string) result[4]).c_str());
      //std::cout << "found a scale transform!" << std::endl;
      tm.scale(sx, sy);
      start = result[0].second;
      continue_parsing = true;
    }

    if (boost::regex_search(start, end, result, rotate_regex)){
      float angle = atof(((std::string) result[1]).c_str());
      float cx = atof(((std::string) result[5]).c_str());
      float cy = atof(((std::string) result[8]).c_str());
      //std::cout << "found a rotate transform!" << std::endl;
      tm.rotate(angle, cx, cy);
      start = result[0].second;
      continue_parsing = true;
    }

    if (boost::regex_search(start, end, result, skewX_regex)){
      float angle = atof(((std::string) result[1]).c_str());
      //std::cout << "found a skewX transform!" << std::endl;
      tm.skewX(angle);
      start = result[0].second;
      continue_parsing = true;
    }

    if (boost::regex_search(start, end, result, skewY_regex)){
      float angle = atof(((std::string) result[1]).c_str());
      //std::cout << "found a skewY transform!" << std::endl;
      tm.skewY(angle);
      start = result[0].second;
      continue_parsing = true;
    }

  }

  return tm;
}

void SVGData::traverse_subtree(TransformMatrix parent_matrix, const xmlpp::Node* node){
  TransformMatrix tm = parent_matrix;

  Glib::ustring nodename = node->get_name();

  if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)){
    const xmlpp::Attribute* id = nodeElement->get_attribute("id");
    const xmlpp::Attribute* transform = nodeElement->get_attribute("transform");

    if (transform)
      tm = parent_matrix * parse_transform(transform->get_value());

    if (nodename == "g"){
      std::cout << "found a group!" << std::endl;
      if(id){
        std::cout << "id=" << id->get_value() << std::endl;      
      }
    }
  }

  setCurrentTransformMatrix(tm);

  if (nodename == "svg"){
    if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)){
      const xmlpp::Attribute* height = nodeElement->get_attribute("height");
      if(height){
        document_height = atof(height->get_value().c_str());
      }
    }
  }

  if (nodename == "path"){
    if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)){
      const xmlpp::Attribute* d = nodeElement->get_attribute("d");
      if(d){
        std::cout << "path description = " << d->get_value() << std::endl;      
        start_path();
        parse_path_description(d->get_value());
        close_path();
      }
    }
  }

  if (nodename == "rect"){
//    std::cout << "rectangle" << std::endl;

    if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)){
      const xmlpp::Attribute* width_attr = nodeElement->get_attribute("width");
      const xmlpp::Attribute* height_attr = nodeElement->get_attribute("height");
      const xmlpp::Attribute* x_attr = nodeElement->get_attribute("x");
      const xmlpp::Attribute* y_attr = nodeElement->get_attribute("y");

      float x, y, width, height;

      x = x_attr ? atof(x_attr->get_value().c_str()) : 0;
      y = y_attr ? atof(y_attr->get_value().c_str()) : 0;

      if(width_attr && height_attr){
        width = atof(width_attr->get_value().c_str());
        height = atof(height_attr->get_value().c_str());

        if(width > 0 && height > 0){
          render_rect(x, y, width, height);
//          std::cout << "x:" << x << " y:" << y << " width:" << width << " height:" << height << std::endl;
        }
      }
    }
  }

  xmlpp::Node::NodeList children = node->get_children();
  for (xmlpp::Node::NodeList::iterator it = children.begin(); it != children.end(); ++it){
    traverse_subtree(tm, *it);
  }
}

PolySet* SVGData::convertToPolyset(){
  if (!parser)
    return NULL;

  const xmlpp::Node* pNode = parser->get_document()->get_root_node();
  TransformMatrix tm;
  tm.setIdentity();
  traverse_subtree(tm, pNode);

	p->is2d = true;
	dxf_tesselate(p, *dxfdata, 0, true, false, 0);
	dxf_border_to_ps(p, *dxfdata);
  return p;
}

