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
#include "polyset.h"
#include "polyset-utils.h"
#include "dxfdata.h"

/*!
   Saves the current Polygon2d as DXF to the given absolute filename.
 */
void export_dxf(const Polygon2d &poly, std::ostream &output)
{
  setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
  // Some importers (e.g. Inkscape) needs a BLOCKS section to be present
  output << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "BLOCKS\n"
         << "  0\n"
         << "ENDSEC\n"
         << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "ENTITIES\n";

  for (const auto &o : poly.outlines()) {
    for (unsigned int i = 0; i < o.vertices.size(); i++) {
      const Vector2d &p1 = o.vertices[i];
      const Vector2d &p2 = o.vertices[(i + 1) % o.vertices.size()];
      double x1 = p1[0];
      double y1 = p1[1];
      double x2 = p2[0];
      double y2 = p2[1];
      output << "  0\n"
             << "LINE\n";
      // Some importers (e.g. Inkscape) needs a layer to be specified
      // The [X1 Y1 X2 Y2] order is the most common and can be parsed linearly.
      // Some libraries, like the python libraries dxfgrabber and ezdxf, cannot open [X1 X2 Y1 Y2] order.
      output << "  8\n"
             << "0\n"
             << " 10\n"
             << x1 << "\n"
             << " 20\n"
             << y1 << "\n"
             << " 11\n"
             << x2 << "\n"
             << " 21\n"
             << y2 << "\n";
    }
  }

  output << "  0\n"
         << "ENDSEC\n";

  // Some importers (e.g. Inkscape) needs an OBJECTS section with a DICTIONARY entry
  output << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "OBJECTS\n"
         << "  0\n"
         << "DICTIONARY\n"
         << "  0\n"
         << "ENDSEC\n";

  output << "  0\n"
         << "EOF\n";

  setlocale(LC_NUMERIC, "");      // Set default locale
}

void export_dxf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
  if (dynamic_cast<const PolySet *>(geom.get())) {
    assert(false && "Unsupported file format");
  }
  else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
    export_dxf(*poly, output);
  }
  else {
    assert(false && "Export as DXF for this geometry type is not supported");
  }
}
