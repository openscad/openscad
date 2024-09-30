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

#include <cassert>
#include <limits>
#include <ostream>
#include <memory>
#include "io/export.h"
#include "geometry/PolySet.h"

/*!
    Saves the current Polygon2d as DXF to the given absolute filename.
 */

void export_dxf_header(std::ostream& output, double xMin, double yMin, double xMax, double yMax) {

  // https://dxfwrite.readthedocs.io/en/latest/headervars.html
  // http://paulbourke.net/dataformats/dxf/min3d.html

  // based on: https://github.com/mozman/ezdxf/tree/master/examples_dxf
  // Minimal_DXF_AC1009.dxf - not working in Adobe Illustrator
  // Minimal_DXF_AC1006.dxf - not working with LibreCAD (due to 3DFACE?)

  // tested to work on:
  // - InkScape 1.0.0
  // - LibreCAD
  // - Adobe Illustrator
  // - https://sharecad.org
  // - generic cutters

  output
    << "999\n" << "DXF from OpenSCAD\n";

  //
  // SECTION 1
  //

  /* --- START --- */

  output
    << "  0\n" << "SECTION\n"
    << "  2\n" << "HEADER\n"
    << "  9\n" << "$ACADVER\n"
    << "  1\n" << "AC1006\n"
    << "  9\n" << "$INSBASE\n"
    << " 10\n" << "0.0\n"
    << " 20\n" << "0.0\n"
    << " 30\n" << "0.0\n"
  ;

  /* --- LIMITS --- */

  output
    << "  9\n" << "$EXTMIN\n"
    << " 10\n" << xMin << "\n"
    << " 20\n" << yMin << "\n"
    << "  9\n" << "$EXTMAX\n"
    << " 10\n" << xMax << "\n"
    << " 20\n" << yMax << "\n";

  output
    << "  9\n" << "$LINMIN\n"
    << " 10\n" << xMin << "\n"
    << " 20\n" << yMin << "\n"
    << "  9\n" << "$LINMAX\n"
    << " 10\n" << xMax << "\n"
    << " 20\n" << yMax << "\n";

  output
    << "  0\n" << "ENDSEC\n";

  //
  // SECTION 2
  //

  output
    << "  0\n" << "SECTION\n";

  output
    << "  2\n" << "TABLES\n";

  /* --- LINETYPE --- */

  output
    << "  0\n" << "TABLE\n"
    << "  2\n" << "LTYPE\n"
    << " 70\n" << "1\n"

    << "  0\n" << "LTYPE\n"
    << "  2\n" << "CONTINUOUS\n"       // linetype name
    << " 70\n" << "64\n"
    << "  3\n" << "Solid line\n"       // descriptive text
    << " 72\n" << "65\n"       // always 65
    << " 73\n" << "0\n"        // number of linetype elements
    << " 40\n" << "0.000000\n"       // total pattern length

    << "  0\n" << "ENDTAB\n";

  /* --- LAYERS --- */

  output
    << "  0\n" << "TABLE\n"
    << "  2\n" << "LAYER\n"
    << " 70\n" << "6\n"

    << "  0\n" << "LAYER\n"
    << "  2\n" << "0\n"         // layer name
    << " 70\n" << "64\n"
    << " 62\n" << "7\n"         // color
    << "  6\n" << "CONTINUOUS\n"

    << "  0\n" << "ENDTAB\n";

  /* --- STYLE --- */

  output
    << "  0\n" << "TABLE\n"
    << "  2\n" << "STYLE\n"
    << " 70\n" << "0\n"
    << "  0\n" << "ENDTAB\n";

  output
    << "  0\n" << "ENDSEC\n";

  //
  // SECTION 3
  //

  output
    << "  0\n" << "SECTION\n"
    << "  2\n" << "BLOCKS\n"
    << "  0\n" << "ENDSEC\n";

}

void export_dxf(const Polygon2d& poly, std::ostream& output)
{
  setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

  // find limits
  double xMin, yMin, xMax, yMax;
  xMin = yMin = std::numeric_limits<double>::max(),
  xMax = yMax = std::numeric_limits<double>::min();
  for (const auto& o : poly.outlines()) {
    for (const auto& p : o.vertices) {
      if (xMin > p[0]) xMin = p[0];
      if (xMax < p[0]) xMax = p[0];
      if (yMin > p[1]) yMin = p[1];
      if (yMax < p[1]) yMax = p[1];
    }
  }

  export_dxf_header(output, xMin, yMin, xMax, yMax);

  // REFERENCE:
  // DXF (AutoCAD Drawing Interchange Format) Family, ASCII variant
  //    https://www.loc.gov/preservation/digital/formats/fdd/fdd000446.shtml#specs
  // About the DXF Format (DXF)
  //    https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3
  // About ASCII DXF Files
  //    https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-20172853-157D-4024-8E64-32F3BD64F883
  // DXF Format
  //    https://documentation.help/AutoCAD-DXF/WSfacf1429558a55de185c428100849a0ab7-5f35.htm

  output << "  0\n" << "SECTION\n"
         << "  2\n" << "ENTITIES\n";

  for (const auto& o : poly.outlines()) {
    switch (o.vertices.size() ) {
    case 1: {
      // POINT: just in case it's supported in the future
      const Vector2d& p = o.vertices[0];
      output << "  0\n" << "POINT\n"
             << "100\n" << "AcDbEntity\n"
             << "  8\n" << "0\n" // layer 0
             << "100\n" << "AcDbPoint\n"
             << " 10\n" << p[0] << "\n" // x
             << " 20\n" << p[1] << "\n"; // y
    } break;
    case 2: {
      // LINE: just in case it's supported in the future
      // The [X1 Y1 X2 Y2] order is the most common and can be parsed linearly.
      // Some libraries, like the python libraries dxfgrabber and ezdxf, cannot open [X1 X2 Y1 Y2] order.
      const Vector2d& p1 = o.vertices[0];
      const Vector2d& p2 = o.vertices[1];
      output << "  0\n" << "LINE\n"
             << "100\n" << "AcDbEntity\n"
             << "  8\n" << "0\n" // layer 0
             << "100\n" << "AcDbLine\n"
             << " 10\n" << p1[0] << "\n" // x1
             << " 20\n" << p1[1] << "\n" // y1
             << " 11\n" << p2[0] << "\n" // x2
             << " 21\n" << p2[1] << "\n"; // y2
    } break;
    default:
      // LWPOLYLINE
      output << "  0\n" << "LWPOLYLINE\n"
             << "100\n" << "AcDbEntity\n"
             << "  8\n" << "0\n"      // layer 0
             << "100\n" << "AcDbPolyline\n"
             << " 90\n" << o.vertices.size() << "\n" // number of vertices
             << " 70\n" << "1\n";         // closed = 1
      for (const auto& p : o.vertices) {
        output << " 10\n" << p[0] << "\n"
               << " 20\n" << p[1] << "\n";
      }
      break;
    }
  }

  output << "  0\n" << "ENDSEC\n";
  output << "  0\n" << "EOF\n";

  setlocale(LC_NUMERIC, ""); // set default locale
}

void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      export_dxf(item.second, output);
    }
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    export_dxf(*poly, output);
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Export as DXF for this geometry type is not supported");
  }
}
