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
#include <algorithm>
#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <ostream>

#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "io/export.h"

/*!
    Saves the current Polygon2d as DXF to the given absolute filename.
 */

static void export_dxf_header(std::ostream& output, double xMin, double yMin, double xMax, double yMax)
{
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

  output << "999\n"
         << "DXF from OpenSCAD\n";

  //
  // SECTION 1
  //

  /* --- START --- */

  output << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "HEADER\n"
         << "  9\n"
         << "$ACADVER\n"
         << "  1\n"
         << "AC1006\n"
         << "  9\n"
         << "$INSBASE\n"
         << " 10\n"
         << "0.0\n"
         << " 20\n"
         << "0.0\n"
         << " 30\n"
         << "0.0\n";

  /* --- LIMITS --- */

  output << "  9\n"
         << "$EXTMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"
         << "  9\n"
         << "$EXTMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n";

  output << "  9\n"
         << "$LINMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"
         << "  9\n"
         << "$LINMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n";

  output << "  0\n"
         << "ENDSEC\n";

  //
  // SECTION 2
  //

  output << "  0\n"
         << "SECTION\n";

  output << "  2\n"
         << "TABLES\n";

  /* --- LINETYPE --- */

  output << "  0\n"
         << "TABLE\n"
         << "  2\n"
         << "LTYPE\n"
         << " 70\n"
         << "1\n"

         << "  0\n"
         << "LTYPE\n"
         << "  2\n"
         << "CONTINUOUS\n"  // linetype name
         << " 70\n"
         << "64\n"
         << "  3\n"
         << "Solid line\n"  // descriptive text
         << " 72\n"
         << "65\n"  // always 65
         << " 73\n"
         << "0\n"  // number of linetype elements
         << " 40\n"
         << "0.000000\n"  // total pattern length

         << "  0\n"
         << "ENDTAB\n";

  /* --- LAYERS --- */

  output << "  0\n"
         << "TABLE\n"
         << "  2\n"
         << "LAYER\n"
         << " 70\n"
         << "6\n"

         << "  0\n"
         << "LAYER\n"
         << "  2\n"
         << "0\n"  // layer name
         << " 70\n"
         << "64\n"
         << " 62\n"
         << "7\n"  // color
         << "  6\n"
         << "CONTINUOUS\n"

         << "  0\n"
         << "ENDTAB\n";

  /* --- STYLE --- */

  output << "  0\n"
         << "TABLE\n"
         << "  2\n"
         << "STYLE\n"
         << " 70\n"
         << "0\n"
         << "  0\n"
         << "ENDTAB\n";

  output << "  0\n"
         << "ENDSEC\n";

  //
  // SECTION 3
  //

  output << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "BLOCKS\n"
         << "  0\n"
         << "ENDSEC\n";
}

// AutoCAD 2020 model-space palette. Index 0 is unused. Group 420 is the exact
// RGB; group 62 is the nearest ACI for readers that ignore true color.
static constexpr uint32_t kAciRgb[256] = {
  0x000000, 0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF, 0xFFFFFF, 0x808080, 0xC0C0C0,
  0xFF0000, 0xFF7F7F, 0xA50000, 0xA55252, 0x7F0000, 0x7F3F3F, 0x4C0000, 0x4C2626, 0x260000, 0x261313,
  0xFF3F00, 0xFF9F7F, 0xA52900, 0xA56752, 0x7F1F00, 0x7F4F3F, 0x4C1300, 0x4C2F26, 0x260900, 0x261713,
  0xFF7F00, 0xFFBF7F, 0xA55200, 0xA57C52, 0x7F3F00, 0x7F5F3F, 0x4C2600, 0x4C3926, 0x261300, 0x261C13,
  0xFFBF00, 0xFFDF7F, 0xA57C00, 0xA59152, 0x7F5F00, 0x7F6F3F, 0x4C3900, 0x4C4226, 0x261C00, 0x262113,
  0xFFFF00, 0xFFFF7F, 0xA5A500, 0xA5A552, 0x7F7F00, 0x7F7F3F, 0x4C4C00, 0x4C4C26, 0x262600, 0x262613,
  0xBFFF00, 0xDFFF7F, 0x7CA500, 0x91A552, 0x5F7F00, 0x6F7F3F, 0x394C00, 0x424C26, 0x1C2600, 0x212613,
  0x7FFF00, 0xBFFF7F, 0x52A500, 0x7CA552, 0x3F7F00, 0x5F7F3F, 0x264C00, 0x394C26, 0x132600, 0x1C2613,
  0x3FFF00, 0x9FFF7F, 0x29A500, 0x67A552, 0x1F7F00, 0x4F7F3F, 0x134C00, 0x2F4C26, 0x092600, 0x172613,
  0x00FF00, 0x7FFF7F, 0x00A500, 0x52A552, 0x007F00, 0x3F7F3F, 0x004C00, 0x264C26, 0x002600, 0x132613,
  0x00FF3F, 0x7FFF9F, 0x00A529, 0x52A567, 0x007F1F, 0x3F7F4F, 0x004C13, 0x264C2F, 0x002609, 0x135817,
  0x00FF7F, 0x7FFFBF, 0x00A552, 0x52A57C, 0x007F3F, 0x3F7F5F, 0x004C26, 0x264C39, 0x002613, 0x13581C,
  0x00FFBF, 0x7FFFDF, 0x00A57C, 0x52A591, 0x007F5F, 0x3F7F6F, 0x004C39, 0x264C42, 0x00261C, 0x135858,
  0x00FFFF, 0x7FFFFF, 0x00A5A5, 0x52A5A5, 0x007F7F, 0x3F7F7F, 0x004C4C, 0x264C4C, 0x002626, 0x135858,
  0x00BFFF, 0x7FDFFF, 0x007CA5, 0x5291A5, 0x005F7F, 0x3F6F7F, 0x00394C, 0x26427E, 0x001C26, 0x135858,
  0x007FFF, 0x7FBFFF, 0x0052A5, 0x527CA5, 0x003F7F, 0x3F5F7F, 0x00264C, 0x26397E, 0x001326, 0x131C58,
  0x003FFF, 0x7F9FFF, 0x0029A5, 0x5267A5, 0x001F7F, 0x3F4F7F, 0x00134C, 0x262F7E, 0x000926, 0x131758,
  0x0000FF, 0x7F7FFF, 0x0000A5, 0x5252A5, 0x00007F, 0x3F3F7F, 0x00004C, 0x26267E, 0x000026, 0x131358,
  0x3F00FF, 0x9F7FFF, 0x2900A5, 0x6752A5, 0x1F007F, 0x4F3F7F, 0x13004C, 0x2F267E, 0x090026, 0x171358,
  0x7F00FF, 0xBF7FFF, 0x5200A5, 0x7C52A5, 0x3F007F, 0x5F3F7F, 0x26004C, 0x39267E, 0x130026, 0x1C1358,
  0xBF00FF, 0xDF7FFF, 0x7C00A5, 0x9152A5, 0x5F007F, 0x6F3F7F, 0x39004C, 0x42264C, 0x1C0026, 0x581358,
  0xFF00FF, 0xFF7FFF, 0xA500A5, 0xA552A5, 0x7F007F, 0x7F3F7F, 0x4C004C, 0x4C264C, 0x260026, 0x581358,
  0xFF00BF, 0xFF7FDF, 0xA5007C, 0xA55291, 0x7F005F, 0x7F3F6F, 0x4C0039, 0x4C2642, 0x26001C, 0x581358,
  0xFF007F, 0xFF7FBF, 0xA50052, 0xA5527C, 0x7F003F, 0x7F3F5F, 0x4C0026, 0x4C2639, 0x260013, 0x58131C,
  0xFF003F, 0xFF7F9F, 0xA50029, 0xA55267, 0x7F001F, 0x7F3F4F, 0x4C0013, 0x4C262F, 0x260009, 0x581317,
  0x000000, 0x656565, 0x666666, 0x999999, 0xCCCCCC, 0xFFFFFF,
};

static int dxfChannel(float value)
{
  const float clamped = std::clamp(value, 0.0f, 1.0f);
  return std::clamp(static_cast<int>(std::lround(clamped * 255.0f)), 0, 255);
}

static int nearestAci(int r, int g, int b)
{
  int best = 7;
  int bestDist = 1 << 30;
  for (int index = 1; index < 256; ++index) {
    const int cr = static_cast<int>((kAciRgb[index] >> 16) & 0xff);
    const int cg = static_cast<int>((kAciRgb[index] >> 8) & 0xff);
    const int cb = static_cast<int>(kAciRgb[index] & 0xff);
    const int dr = cr - r;
    const int dg = cg - g;
    const int db = cb - b;
    const int dist = dr * dr + dg * dg + db * db;
    if (dist < bestDist) {
      bestDist = dist;
      best = index;
    }
  }
  return best;
}

static void writeDxfColor(std::ostream& output, const Color4f& color)
{
  if (!color.hasRgb()) return;
  const int r = dxfChannel(color.r());
  const int g = dxfChannel(color.g());
  const int b = dxfChannel(color.b());
  const int truecolor = (r << 16) | (g << 8) | b;
  output << " 62\n"
         << nearestAci(r, g, b) << "\n"
         << "420\n"
         << truecolor << "\n";
}

static void export_dxf(const Polygon2d& poly, std::ostream& output)
{
  setlocale(LC_NUMERIC, "C");  // Ensure radix is . (not ,) in output

  // find limits
  double xMin, yMin, xMax, yMax;
  xMin = yMin = std::numeric_limits<double>::max(), xMax = yMax = std::numeric_limits<double>::min();
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

  output << "  0\n"
         << "SECTION\n"
         << "  2\n"
         << "ENTITIES\n";

  for (const auto& o : poly.outlines()) {
    switch (o.vertices.size()) {
    case 1: {
      // POINT: just in case it's supported in the future
      const Vector2d& p = o.vertices[0];
      output << "  0\n"
             << "POINT\n"
             << "100\n"
             << "AcDbEntity\n"
             << "  8\n"
             << "0\n";  // layer 0
      writeDxfColor(output, o.color);
      output << "100\n"
             << "AcDbPoint\n"
             << " 10\n"
             << p[0] << "\n"  // x
             << " 20\n"
             << p[1] << "\n";  // y
    } break;
    case 2: {
      // LINE: just in case it's supported in the future
      // The [X1 Y1 X2 Y2] order is the most common and can be parsed linearly.
      // Some libraries, like the python libraries dxfgrabber and ezdxf, cannot open [X1 X2 Y1 Y2] order.
      const Vector2d& p1 = o.vertices[0];
      const Vector2d& p2 = o.vertices[1];
      output << "  0\n"
             << "LINE\n"
             << "100\n"
             << "AcDbEntity\n"
             << "  8\n"
             << "0\n";  // layer 0
      writeDxfColor(output, o.color);
      output << "100\n"
             << "AcDbLine\n"
             << " 10\n"
             << p1[0] << "\n"  // x1
             << " 20\n"
             << p1[1] << "\n"  // y1
             << " 11\n"
             << p2[0] << "\n"  // x2
             << " 21\n"
             << p2[1] << "\n";  // y2
    } break;
    default:
      // LWPOLYLINE
      output << "  0\n"
             << "LWPOLYLINE\n"
             << "100\n"
             << "AcDbEntity\n"
             << "  8\n"
             << "0\n";  // layer 0
      writeDxfColor(output, o.color);
      output << "100\n"
             << "AcDbPolyline\n"
             << " 90\n"
             << o.vertices.size() << "\n"  // number of vertices
             << " 70\n"
             << "1\n";  // closed = 1
      for (const auto& p : o.vertices) {
        output << " 10\n"
               << p[0] << "\n"
               << " 20\n"
               << p[1] << "\n";
      }
      break;
    }
  }

  output << "  0\n"
         << "ENDSEC\n";
  output << "  0\n"
         << "EOF\n";

  setlocale(LC_NUMERIC, "");  // set default locale
}

void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      export_dxf(item.second, output);
    }
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    export_dxf(*poly, output);
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {  // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else {  // NOLINT(bugprone-branch-clone)
    assert(false && "Export as DXF for this geometry type is not supported");
  }
}
