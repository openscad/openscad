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
#include "io/export.h"
#include "io/import.h"

#include <cassert>
#include <clocale>
#include <cmath>
#include <memory>
#include <ostream>

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

void find_colormap_from_value(const boost::property_tree::ptree& pt, const unsigned int color,
                              std::string& label, int& power, int& feed)
{
  // set the default return value for not found pass through.
  power = feed = -1;

  if (pt.empty()) {
    std::cerr << "In find_colormap_from_value: ";
    std::cerr << "\"" << pt.data() << "\"" << std::endl;
    return;
  }

  BOOST_FOREACH (const boost::property_tree::ptree::value_type& v, pt) {
    try {
      const unsigned int ref_color = pt.get<unsigned int>(v.first + ".property.color");
      if (ref_color == color) {
        label = v.first;

        power = pt.get<int>(v.first + ".property.power");
        feed = pt.get<int>(v.first + ".property.feed");
        return;
      }
    } catch (const boost::property_tree::ptree_error& e) {
      // intintionally ignore the error -- not all parameters have an
      // associated color.
    }
  }
}

void output_gcode_pars(std::ostream& output, int gnum, double x, double y, double feed, double power)
{
  static int gnum_cached = -1;
  static double x_cached = NAN;
  static double y_cached = NAN;
  static double feed_cached = NAN;
  static double power_cached = NAN;

  if (gnum == -2) {
    // reinitialize the cache
    gnum_cached = -1;
    x_cached = NAN;
    y_cached = NAN;
    feed_cached = NAN;
    power_cached = NAN;

    return;
  }

  if (gnum != -1 && gnum != gnum_cached) {
    output << "G" << gnum;
    gnum_cached = gnum;
  }
  if (!std::isnan(x) && x != x_cached) {
    output << "X" << x;
    x_cached = x;
  }
  if (!std::isnan(y) && y != y_cached) {
    output << "Y" << y;
    y_cached = y;
  }
  if (!std::isnan(feed) && feed != feed_cached) {
    output << "F" << feed;
    feed_cached = feed;
  }
  if (!std::isnan(power) && power != power_cached) {
    output << "S" << power;
    power_cached = power;
  }

  output << "\r\n";
}

static double color_to_parm(const boost::property_tree::ptree& pt, const Color4f color, const int pos,
                            const int dynamic)
{
  int r, g, b, a;
  double parm;

  color.getRgba(r, g, b, a);
  unsigned int color_val;
  color_val = ((unsigned int)(r) << 24) | ((unsigned int)(g) << 16) | ((unsigned int)(b) << 8) |
              ((unsigned int)(a) & 0xFF);

  std::string label;
  int ipower, ifeed;

  switch (pos) {
  case 0:  // power
    if (dynamic == 1) {
      parm = double(color_val >> 22);
    } else {
      find_colormap_from_value(pt, color_val, label, ipower, ifeed);
      parm = double(ipower);
    }
    break;
  case 1:  // feed/speed
    if (dynamic == 1) {
      parm = double(((color_val & 0x3FFFFF) >> 8) | (((~(unsigned int)(a)) & 0xFF) << 16));
    } else {
      find_colormap_from_value(pt, color_val, label, ipower, ifeed);
      parm = double(ifeed);
    }
    break;
  default: fprintf(stderr, "Internal Error: invalid colar param position.\n"); return -1;
  }

  return (parm);
}

static void append_gcode(boost::property_tree::ptree pt, const Polygon2d& poly, std::ostream& output,
                         const ExportInfo& exportInfo, const int lasermode)
{
  auto options = exportInfo.optionsGcode;

  for (const auto& o : poly.outlines()) {
    const Eigen::Vector2d& p0 = o.vertices[0];
    const double laserpower = color_to_parm(pt, o.color, 0, lasermode);
    const double feedrate = color_to_parm(pt, o.color, 1, lasermode);

    output_gcode_pars(output, 0, p0.x(), p0.y(), NAN, NAN);
    output_gcode_pars(output, -1, NAN, NAN, NAN, laserpower);
    int n = o.vertices.size();
    for (unsigned int idx = 1; idx <= n; ++idx) {
      const Eigen::Vector2d& p = o.vertices[idx % n];
      output_gcode_pars(output, 1, p.x(), p.y(), feedrate, laserpower);
    }
    output_gcode_pars(output, -1, NAN, NAN, NAN, 0);
  }
}

static void append_gcode(boost::property_tree::ptree pt, const std::shared_ptr<const Geometry>& geom,
                         std::ostream& output, const ExportInfo& exportInfo, const int lasermode)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      append_gcode(pt, item.second, output, exportInfo, lasermode);
    }
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    append_gcode(pt, *poly, output, exportInfo, lasermode);
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {  // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else {  // NOLINT(bugprone-branch-clone)
    assert(false && "Export as SVG for this geometry type is not supported");
  }
}

// global accessible version of the machine config settings that are
// used by export_gcode for colormapping power and feed.
boost::property_tree::ptree _machineconfig_settings_;

void export_gcode(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                  const ExportInfo& exportInfo)
{
  setlocale(LC_NUMERIC, "C");  // Ensure radix is . (not ,) in output
  BoundingBox bbox = geom->getBoundingBox();

  auto options = exportInfo.optionsGcode;

  // check to see if MachineConfig overwrites the lasermode
  int lasermode;
  try {
    lasermode = _machineconfig_settings_.get<int>("default.property.lasermode");
    // if pt.get succeeds, then lasermode is set by MachineConfig

    // handle cases where MachineConfig sets an invalid lasermode
    if ((0 != lasermode) && (1 != lasermode)) {
      std::cerr << "\nWarning: invlaid lasermode (" << lasermode << ") obtained from MachineConfig.\n\n";
      lasermode = options->lasermode;
    }
  } catch (const boost::property_tree::ptree_error& e) {
    // MachineConfig does not have a lasermode setting, so grab the
    // value from the ExportGCode GUI
    lasermode = options->lasermode;
  }

  // ditto for initCode and exitCode
  std::string initCode, exitCode;
  try {
    initCode = _machineconfig_settings_.get<std::string>("default.property.initCode");
  } catch (const boost::property_tree::ptree_error& e) {
    initCode = options->initCode;
  }

  try {
    exitCode = _machineconfig_settings_.get<std::string>("default.property.exitCode");
  } catch (const boost::property_tree::ptree_error& e) {
    exitCode = options->exitCode;
  }

  // reset the cached parameters
  output_gcode_pars(output, -2, NAN, NAN, NAN, NAN);

  // begin converting the geometry into gcode
  output << initCode << "\r\n";
  if (lasermode == 1) {
    output << "M4 S0\r\n";
  } else {
    output << "M3 S0\r\n";
  }
  append_gcode(_machineconfig_settings_, geom, output, exportInfo, lasermode);
  output << "M5 S0\r\n";
  output << exitCode;
  setlocale(LC_NUMERIC, "");  // Set default locale
}
