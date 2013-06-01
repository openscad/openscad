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

void SVGData::add_point(double x, double y){
  int this_point = -1;
  if (grid->has(x, y)) {
	  this_point = grid->align(x, y);
  } else {
	  this_point = grid->align(x, y) = dxfdata->points.size();
	  dxfdata->points.push_back(Vector2d(x, y));
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

std::vector<float> SVGData::get_params(std::string str){
  std::string pattern = "\\s*(-?[0-9]+(\\.[0-9]+)?),(-?[0-9]+(\\.[0-9]+)?)";
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
    std::cout << "value1=" << value << std::endl;

    value = atof(((std::string) result[3]).c_str());
    values.push_back(value);
    std::cout << "value2=" << value << std::endl;

    start = result[2].second;
  }

  return values;
}

void SVGData::render_line_to(float x0, float y0, float x1, float y1){
  add_point(x0,y0);
  add_point(x1,y1);
}

void SVGData::render_curve_to(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3){
//This is wrong! Implement-me correctly:
  add_point(x0,y0);
  add_point(x1,y1);
  add_point(x2,y2);
  add_point(x3,y3);
}

void SVGData::parse_path_description(Glib::ustring description){
  std::string d = (std::string) description;
  std::string pattern = "([a-zA-Z][^a-zA-Z]*)";
  boost::regex regexPattern(pattern);
  boost::match_results<std::string::const_iterator> result;

  std::string::const_iterator start, end;
  start = d.begin();
  end = d.end();

  float x=0, y=0;
  while(boost::regex_search(start, end, result, regexPattern)){
    std::cout << "result: " << std::endl << result[1] << std::endl << std::endl;
    const char* substring = ((std::string) result[1]).c_str();
    char instruction_code = substring[0];
    std::vector<float> params = get_params(&substring[1]);

    int idx=0;
    switch (instruction_code){
      case 'm':
        while (params.size() - idx >= 2){
          if (idx>0)
            render_line_to(x, y, x+params[0], y+params[1]);
          x += params[0];
          y += params[1];
          std::cout << "m: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'M':
        while (params.size() - idx >= 2){
          if (idx>0)
            render_line_to(x, y, params[0], params[1]);
          x = params[0];
          y = params[1];
          std::cout << "M: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'l':
        while (params.size() - idx >= 2){
          render_line_to(x, y, x+params[0], y+params[1]);
          x += params[0];
          y += params[1];
          std::cout << "l: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'L':
        while (params.size() - idx >= 2){
          render_line_to(x, y, params[0], params[1]);
          x = params[0];
          y = params[1];
          std::cout << "L: x=" << x << " y=" << y << std::endl;
          idx+=2;
        }
        break;
      case 'c':
        while (params.size() - idx >= 6){
          render_curve_to(x, y, x+params[idx], y+params[idx+1], x+params[idx+2], y+params[idx+3], x+params[idx+4], y+params[idx+5]);
          x += params[idx+4];
          y += params[idx+5];
          std::cout << "c: x=" << x << " y=" << y << std::endl;
          idx+=6;
        }
        break;
      case 'C':
        while (params.size() - idx >= 6){
          render_curve_to(x, y, params[0], params[1], params[2], params[3], params[4], params[5]);
          x = params[4];
          y = params[5];
          std::cout << "C: x=" << x << " y=" << y << std::endl;
          idx+=6;
        }
        break;
    }

    start = result[1].second;
  }
}

void SVGData::traverse_subtree(const xmlpp::Node* node){
  Glib::ustring nodename = node->get_name();
  if (nodename == "g"){
    std::cout << "found a group!" << std::endl;

    if(const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)){
      const xmlpp::Attribute* id = nodeElement->get_attribute("id");
      if(id){
        std::cout << "id=" << id->get_value() << std::endl;      
      }
    }
  }

  if (nodename== "path"){
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

  xmlpp::Node::NodeList children = node->get_children();
  for (xmlpp::Node::NodeList::iterator it = children.begin(); it != children.end(); ++it){
    traverse_subtree(*it);
  }
}

PolySet* SVGData::convertToPolyset(){
  if (!parser)
    return NULL;

  const xmlpp::Node* pNode = parser->get_document()->get_root_node();
  traverse_subtree(pNode);

	p->is2d = true;
	dxf_tesselate(p, *dxfdata, 0, true, false, 0);
	dxf_border_to_ps(p, *dxfdata);
  return p;
}

