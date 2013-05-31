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

SVGData::SVGData(std::string filename) : filename(filename) {
	handle_dep(filename); // Register ourselves as a dependency

  dxfdata = new DxfData();
  p = new PolySet();
  grid = new Grid2d<int>(GRID_COARSE);

	std::ifstream stream(filename.c_str());
	if (!stream.good()) {
		PRINTB("WARNING: Can't open SVG file '%s'.", filename);
		return;
	}
}

SVGData::~SVGData(){
  free(dxfdata);
  free(p);
  free(grid);
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

PolySet* SVGData::convertToPolyset(int fn){
  int R=15;
	p->is2d = true;

  start_path();
  for(float angle=0; angle<360; angle += 360.0/fn) {
	  double x = R*cos(2*3.1415*angle/360);
    double y = R*sin(2*3.1415*angle/360);
    add_point(x,y);
  }
  close_path();

  start_path();
  for(float angle=0; angle<360; angle += 360.0/fn) {
	  double x = R*cos(2*3.1415*angle/360);
    double y = R*sin(2*3.1415*angle/360);
    add_point(31+x,y);
  }
  close_path();

	dxf_tesselate(p, *dxfdata, 0, true, false, 0);
	dxf_border_to_ps(p, *dxfdata);
  return p;
}

