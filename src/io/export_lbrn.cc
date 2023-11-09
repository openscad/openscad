/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include "export.h"
#include "PolySet.h"

#include <iostream>

static void append_lbrn(const Polygon2d& poly, std::ostream& output, ExportLbrnOptions *options)
{
  BoundingBox bbox = poly.getBoundingBox();
  int minx = (int)floor(bbox.min().x());
  int miny = (int)floor(-bbox.max().y());
  int maxx = (int)ceil(bbox.max().x());
  int maxy = (int)ceil(-bbox.min().y());
  //int offset = 30;
  int offset = 0;
  //int width = maxx - minx + offset;
  int width = 0 + offset;
  int height = maxy - miny + offset;
  
  unsigned int cnt = 0;
  unsigned int last = 0;
  
  
  for (const auto& o : poly.outlines()) {
    output << "    <Shape Type=\"Path\" CutIndex=\"" << options->colorIndex << "\">\n";
    //output << "    <Shape Type=\"Path\" CutIndex=\"" << "1" << "\">\n";
    output << "        <XForm>1 0 0 1 " << width << " " << height << "</XForm>\n";
  	last = cnt;
  	
    if (o.vertices.empty()) {
      continue;
    }

    const Eigen::Vector2d& p0 = o.vertices[0];
    output << "        <V vx=\"" << p0.x() << "\" vy=\"" << -p0.y() << "\"/>\n";

    for (unsigned int idx = 1; idx < o.vertices.size(); ++idx) {
      const Eigen::Vector2d& p = o.vertices[idx];
      output << "        <V vx=\"" << p.x() << "\" vy=\"" << -p.y() << "\"/>\n";
    }

    for (unsigned int idx = 0; idx < o.vertices.size(); ++idx) {
      const Eigen::Vector2d& p = o.vertices[idx];
      if (idx != o.vertices.size()-1) {
        output << "        <P T=\"L\" p0=\"" << idx << "\" p1=\"" << idx+1 << "\"/>\n";
      } else {
      	last=0;
        output << "        <P T=\"L\" p0=\"" << idx << "\" p1=\"" << last << "\"/>\n";
      }
      cnt++;
    }

    output << "    </Shape>\n";
  }  
}

static void append_lbrn(const shared_ptr<const Geometry>& geom, std::ostream& output, ExportLbrnOptions *options)
{
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      append_lbrn(item.second, output, options);
    }
  } else if (const auto poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
    append_lbrn(*poly, output, options);
  } else if (dynamic_pointer_cast<const PolySet>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Export as LBRN for this geometry type is not supported");
  }
}

//void export_lbrn(const shared_ptr<const Geometry>& geom, std::ostream& output)
void export_lbrn(const shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo)
{
  // Extract the options.  This will change when options becomes a variant.
  ExportLbrnOptions *exportLbrnOptions;
  ExportLbrnOptions defaultLbrnOptions;

  ExportFileOptions efo = exportInfo.exportFileOptions;

  // could use short-circuit short-form, but will need to grow.
  if (exportInfo.lbrnOptions==NULL) {
    exportLbrnOptions=&defaultLbrnOptions;
    
    exportLbrnOptions->colorIndex = std::stoi(efo.get_option_value("lbrn", "colorIndex"));
    exportLbrnOptions->minPower   = std::stoi(efo.get_option_value("lbrn", "minPower"));
    exportLbrnOptions->maxPower   = std::stoi(efo.get_option_value("lbrn", "maxPower"));
    exportLbrnOptions->speed      = std::stoi(efo.get_option_value("lbrn", "speed"));
    exportLbrnOptions->numPasses  = std::stoi(efo.get_option_value("lbrn", "numPasses"));
    	
  } else {
    exportLbrnOptions=exportInfo.lbrnOptions;
  }

  setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

  output
    << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    << "<LightBurnProject AppVersion=\"1.4.03\" FormatVersion=\"0\" MaterialHeight=\"0\" MirrorX=\"False\" MirrorY=\"True\">\n"
    << "    <UIPrefs>\n"
    << "        <CutOrigin Value=\"" << exportLbrnOptions->colorIndex << "\"/>\n"
    << "        <AlignH Value=\"2\"/>\n"
    << "        <AlignV Value=\"0\"/>\n"
    << "        <Optimize_ByLayer Value=\"0\"/>\n"
    << "        <Optimize_ByGroup Value=\"-1\"/>\n"
    << "        <Optimize_ByPriority Value=\"1\"/>\n"
    << "        <Optimize_WhichDirection Value=\"0\"/>\n"
    << "        <Optimize_InnerToOuter Value=\"1\"/>\n"
    << "        <Optimize_ByDirection Value=\"0\"/>\n"
    << "        <Optimize_ReduceTravel Value=\"1\"/>\n"
    << "        <Optimize_HideBacklash Value=\"0\"/>\n"
    << "        <Optimize_ReduceDirChanges Value=\"0\"/>\n"
    << "        <Optimize_ChooseCorners Value=\"0\"/>\n"
    << "        <Optimize_AllowReverse Value=\"1\"/>\n"
    << "        <Optimize_RemoveOverlaps Value=\"0\"/>\n"
    << "        <Optimize_OptimalEntryPoint Value=\"1\"/>\n"
    << "        <Optimize_OverlapDist Value=\"0.025\"/>\n"
    << "    </UIPrefs>\n";

  output
    << "    <CutSetting type=\"Cut\">\n"
    << "        <index Value=\"" << exportLbrnOptions->colorIndex << "\"/>\n"
    << "        <name Value=\"C" << exportLbrnOptions->colorIndex << "\"/>\n"
    << "        <minPower Value=\"" << exportLbrnOptions->minPower << "\"/>\n"
    << "        <maxPower Value=\"" << exportLbrnOptions->maxPower << "\"/>\n"
    << "        <maxPower2 Value=\"" << exportLbrnOptions->maxPower << "\"/>\n"
    << "        <speed Value=\"" << exportLbrnOptions->speed << "\"/>\n"
    << "        <numPasses Value=\"" << exportLbrnOptions->numPasses << "\"/>\n"
    << "        <dotTime Value=\"1\"/>\n"
    << "        <dotSpacing Value=\"0.01\"/>\n"
    << "        <overscan Value=\"0\"/>\n"
    << "        <priority Value=\"0\"/>\n"
    << "        <tabSize Value=\"0.50038\"/>\n"
    << "        <tabCount Value=\"1\"/>\n"
    << "        <tabCountMax Value=\"1\"/>\n"
    << "        <tabSpacing Value=\"50.038\"/>\n"
    << "    </CutSetting>\n";

  append_lbrn(geom, output, exportLbrnOptions);

  output << "    <Notes ShowOnLoad=\"0\" Notes=\"\"/>\n";
  output << "</LightBurnProject>\n";
  setlocale(LC_NUMERIC, ""); // Set default locale
}
